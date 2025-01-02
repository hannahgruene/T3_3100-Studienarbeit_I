// ******************************************
// ******* Studienarbeit Hannah Grüne *******
// ******************************************

// ************** Bibliotheken **************
#include <Wire.h>
#include <RTClib.h>
#include <vector>
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

// ************** Globale Variablen **************
RTC_DS1307 rtc; // RTC-Objekt zur Zeitmessung
const char* jsonFilePath = "plants.txt"; // Pfad zur JSON-Datei
const int chipSelect = 10; // SD-Karten Chip-Select-Pin

// Vorwärtsdeklarationen der Klassen und Variablen
class Plant;
class LevelSensor;
class Pump;

std::vector<Plant> plants; // Liste der Pflanzen
LevelSensor* levelSensor; // Zeiger auf den Füllstandssensor
Pump* pump; // Zeiger auf die Pumpe

// ************** Klassendefinitionen **************

// Basisklasse für Sensoren
class Sensor {
public:
    virtual float readValue() = 0; // Virtuelle Funktion zur Wertablesung
};

// Feuchtigkeitssensor
class HumiditySensor : public Sensor {
private:
    int pin, minValue, maxValue; // Sensor-Pin und Kalibrierungswerte
public:
    HumiditySensor(int p, int minV, int maxV) : pin(p), minValue(minV), maxValue(maxV) {}
    
    // Messwert normiert in Prozent
    float readValue() override {
        int analogValue = analogRead(pin);
        return map(analogValue, minValue, maxValue, 0, 100);
    }
};

// Füllstandssensor
class LevelSensor : public Sensor {
private:
    int pin, minValue, maxValue; // Sensor-Pin und Kalibrierungswerte
public:
    LevelSensor(int p, int minV, int maxV) : pin(p), minValue(minV), maxValue(maxV) {}

    // Messwert normiert in Prozent
    float readValue() override {
        int analogValue = analogRead(pin);
        return map(analogValue, minValue, maxValue, 0, 100);
    }
};

// Pumpensteuerung
class Pump {
private:
    int pin; // Pin für Pumpensteuerung
public:
    Pump(int p) : pin(p) {
        pinMode(pin, OUTPUT);
        stop(); // Standardmäßig Pumpe aus
    }
    void start() { digitalWrite(pin, HIGH); } // Pumpe einschalten
    void stop() { digitalWrite(pin, LOW); } // Pumpe ausschalten
};

// Klasse für Messungen
class Measurements {
public:
    std::vector<JsonObject> values; // Liste der Messungen

    // Fügt eine neue Messung hinzu
    void addMeasurement(const String& timestamp, float humidity) {
    DynamicJsonDocument* doc = new DynamicJsonDocument(128); // Dynamischer Speicher
    (*doc)["timestamp"] = timestamp;
    (*doc)["humidity"] = humidity;
    values.push_back(doc->as<JsonObject>()); // Objekt hinzufügen
    serializeJsonPretty(*doc, Serial); // Debug-Ausgabe
  }

};

// Klasse für Gießhistorie
class Watering {
public:
    std::vector<String> entries; // Liste der Einträge

    // Fügt einen neuen Eintrag hinzu
    void update(const String& entry) {
        entries.push_back(entry);
    }
};

// Pflanze mit Feuchtigkeitssensor und Pumpe
class Plant {
public:
    int id; // Pflanzen-ID
    String name, icon; // Pflanzenname und Icon
    Measurements measurements; // Messwerte
    Watering watering; // Gießhistorie
    HumiditySensor humiditySensor; // Feuchtigkeitssensor
    Pump& pump; // Pumpe
    float humidityThreshold; // Schwellenwert für Bewässerung

    // Konstruktor
    Plant(int i, String n, int humidityPin, int humidityMin, int humidityMax, Pump& p, String ic, float threshold)
        : id(i), name(n), humiditySensor(humidityPin, humidityMin, humidityMax), pump(p), icon(ic), humidityThreshold(threshold) {}

    // Messungen aktualisieren
    void updateMeasurements() {
    DateTime now = rtc.now();
    String timestamp = formatTimestamp(now);
    float humidity = humiditySensor.readValue();
    measurements.addMeasurement(timestamp, humidity);
}

