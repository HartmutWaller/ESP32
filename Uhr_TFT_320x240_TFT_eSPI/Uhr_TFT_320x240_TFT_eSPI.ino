#ifdef ESP8266
  #include "ESP8266WiFi.h"

#else 
  #include "WiFi.h"
#endif

#include "time.h"
#include "TFT_eSPI.h"
#include "DHT.h"

int SENSOR_DHT = 22;

#define SensorTyp DHT22

// Sensor einen Namen zuweisen
DHT dht(SENSOR_DHT, SensorTyp); 

TFT_eSPI tft = TFT_eSPI();

// WiFi-Daten
char Router[] = "Router_SSID;
char Passwort[] = "xxxxxxxx";

// Variablen des TFTs (Höhe, Breite, Radius)
const int MitteHoehe = 120;
const int MitteBreite = 120;
const int Radius = 120;

// Multiplikatoren für x- y-Positionen der Stunden, Minuten und Sekunden
float MinutePosX = 0, MinutePosY = 0, StundePosX = 0, StundePosY = 0;
float GradSekunden = 0, GradMinuten = 0, GradStunden = 0;

// x- y-Koordinaten für die Anzeige Stunden, Minuten und Sekunden
int MinutenZeigerX = MitteHoehe, MinutenZeigerY = MitteHoehe;
int StundenZeigerX = MitteHoehe, StundenZeigerY = MitteHoehe; 

// Start wird nur beim ersten Start für den Aufbau des TFTs benötigt
bool Start = 1;

// Variablen für die Markierungen und Punkte und Striche des Ziffernblatts
float PosX, PosY;
int PunktX, PunktY, PunktX1, PunktX2, PunktY1, PunktY2;

// Variablen für die Zeit
int Stunden, Minuten, Sekunden;

// Farben
#define SCHWARZ     0x0000
#define WEISS       0xFFFF
#define BLAU        0x001F
#define ROT         0xF800
#define GRUEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define GELB        0xFFE0
#define BRAUN       0x9A60
#define GRAU        0x7BEF
#define GRUENGELB   0xB7E0
#define DUNKELCYAN  0x03EF
#define ORANGE      0xFDA0
#define PINK        0xFE19
#define BORDEAUX    0xA000
#define HELLBLAU    0x867D
#define VIOLETT     0x915C
#define SILBER      0xC618
#define GOLD        0xFEA0

// Farben innerer Kreis, Randfarbe und Zeigerfarbe
// die Farben der Zeiger können aber auch individuell gesetzt werden
const int Kreisfarbe = SCHWARZ;
const int Zeigerfarbe = WEISS;
const int Randfarbe = BORDEAUX;

// true -> Datum anzeigen
// false -> Datum nicht anzeigen
bool DatumAnzeigen = true;

// Ziffern 12 3 6 9 anzeigen/nicht anzeigen
bool Ziffernanzeigen = true;

// NTP-Server aus dem Pool
#define Zeitserver "de.pool.ntp.org"

/*
  Liste der Zeitzonen
  https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  Zeitzone CET = Central European Time -1 -> 1 Stunde zurück
  CEST = Central European SuMinutener Time von
  M3 = März, 5.0 = Sonntag 5. Woche, 02 = 2 Uhr
  bis M10 = Oktober, 5.0 = Sonntag 5. Woche 03 = 3 Uhr
*/
#define Zeitzone "CET-1CEST,M3.5.0/02,M10.5.0/03"

// time_t enthält die Anzahl der Sekunden seit dem 1.1.1970 0 Uhr
time_t aktuelleZeit;

/* 
  Struktur tm
  tm_hour -> Stunde: 0 bis 23
  tm_min -> Minuten: 0 bis 59
  tm_sec -> Sekunden 0 bis 59
  tm_mday -> Tag 1 bis 31
  tm_mon -> Monat: 0 (Januar) bis 11 (Dezember)
  tm_year -> Jahre seit 1900
  tm_yday -> vergangene Tage seit 1. Januar des Jahres
  tm_isdst -> Wert > 0 = SoMinutenerzeit (dst = daylight saving time)
*/
tm Zeit;

