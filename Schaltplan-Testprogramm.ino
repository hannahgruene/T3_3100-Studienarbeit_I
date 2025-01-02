#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

RTC_DS3231 rtc;

File dataFile;

const int chipSelect = 10;

int sensorPin = A0;  // Der Pin, an dem der kapazitive Sensor angeschlossen ist
int sensorValue = 0; // Variable für den Sensorwert

void setup() {
  // Serielle Kommunikation starten
  Serial.begin(9600);
  while (!Serial) {
    ; //Warten auf Serielle verbindung
  }

  pinMode (A0, INPUT); //Sensor als Input

  // SD-Karte initialisieren
  if (!SD.begin(10)) {
    Serial.println("SD-Karte konnte nicht initialisiert werden.");
    return;
  }
  Serial.println("SD-Karte bereit.");

  // RTC initialisieren
  if (!rtc.begin()) {
    Serial.println("Kein RTC gefunden");
    while (1);
  }

  // RTC prüfen, ob die Zeit korrekt eingestellt ist
  if (rtc.lostPower()) {
    Serial.println("RTC hat die Zeit verloren, stelle sie ein.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Setzt die Zeit auf die Kompilierzeit
  }


  // Datei öffnen oder neu erstellen
  dataFile = SD.open("time_log.json", FILE_WRITE);
  if (dataFile) {
    dataFile.println("[");  // Beginn der JSON-Datei
    dataFile.close();
  } else {
    Serial.println("Fehler beim Öffnen der Datei.");
  }
}

void loop() {
  delay(3600);
  // Zeit von der RTC holen
  DateTime now = rtc.now();

  // Sensorwert lesen
  int sensorValue = analogRead(A0); // Lese den Wert des kapazitiven Sensors
  Serial.println(sensorValue);
  delay(1000);

  // Öffne die JSON-Datei im Append-Modus
  dataFile = SD.open("time_log.json", FILE_WRITE);
  if (dataFile) {
    // Formatiere die Zeit und den Sensorwert als JSON-Objekt
    dataFile.print("{");
    dataFile.print("\"year\": ");
    dataFile.print(now.year());
    dataFile.print(", \"month\": ");
    dataFile.print(now.month());
    dataFile.print(", \"day\": ");
    dataFile.print(now.day());
    dataFile.print(", \"hour\": ");
    dataFile.print(now.hour());
    dataFile.print(", \"minute\": ");
    dataFile.print(now.minute());
    dataFile.print(", \"second\": ");
    dataFile.print(now.second());
    dataFile.print(", \"sensorValue\": ");
    dataFile.print(sensorValue);  // Speichere den Sensorwert
    dataFile.println("},");
    
    dataFile.close();  // Datei schließen
  } else {
    Serial.println("Fehler beim Öffnen der Datei.");
  }

  // Warten, bis die nächste Minute kommt
  delay(60000);  // 1 Minute
}

