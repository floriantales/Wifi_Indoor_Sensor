// Used Libraries :
// DHT Sensor Library by Adafruit (+ install Adafruit Unified Sensor Library)
// NTPClient by Fabrice Weinberg -> https://projetsdiy.fr/esp8266-web-serveur-partie3-heure-internet-ntp-ntpclientlib/#Comparaison_des_librairies
// Send udp packet to influxbd : https://www.influxdata.com/blog/how-to-send-sensor-data-to-influxdb-from-an-arduino-uno/

// Les includes pour le DHT22
#include "DHT.h"
#define DHTPIN 2        // what digital pin where DHT22 is connected to - GPIO 2 = wemos D4 pin 
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

// Les includes pour le WIFI
#include <ESP8266WiFi.h>
#define NB_TRYWIFI  15    // Nbr de tentatives de connexion au réseau WiFi
const char* ssid = "freebox";
const char* password = "maison de clairon et tata";
WiFiClient espClient;

// Les includes pour le NTP
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
const char ntp_server[] = "europe.pool.ntp.org";  // Le serveru NTP source
NTPClient timeClient(ntpUDP, ntp_server);

// InfluxDB params
const char host[]="192.168.0.11";
const int port = 8089;
const char influxdb_table_name[] = "dht22"; // Le nom de la table influxd
const char influxdb_tag_sensor_id[] = "wemos2"; // Le nom du device dans le tag "sensor_id" de table "DHT22"
WiFiUDP influxdbUDP;

// Gestion du DEBUG
#define DEBUG // Comment to disable debugging mode
#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTNUM(x,y)     Serial.print (x,y)
 #define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTNUM(x,y)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x) 
#endif

void setup() {
  Serial.begin(9600);
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("##############################");
  DEBUG_PRINTLN("Startup Wireless Indoor Sensor");
  DEBUG_PRINTLN("##############################");
  DEBUG_PRINT("Startup reason: ");DEBUG_PRINTLN(ESP.getResetReason());

  //########################
  // Connexion Wifi
  //########################
  DEBUG_PRINTLN("Connecting to WiFi");
  int _try = 0;
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("..");
    delay(500);
    _try++;
    if ( _try >= NB_TRYWIFI ) {
        DEBUG_PRINTLN("Impossible to connect WiFi network, go to deep sleep");
        ESP.deepSleep(10e6);
    }
  }
  DEBUG_PRINTLN("Connected to the WiFi network!");
  DEBUG_PRINT("IP Address: ");DEBUG_PRINTLN(WiFi.localIP());
  DEBUG_PRINT("signal strength (RSSI):");DEBUG_PRINT(WiFi.RSSI());DEBUG_PRINTLN(" dBm");

  //########################
  // Capture DHT22
  //########################
  dht.begin();
  DEBUG_PRINTLN("Reading DHT22 sensor :");
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default) - dht.readTemperature(true) (isFahrenheit = true) as Fahrenheit
  float temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    DEBUG_PRINT("Failed to read from DHT sensor!");
    return;
  }
  // Compute heat index in Celsius (isFahreheit = false)
  float heatindex = dht.computeHeatIndex(temperature, humidity, false);

  DEBUG_PRINT("Humidity: ");
  DEBUG_PRINTNUM(humidity,1);
  DEBUG_PRINT("%  Temperature: ");
  DEBUG_PRINTNUM(temperature,1);
  DEBUG_PRINT(" deg C  Heat index: ");
  DEBUG_PRINTNUM(heatindex,1);
  DEBUG_PRINTLN(" deg C ");

  //########################
  // Recuperation NTP
  //########################
  DEBUG_PRINTLN("Start NTP Client");
  timeClient.begin();
  DEBUG_PRINT("Reading NTP Timestamp from ");DEBUG_PRINTLN(ntp_server);
  timeClient.update();
  DEBUG_PRINT("Formatted Time = ");DEBUG_PRINTLN(timeClient.getFormattedTime());
  DEBUG_PRINT("Epoch Time = ");DEBUG_PRINTLN(timeClient.getEpochTime());

  //########################
  // Envoi vers Infuxdb
  //########################
  DEBUG_PRINT("Sending UDP packets to Influxdb Server ");DEBUG_PRINT(host);DEBUG_PRINT(":");DEBUG_PRINT(port);DEBUG_PRINTLN(" :");
  // concatenate data into the line protocol -> https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference#sidebar
  String data_udp = String(String(influxdb_table_name) + ",sensor_id=" + String(influxdb_tag_sensor_id) + " temperature=" + String(temperature) + ",heatindex=" + String(heatindex) + ",humidity=" + String(humidity) + " " + timeClient.getEpochTime() + "000000000");
  DEBUG_PRINT("Sending data : ");DEBUG_PRINTLN(data_udp);
  // send the packet
  influxdbUDP.beginPacket(host, port);
  influxdbUDP.print(data_udp);
  influxdbUDP.endPacket();
  //yield(); // ça marche pas : https://www.tabsoverspaces.com/233359-udp-packets-not-sent-from-esp-8266-solved
  delay(500); // ça ça marche, sans délais ça marche pas

  //########################
  // Deep Sleep
  //########################
  DEBUG_PRINTLN("Go to Deep Sleep mode");
  ESP.deepSleep(55e6); //55 secondes
}

void loop() {
}
