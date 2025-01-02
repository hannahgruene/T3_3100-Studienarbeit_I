// Pin-Definition
const int sensorPin = A1;  // Analog Pin für den Sensor
// A0 für Bodenfeuchtesensor; A1 für Füllstandsensor 

// Variablen für Zeitsteuerung
unsigned long previousMillis = 0; // vorherige Zeit
const unsigned long interval = 30000; // Intervall: 30 Sekunden (in Millisekunden)

void setup() {
  // Serielle Kommunikation starten
  Serial.begin(9600);
  Serial.println("Sensor-Testprogramm gestartet");
  
  // Sensor-Pin konfigurieren
  pinMode(sensorPin, INPUT);
}

void loop() {
  // Aktuelle Zeit erfassen
  unsigned long currentMillis = millis();

  // Überprüfen, ob 30 Sekunden vergangen sind
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Zeit zurücksetzen

    // Sensorwert lesen
    int sensorValue = analogRead(sensorPin);
    
    // Sensorwert auf der seriellen Konsole ausgeben
    Serial.println(sensorValue);
	// Die Ausgabe wird anschließend in eine .txt-Datei kopiert
  }
}