    // Überprüfung, ob Bewässerung benötigt wird
    bool needsWatering() {
        float humidity = humiditySensor.readValue();
        return humidity < humidityThreshold;
    }

    // Überprüfung, ob überwässert wird - Begrenzung auf 3 Gießzyklen
    bool canWaterMoreToday() {
        DateTime now = rtc.now();
        String today = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day());
        int waterCount = 0;

        for (const String& entry : watering.entries) {
            if (entry.startsWith(today)) {
                waterCount++;
            }
        }

        return waterCount < 3; // Maximal 3 Mal pro Tag gießen
    }

    // Pflanze bewässern
    void waterPlant() {
        DateTime now = rtc.now();
        String entry = formatTimestamp(now);
        watering.update(entry);
        pump.start(); // Pumpe starten
        delay(10000); // 10 Sekunden Bewässerung
        pump.stop(); // Pumpe stoppen
    }

    // Zeitstempel formatieren
    // Bei der vollen Stunde können die Minuten und Sekunden genullt werden
    String formatTimestamp(const DateTime& now) {
        return String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) +
               "T" + String(now.hour()) + ":00:00Z");
    }
    // Zeitstempel für Testprogramm:
    /*String formatTimestamp(const DateTime& now) {
        return String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) +
               "T" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second() + "Z");
    }*/

    // Überprüfen, ob die Pflanze in Ordnung ist
    bool isOk() {
        return !needsWatering();
    }
};

// ************** JSON-Operationen **************
void createJsonFileIfNotExists() {
    if (!SD.exists(jsonFilePath)) {
        Serial.println("JSON-Datei existiert nicht. Erstelle eine neue Datei...");
        saveToJson(); // Speichert eine leere oder initialisierte Datei
    }
}

// JSON-Daten laden
void loadFromJson() {
    File file = SD.open(jsonFilePath, FILE_READ);
    if (!file) {
        Serial.println("JSON-Datei konnte nicht geöffnet werden.");
        saveToJson();
        return;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Fehler beim Lesen der JSON-Datei: ");
        Serial.println(error.f_str());
        file.close();
        return;
    }

    JsonArray plantsArray = doc["plants"].as<JsonArray>();
    for (JsonObject plantObj : plantsArray) {
        int id = plantObj["id"];
        String name = plantObj["name"];
        JsonArray measurements = plantObj["measurements"];
        JsonArray watering = plantObj["watering"];

        for (Plant& plant : plants) {
            if (plant.id == id) {
                for (JsonObject measurement : measurements) {
                    String timestamp = measurement["timestamp"];
                    float humidity = measurement["humidity"];
                    plant.measurements.addMeasurement(timestamp, humidity);
                }
                for (JsonObject waterEntry : watering) {
                    String entry = waterEntry["entry"];
                    plant.watering.update(entry);
                }
                break;
            }
        }
    }

    file.close();
    Serial.println("JSON geladen.");
}