void setup() 
{
    // Sensor starten
  dht.begin();

  Serial.begin(9600);

  // Zeitzone: Parameter für die zu ermittelnde Zeit
  configTzTime(Zeitzone, Zeitserver);

  // WiFi starten
  WiFi.mode(WIFI_STA);
  WiFi.begin(Router, Passwort);

  Serial.println("------------------------");

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(200);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("Verbunden mit ");
  Serial.println(Router);
  Serial.print("IP über DHCP: ");
  Serial.println(WiFi.localIP());

  // Zeit holen
  time(&aktuelleZeit);

  // localtime_r -> Zeit in die lokale Zeitzone setzen
  localtime_r(&aktuelleZeit, &Zeit);

  // Zeit in Stunden, Minuten und Sekunden
  Stunden = Zeit.tm_hour, Minuten = Zeit.tm_min, Sekunden = Zeit.tm_sec;
  
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(Kreisfarbe);

  // 4 Pixel breiter äußerer Rand, Farbe au der Farbpalette wählen
  tft.drawCircle(MitteHoehe, MitteBreite, Radius - 1, Randfarbe);
  tft.drawCircle(MitteHoehe, MitteBreite, Radius - 2, Randfarbe);
  tft.drawCircle(MitteHoehe, MitteBreite, Radius - 3, Randfarbe);
  tft.drawCircle(MitteHoehe, MitteBreite, Radius - 4, Randfarbe);

  // innere Fläche bis auf den Rand von 4 Pixeln vollständig löschen
  // wenn andere Farbe als äußerer Rand gewählt wird ergibt sich ein schmaler Rand
  tft.fillCircle(MitteHoehe, MitteBreite, Radius - 4, Kreisfarbe);

  /*
    alle 30° Linie am Rand als Stundenmarkierung zeichnen
    DEG_TO_RAD (= PI/180 = 0.0174532925) -> Winkel in Bogenmaß umrechnen
    sin/cos berechnen die x-/y-Kordinaten des Punktes auf der Kreislinie
  */
  for (int i = 0; i < 360; i += 30) 
  {
    PosX = cos((i - 90) * DEG_TO_RAD);
    PosY = sin((i - 90) * DEG_TO_RAD);

    // kurze Linien zeichnen, von 114 bis 100 vom äußeren Rand aus
    // Farbe individuell wählbar
    int PunktX1 = PosX * 110 + Radius;
    int PunktY1 = PosY * 110 + Radius;
    int PunktX2 = PosX * 100 + Radius;
    int PunktY2 = PosY * 100 + Radius;

    tft.drawLine(PunktX1, PunktY1, PunktX2, PunktY2, Zeigerfarbe);

    // keine Striche an der Position der Zahlen
    if (Ziffernanzeigen)
    {
      if (PunktX1 == 10 || PunktX1 == 120 || PunktX1 == 230)
      {
        tft.drawLine(PunktX1, PunktY1, PunktX2, PunktY2, Kreisfarbe);
      }
    }
  }

  // alle 6 Grad Punkte als Minutenmarkierung zeichnen
  for (int i = 0; i < 360; i += 6) 
  {
    PosX = cos((i - 90) * DEG_TO_RAD);
    PosY = sin((i - 90) * DEG_TO_RAD);

    // Positionen der Punkte
    // 108 -> Abstand vom Mittelpunkt
    PunktX = PosX * 108 + Radius;
    PunktY = PosY * 108 + Radius;
    tft.drawPixel(PunktX, PunktY, Zeigerfarbe);  
  }

  // Markierung 12 3 6 9
  if (Ziffernanzeigen)
  {
    tft.setTextSize(2);
    tft.setTextColor(Zeigerfarbe);
    tft.setCursor(105, 16);
    tft.print("12");

    tft.setCursor(10, 127);
    tft.print("9");

    tft.setCursor(220, 130);
    tft.print("3");
  
    tft.setCursor(113, 220);
    tft.print("6");
  }

  if (DatumAnzeigen)
  {
    ZeigeDatum();
  }
  TemperaturAnzeigen();
}

