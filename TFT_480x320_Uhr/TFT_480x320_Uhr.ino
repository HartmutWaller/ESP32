 
  #include "WiFi.h"

#include "time.h"

#include "Adafruit_ST7796S_kbv.h"

// Adafruit-Schriftart einbinden
#include "Fonts/FreeSans12pt7b.h"

// XIAO
// #define TFT_CS D7
// #define TFT_RST D1
// #define TFT_DC D2

// Arduino Nano ESP 32
// #define TFT_CS       10
// #define TFT_RST       9
// #define TFT_DC        8

// ESP32-C6
// #define TFT_CS       18
// #define TFT_RST       3
// #define TFT_DC        2

// ESP32-WROOM
// #define TFT_CS        5
// #define TFT_RST       4
// #define TFT_DC        2

Adafruit_ST7796S_kbv tft = Adafruit_ST7796S_kbv(TFT_CS, TFT_DC, TFT_RST);

// WiFi-Daten
char Router[] = "Router_SSID";
char Passwort[] = "xxxxxxxx";

// Variablen des TFTs (Höhe, Breite, Radius)
const int MitteHoehe = 160;
const int MitteBreite = 160;
const int Radius = 160;

// Multiplikatoren für x- y-Positionen der Stunden, Minuten und Sekunden
float SekundePosX = 0, SekundePosY = 0, MinutePosX = 0, MinutePosY = 0, StundePosX = 0, StundePosY = 0;
float GradSekunden = 0, GradMinuten = 0, GradStunden = 0;

// x- y-Koordinaten für die Anzeige Stunden, Minuten und Sekunden
int SekundenZeigerX = MitteHoehe, SekundenZeigerY = MitteHoehe;
int MinutenZeigerX = MitteHoehe, MinutenZeigerY = MitteHoehe;
int StundenZeigerX = MitteHoehe, StundenZeigerY = MitteHoehe; 

// Start wird nur beim ersten Start für den Aufbau des TFTs benötigt
bool Start = true;

// Variable für die Kontrolle des Zeittaktes von 1 Sekunde
int ZeitTakt = 0;

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

// true -> Sekundenzeiger nur als Kreis
bool SekundenzeigerKreis = true;

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
  Serial.begin(9600);

  // Zeitzone: Parameter für die zu ermittelnde Zeit
  configTzTime(Zeitzone, Zeitserver);

  // WiFi starten
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
  tft.invertDisplay(1);
  tft.setRotation(2);
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
    int PunktX1 = PosX * 155 + Radius;
    int PunktY1 = PosY * 155 + Radius;
    int PunktX2 = PosX * 145 + Radius;
    int PunktY2 = PosY * 145 + Radius;
    tft.drawLine(PunktX1, PunktY1, PunktX2, PunktY2, Zeigerfarbe);
    
    // keine Striche an der Position der Zahlen
    if (Ziffernanzeigen)
    {

      if (PunktX1 == 160 || PunktX1 == 315 || PunktX1 == 5)
      {
        tft.drawLine(PunktX1, PunktY1, PunktX2, PunktY2, Kreisfarbe);
      }  
    }  
  }

  // alle 6 Grad Punkte als Sekundenmarkierung zeichnen
  for (int i = 0; i < 360; i += 6) 
  {
    PosX = cos((i - 90) * DEG_TO_RAD);
    PosY = sin((i - 90) * DEG_TO_RAD);

    // Positionen der Punkte
    // 108 -> Abstand vom Mittelpunkt
    PunktX = PosX * 150 + Radius;
    PunktY = PosY * 150 + Radius;
    tft.drawPixel(PunktX, PunktY, Zeigerfarbe);  
  }

  // Markierung 12 3 6 9
  if (Ziffernanzeigen)
  {
    tft.setFont(&FreeSans12pt7b);
    tft.setTextColor(Zeigerfarbe);
    tft.setCursor(145, 25);
    tft.print("12");
    
    tft.setCursor(10, 170);
    tft.print("9");
    
    tft.setCursor(300, 170);
    tft.print("3");
  
    tft.setCursor(155, 310);
    tft.print("6");
  }

  if (DatumAnzeigen)
  {
    ZeigeDatum();
  }
}

