import matplotlib.pyplot as plt

 # Funktion zum Einlesen von Messdaten aus einer Datei
 def lese_messdaten(dateiname):
    try:
        with open(dateiname, 'r') as file:
            daten = [float(zeile.strip()) for zeile in file if zeile.strip()]
        return daten
    except FileNotFoundError:
        print(f"Datei '{dateiname}' wurde nicht gefunden.")
        return []
    except ValueError:
        print("Fehler beim Verarbeiten der Datei. Stellen Sie sicher, dass die Datei nur Zahlen ent
        return []
        
 # Mittelwert berechnen
 def berechne_mittelwert(daten):
    if not daten:
        return None
    return sum(daten) / len(daten)
 
 # Funktion zum Plotten der Messdaten
 def plot_daten(daten, mittelwert):
    plt.figure(figsize=(10, 6))
    plt.plot(daten, label='Messwerte', marker='o')
    plt.axhline(mittelwert, color='red', linestyle='--', label=f'Mittelwert: {mittelwert
    plt.title('Messwerte und Mittelwert')
    plt.xlabel('Index')
    plt.ylabel('Wert')
    plt.legend()
    plt.grid(True)
    plt.show()
    
 # Hauptfunktion
 def ausfuehren(dateiname):
    daten = lese_messdaten(dateiname)
    
    if not daten:
        return
 
    mittelwert = berechne_mittelwert(daten)
    
    if mittelwert is not None:
        print(f"Mittelwert der Messreihe: {mittelwert:.2f}")
        plot_daten(daten, mittelwert)
    else:
        print("Es wurden keine gültigen Daten gefunden.")
 
 # Aufruf der Funktion mit den Daten des Bodenfeuchtesensors
 ausfuehren('A0_trocken.txt')
 ausfuehren('A0_nass.txt')
 
 # Aufruf der Funktion mit den Daten des Füllstandsensors
 ausfuehren('A1_leer.txt')
 ausfuehren('A1_voll.txt')