void loop() 
{
  // Sekunden weiter zählen
  // wenn millis() ohne Rest durch 1000 teilbar ist
  if (millis() %1000 == 0)
  {
    Sekunden++;  

    if (Sekunden == 60) 
    { 
      Sekunden = 0; 
      Minuten++; 

      // Zeit jede Sekunde mit Zeitserver synchronisieren
      // aktuelle Zeit holen
      time(&aktuelleZeit);

      // localtime_r -> Zeit in die lokale Zeitzone setzen
      localtime_r(&aktuelleZeit, &Zeit);

      // Zeit in Stunden, Minuten und Sekunden
      Stunden = int(Zeit.tm_hour), Minuten = int(Zeit.tm_min), Sekunden = int(Zeit.tm_sec);
      TemperaturAnzeigen();

      if (Minuten > 59) 
      {
        Minuten = 0;
        Stunden++;  
        if (Stunden > 11) 
        {
          Stunden = 0;
          
          // Datum anzeigen/nicht anzeigen
          if (DatumAnzeigen) ZeigeDatum();
        }
      }
    }

    // Minuten+Sekunden in Relation zu 3600 setzen
    // 60 / 3600 = 0.01666667         
    GradMinuten = Minuten * 6 + GradSekunden * 0.01666667; 

    // 30 / 3600 = 0.0833333
    // Stunden+Minuten in Relation zu 360 setzen
    // 0-11 -> 0-360 
    GradStunden = Stunden * 30 + GradMinuten * 0.0833333;  // 0-11 -> 0-360 
    StundePosX = cos((GradStunden - 90) * DEG_TO_RAD);
    StundePosY = sin((GradStunden - 90) * DEG_TO_RAD);

    MinutePosX = cos((GradMinuten - 90) * DEG_TO_RAD) ;
    MinutePosY = sin((GradMinuten - 90) * DEG_TO_RAD);

    // nach jeder Minute Minuten-/Stundenzeiger löschen
    // oder einmalig beim Start der Anzeige
    if (Sekunden == 0 || Start) 
    {
      // Datum erneuern wenn es korrekt ist (!= 1970)
      if (Zeit.tm_year + 1900 != 1970 && DatumAnzeigen)
      {
        ZeigeDatum();
      }
      
      Start = false;
      tft.drawLine(StundenZeigerX, StundenZeigerY, MitteHoehe, MitteHoehe + 1, Kreisfarbe);  

      // 62 Pixel -> Länge des Stundenzeigers
      // Mittelpunkt + 1 -> Mittelpunkt soll nicht gelöscht werden
      StundenZeigerX = StundePosX * 62 + MitteHoehe + 1;
      StundenZeigerY = StundePosY * 62 + MitteHoehe + 1;
      tft.drawLine(MinutenZeigerX, MinutenZeigerY, MitteHoehe, MitteHoehe + 1, Kreisfarbe);

      // 84 Pixel -> Länge des Minutenzeigers
      // Mittelpunkt + 1 -> Mittelpunkt soll nicht gelöscht werden
      MinutenZeigerX = MinutePosX * 84 + MitteHoehe;
      MinutenZeigerY = MinutePosY * 84 + MitteHoehe + 1;
    }
    
    // Minuten
    tft.drawLine(MinutenZeigerX, MinutenZeigerY, MitteHoehe, MitteHoehe + 1, Zeigerfarbe);

    // Stunden
    tft.drawLine(StundenZeigerX, StundenZeigerY, MitteHoehe, MitteHoehe + 1, Zeigerfarbe);

    // Mittelpunkt zeichnen
    tft.fillCircle(MitteHoehe, MitteHoehe + 1, 3, Zeigerfarbe); 
  }
}

void ZeigeDatum()
{
  tft.setTextSize(2);
  tft.setCursor(65, 150);
  tft.setTextColor(GRUEN);
      
  // Bildschirmbereich für das Datum löschen
  tft.fillRect(60, 150, 125, 25, Kreisfarbe);
  if (Zeit.tm_mday < 10) tft.print("0");
  tft.print(Zeit.tm_mday);
  tft.print(".");

  // Monat: führende 0 ergänzen
  if (Zeit.tm_mon < 9) tft.print("0");
    
  // Zählung beginnt mit 0 -> +1
  tft.print(Zeit.tm_mon + 1);
  tft.print(".");

  // Anzahl Jahre seit 1900
  tft.print(Zeit.tm_year + 1900);

}

void TemperaturAnzeigen()
{
   // Temperatur lesen 
  String Temperatur = String(dht.readTemperature());

  // replace -> . durch , ersetzen
  Temperatur.replace(".", ",");

  // Luftfeuchtigkeit lesen 
  String Luftfeuchtigkeit = String(dht.readHumidity());

  // replace -> . durch , ersetzen
  Luftfeuchtigkeit.replace(".", ",");

  tft.fillRect(230, 185, 125, 50, Kreisfarbe);
  tft.setTextSize(2);
  tft.setCursor(230, 190);
  tft.setTextColor(BLAU);
  tft.print(Luftfeuchtigkeit + " %");

  tft.setCursor(230, 210);
  tft.setTextColor(ROT);
  tft.println(Temperatur + " C");

  // Gradzeichen als Kreis
  tft.drawCircle(296, 213, 3, ROT);
}