void loop() 
{
  // Zeit weiter zählen
  if (ZeitTakt < millis()) 
  {
    ZeitTakt += 1000;
    Sekunden++;  
  
    if (Sekunden == 60) 
    { 
      Sekunden = 0; 
      Minuten++; 

      // Zeit jede Minute mit Zeitserver synchronisieren
      // aktuelle Zeit holen
      time(&aktuelleZeit);

      // localtime_r -> Zeit in die lokale Zeitzone setzen
      localtime_r(&aktuelleZeit, &Zeit);

      // Zeit in Stunden, Minuten und Sekunden
      Stunden = int(Zeit.tm_hour), Minuten = int(Zeit.tm_min), Sekunden = int(Zeit.tm_sec);

      if (Minuten > 59) 
      {
        Minuten = 0;
        Stunden++;  
        if (Stunden > 11) 
        {
          Stunden = 0;
          
          // Datum nach 24 Stunden erneut abfragen
          if (DatumAnzeigen && Stunden > 23)
          {
            ZeigeDatum();
          }
        }
      }
    }

    // Datum anzeigen/nicht anzeigen
    // Datumsanzeige nur erneuern, wenn der Sekundenzeiger
    // vollständig als Strich und Kreis angezeigt wird
    if (DatumAnzeigen && !SekundenzeigerKreis)
    {
      // Anzeige des Datums nur aktualisieren,
      // wenn sich der Sekundenzeiger darüber befindet
      if (Sekunden > 20 & Sekunden < 40)
      {
        ZeigeDatum();
      }
    }

    // Vorausberechnung der x- und y-Koordinaten
    // alle 6° eine Sekunde vorwärts
    GradSekunden = Sekunden * 6;    

    // alle 6° eine Minute vorwärts     
    GradMinuten = Minuten * 6; 

    // alle 30° eine Stunde vorwärts
    // 30 / 3600 = 0.0833333
    // sorgt dafür, dass der Stundenzeiger entsprechend 
    // der Anzahl der Minuten weiter "wandert"
    GradStunden = Stunden * 30 + GradMinuten * 0.0833333; 
    StundePosX = cos((GradStunden - 90) * DEG_TO_RAD);
    StundePosY = sin((GradStunden - 90) * DEG_TO_RAD);

    MinutePosX = cos((GradMinuten - 90) * DEG_TO_RAD) ;
    MinutePosY = sin((GradMinuten - 90) * DEG_TO_RAD);

    SekundePosX = cos((GradSekunden - 90) * DEG_TO_RAD);
    SekundePosY = sin((GradSekunden - 90) * DEG_TO_RAD);

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
      StundenZeigerX = StundePosX * 85 + MitteHoehe + 1;
      StundenZeigerY = StundePosY * 85 + MitteHoehe + 1;
      tft.drawLine(MinutenZeigerX, MinutenZeigerY, MitteHoehe, MitteHoehe + 1, Kreisfarbe);

      // 84 Pixel -> Länge des Minutenzeigers
      // Mittelpunkt + 1 -> Mittelpunkt soll nicht gelöscht werden
      MinutenZeigerX = MinutePosX * 120 + MitteHoehe;
      MinutenZeigerY = MinutePosY * 120 + MitteHoehe + 1;
    }

    // Sekundenzeiger löschen
    if (!SekundenzeigerKreis) tft.drawLine(SekundenZeigerX, SekundenZeigerY, MitteHoehe, MitteHoehe + 1, Kreisfarbe);

    // Kreis am Sekundenzeiger löschen, Radius 5
    tft.fillCircle(SekundenZeigerX, SekundenZeigerY, 5, Kreisfarbe);

    // 85 Pixel -> Länge des Sekundenzeigers
    SekundenZeigerX = SekundePosX * 120 + MitteHoehe + 1;
    SekundenZeigerY = SekundePosY * 120 + MitteHoehe + 1;

    // Zeiger neu zeichnen
    // Sekunden Linie nur anzeigen wenn SekundenzeigerKreis false
    if (!SekundenzeigerKreis) tft.drawLine(SekundenZeigerX, SekundenZeigerY, MitteHoehe, MitteHoehe + 1, ROT);
 
    // Minuten
    tft.drawLine(MinutenZeigerX, MinutenZeigerY, MitteHoehe, MitteHoehe + 1, Zeigerfarbe);

    // Stunden
    tft.drawLine(StundenZeigerX, StundenZeigerY, MitteHoehe, MitteHoehe + 1, Zeigerfarbe);

    // Kreis an der Spitze des Sekundenzeigers, Radius 5
    tft.fillCircle(SekundenZeigerX, SekundenZeigerY, 5, ROT);

    // Mittelpunkt zeichnen
    tft.fillCircle(MitteHoehe, MitteHoehe + 1, 3, Zeigerfarbe); 
  }
}

void ZeigeDatum()
{
  tft.setFont(&FreeSans12pt7b);  
  tft.setCursor(110, 240);
  tft.setTextColor(GRUEN);
      
  // Bildschirmbereich für das Datum löschen
  tft.fillRect(110, 220, 125, 25, Kreisfarbe);
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