// JSON-Daten speichern
void saveToJson() {
    StaticJsonDocument<2048> doc;
    JsonArray plantsArray = doc.createNestedArray("plants");
    
    for (Plant& plant : plants) {
        JsonObject plantObj = plantsArray.createNestedObject();
        plantObj["id"] = plant.id;
        plantObj["name"] = plant.name;

        JsonArray measurementsArray = plantObj.createNestedArray("measurements");
        for (const JsonObject& m : plant.measurements.values) {
            measurementsArray.add(m);
        }

        JsonArray wateringArray = plantObj.createNestedArray("watering");
        for (const String& entry : plant.watering.entries) {
            JsonObject wateringEntry = wateringArray.createNestedObject();
            wateringEntry["entry"] = entry;
        }
        plantObj["icon"] = plant.icon;
    }

    JsonArray notifications = doc.createNestedArray("notifications");

    if (levelSensor->readValue() <= 10) {
        JsonObject lowWaterNotification = notifications.createNestedObject();
        lowWaterNotification["code"] = "water_level_too_low";
        lowWaterNotification["status"] = true;
        lowWaterNotification["error_message"] = "Please refill your water!";
    }

    for (Plant& plant : plants) {
        if (!plant.isOk()) {
            if (!plant.canWaterMoreToday() && plant.needsWatering()) {
                JsonObject wetSoilNotification = notifications.createNestedObject();
                wetSoilNotification["code"] = "soil_too_wet";
                wetSoilNotification["status"] = false;
                wetSoilNotification["error_message"] = "One of your plants is drowning :(";
            }
        }
    }

    JsonArray updatedNotifications = doc["notifications"].as<JsonArray>();

    for (int i = updatedNotifications.size() - 1; i >= 0; i--) {
        JsonObject notification = updatedNotifications[i];

        if (notification["code"] == "water_level_too_low" && levelSensor->readValue() > 10) {
            updatedNotifications.remove(i);
        }
        if (notification["code"] == "soil_too_wet") {
            bool plantOk = true;
            for (Plant& plant : plants) {
                if (!plant.isOk()) {
                    plantOk = false;
                    break;
                }
            }
            if (plantOk) {
                updatedNotifications.remove(i);
            }
        }
    }

    File file = SD.open(jsonFilePath, O_WRITE);
    if (file) {
        serializeJsonPretty(doc, file);
        file.close();
        Serial.println("JSON gespeichert.");
    } else {
        Serial.println("Fehler beim Öffnen der JSON-Datei zum Schreiben.");
    }
}

// ************** Setup und Loop **************
LevelSensor levelSensorInstance(A1, 387, 243); // Füllstandssensor Pin und Analogwertgrenzen
Pump pumpInstance(3); // Pumpenpin auf Pin 3

void setup() {
    delay(10000); // Startverzögerung zur Initialisierung
    Serial.begin(9600);
    Serial.println("Setup gestartet...");

    // RTC initialisieren
    if (!rtc.begin()) {
        Serial.println("RTC nicht gefunden!");
        while (1); // Anhalten bei Fehler
    } else {
        Serial.println("RTC erfolgreich initialisiert.");
    }

    if (!rtc.isrunning()) {
        Serial.println("RTC läuft nicht, Zeit wird gesetzt!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Zeit auf Kompiler-Zeit setzen
    }

    // SD-Karte initialisieren
    if (!SD.begin(chipSelect)) {
        Serial.println("SD-Karte konnte nicht initialisiert werden.");
        return;
    }

    // Füllstandssensor und Pumpe zuweisen
    levelSensor = &levelSensorInstance;
    pump = &pumpInstance;

    // Pflanzeninitialisierung:
    // ID (aufsteigend)
    // Name der Pflanze
    // Pinpelegung des Bodenfeuchtesensors
    // Untere Grenze des Bodenfeuchtesensors (analog)
    // Obere Grenze (analog)
    // Zuordnung der Pumpe (bei mehreren Pflanzen ggfs mehrere Pumpen notwendig)
    // Icon der Pflanze (für App)
    // Schwellenwert zur Bewässerung (in %)
    plants.push_back(Plant(1, "Beaucarnea", A0, 577, 243, *pump, "beaucarnea", 50.0));
    
    // Beispielinitialisierung einer zweiten Pflanze
    //plants.push_back(Plant(2, "Alocasia", A2, 600, 250, *pump, "alocasia", 35.0));

    // JSON-Daten aus der Datei laden
    createJsonFileIfNotExists(); // JSON-Datei erstellen, falls sie nicht existiert
    loadFromJson();
}

void loop() {
    static unsigned long lastUpdate = 0;

    // Prüfung zur vollen Stunde
    if (millis() - lastUpdate >= 300000) { // Für den Funktionstest wird hier 5min (300000) verwendet
        lastUpdate = millis();

        for (Plant& plant : plants) {
            plant.updateMeasurements(); //Messwerterfassung

            // Gießlogik: Füllstand ok, Pflanze benötigt Wasser, Limit an Gießzyklen/Tag nicht überschritten
            if (levelSensor->readValue() > 10 && plant.needsWatering() && plant.canWaterMoreToday()) {
                plant.waterPlant();
            }
        }
        saveToJson(); // Speichern der aktualisierten Daten in die JSON-Datei
    }
}