#define _DEBUG_

#include "ESP8266WiFi.h"
#include <ESP8266HTTPClient.h>
#include "EasyNextionLibrary.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <myConfig.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "time.h"
#include "TimeLib.h"


#define MQTT_HOST IPAddress(MQTT_HOST_1, MQTT_HOST_2, MQTT_HOST_3, MQTT_HOST_4)
#define STR_BUFF_SIZE 7

EasyNex myNex(Serial);

// Web client
HTTPClient HTTPclient;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// OpenWeatherMap Data
const char* hostOpenWeatherMap = "http://api.openweathermap.org/data/2.5/weather";
const char* cityID = "2509402"; 

// timer
unsigned long lastQuery = 0;
unsigned long queryTime = 5000; // Time in miliseconds

// Define NTP Client to get time
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const long utcOffsetInSeconds = 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//MQTT
const char* topicInt = "esp8266_A/bme280/values";
const char* topicExt = "esp8266_B/ds18b20/values";

char tempExt[STR_BUFF_SIZE];
char tempInt[STR_BUFF_SIZE];
char humExt[STR_BUFF_SIZE];
char humInt[STR_BUFF_SIZE];

void configWifi() {
#ifdef _DEBUG_
  Serial.print("Connecting with ");
  Serial.println(WIFI_SSID);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef _DEBUG_
    Serial.print(".");
#endif
  }

#ifdef _DEBUG_
  Serial.println("");
  Serial.println("Connecting to WiFi");
#endif
}

void run() {
  char url[200];
  strcpy(url, hostOpenWeatherMap);
  strcat(url, "?id=");
  strcat(url, cityID);
  strcat(url, "&appid="); 
  strcat(url, OpenWeatherAPIKEY); 

#ifdef _DEBUG_
  Serial.print("HTTP request: ");
  Serial.println(url);
#endif

  // Server conexion & request config
  HTTPclient.begin(url);

  // Send request
  int HTTPcode = HTTPclient.GET();

#ifdef _DEBUG_
  Serial.print("HTTP code: ");
  Serial.println(HTTPcode);
#endif

  if (HTTPcode > 0) {
    if (HTTPcode == HTTP_CODE_OK) {
#ifdef _DEBUG_
      Serial.print("JSON file: ");
      Serial.println(HTTPclient.getString());
#endif

      // Visit https://arduinojson.org/v6/assistant/
      const size_t capacity = JSON_ARRAY_SIZE(3) +
                              2 * JSON_OBJECT_SIZE(1) +
                              JSON_OBJECT_SIZE(2) +
                              3 * JSON_OBJECT_SIZE(4) +
                              JSON_OBJECT_SIZE(5) +
                              JSON_OBJECT_SIZE(6) +
                              JSON_OBJECT_SIZE(12) +
                              304;
      DynamicJsonDocument doc(capacity);

      // JSON Object Parsing
      DeserializationError error = deserializeJson(doc, HTTPclient.getString());
      if (error) {
#ifdef _DEBUG_
        Serial.print("Error parsing JSON. Error: ");
        Serial.println(error.c_str());
#endif
        return;
      }

      // Temperature
      float tempF = doc["main"]["temp"];
      tempF = tempF - 273.15; // to Celsius
      char temperature[STR_BUFF_SIZE];
      snprintf(temperature, STR_BUFF_SIZE, "%.0f C", tempF);

      // humidity
      char humidity[7];
      snprintf(humidity, STR_BUFF_SIZE, "%d%%", (int)doc["main"]["humidity"]) ;

      // Temperature min
      float tempMinF = doc["main"]["temp_min"];
      tempMinF = tempMinF - 273.15; // to Celsius
      char tempMin[7];
      snprintf(tempMin, STR_BUFF_SIZE, "%.0f C", tempMinF);

      // Temperature max
      float tempMaxF = doc["main"]["temp_max"];
      tempMaxF = tempMaxF - 273.15; // to Celsius
      char tempMax[7];
      snprintf(tempMax, STR_BUFF_SIZE, "%.0f C", tempMaxF);

      // windF
      char wind[7];
      snprintf(wind, STR_BUFF_SIZE, "%.0fm/s", (float)doc["wind"]["speed"]);

      // City
      const char* city = doc["name"];
 //     nexCity.setText(city);

      unsigned long epochTime = timeClient.getEpochTime();
      char datetime[100];
      sprintf(datetime, "%s %02d:%02d %d/%d/%d", daysOfTheWeek[timeClient.getDay()],
                                                 hour(epochTime),
                                                 minute(epochTime),
                                                 day(epochTime),
                                                 month(epochTime),
                                                 year(epochTime));


      // weather main
      const char* weather_main = doc["weather"][0]["main"];

      // Update display
     myNex.writeStr("page page0");
     myNex.writeStr("t5.txt", temperature); 
     myNex.writeStr("t1.txt", humidity);  
     myNex.writeStr("t3.txt", tempMin);
     myNex.writeStr("t4.txt", tempMax);
     myNex.writeStr("t2.txt", wind);
     myNex.writeStr("t8.txt", tempInt); 
     myNex.writeStr("t11.txt", tempExt); 
     myNex.writeStr("t10.txt", weather_main); 
     myNex.writeStr("t9.txt", datetime);
     
      // weather indicator icon 
      const char* icon = doc["weather"][0]["icon"];


      // Icon  assingment 
      if (strcmp(icon, "01d") == 0){
        myNex.writeNum("q0.picc", 1);
      } else if (strcmp(icon, "01n") == 0) {
        myNex.writeNum("q0.picc", 10);
      } else if (strcmp(icon, "02d") == 0) {
        myNex.writeNum("q0.picc", 2);
      } else if (strcmp(icon, "02n") == 0) {
        myNex.writeNum("q0.picc", 11);
      } else if (strcmp(icon, "03d") == 0 || strcmp(icon, "03n") == 0) {
        myNex.writeNum("q0.picc", 3);
      } else if (strcmp(icon, "04d") == 0 || strcmp(icon, "04n") == 0) {
        myNex.writeNum("q0.picc", 4);
      } else if (strcmp(icon, "09d") == 0 || strcmp(icon, "09n") == 0) {
        myNex.writeNum("q0.picc", 9);
      } else if (strcmp(icon, "10d") == 0) {
        myNex.writeNum("q0.picc", 6);
      } else if (strcmp(icon, "10n") == 0) {
        myNex.writeNum("q0.picc", 12);   
      } else if (strcmp(icon, "11d") == 0 || strcmp(icon, "11n") == 0) {
        myNex.writeNum("q0.picc", 7);
      } else if (strcmp(icon, "13d") == 0 || strcmp(icon, "13n") == 0) {
        myNex.writeNum("q0.picc", 8);
      } else if (strcmp(icon, "50d") == 0 || strcmp(icon, "50n") == 0) {
        myNex.writeNum("q0.picc", 9);
      } else {
        myNex.writeNum("q0.picc", 1);
      }

#ifdef _DEBUG_
      Serial.println("OpenWeatherMap Data");
      Serial.print("Temperature: ");
      Serial.println(temperature);
      Serial.print("humidity: ");
      Serial.println(humidity);
      Serial.print("Temp. Min: ");
      Serial.println(tempMin);
      Serial.print("Temp. Max: ");
      Serial.println(tempMax);
      Serial.print("wind: ");
      Serial.println(wind);
      Serial.print("icon: ");
      Serial.println(icon);
      Serial.print("city: ");
      Serial.println(city);
#endif
    } else {
#ifdef _DEBUG_
      Serial.println("Error receiving request.");
#endif
    }
  }
}

void timer() {
  if (millis() < lastQuery) {
    lastQuery = millis();
  }

  if ((millis() - lastQuery) > queryTime) {
    lastQuery = millis();
    run();
   }
}

void readJSON(byte* payload, char *temp, char *hum) {
  StaticJsonDocument<170> doc; 

  char *payload_str = reinterpret_cast<char*>(payload);
  Serial.print("readJSON payload_str: ");
  Serial.println(payload_str);
  
  DeserializationError error = deserializeJson(doc, payload_str);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  snprintf(temp, STR_BUFF_SIZE , "%d C", (int) doc["data"][0]["temperature"]);
  snprintf(hum, STR_BUFF_SIZE , "%d %", (int) doc["data"][0]["humidity"]);
  //snprintf(pres, STR_BUFF_SIZE , "%d", ret[2]);

  Serial.println(temp);
  Serial.println(hum);
  //Serial.println(pres);

  }

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message recived [");
  Serial.print(topic);
  Serial.println("]");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, topicInt) == 0) {
    memset(tempInt, 0, STR_BUFF_SIZE);
    memset(humInt, 0, STR_BUFF_SIZE);
    readJSON(payload, tempInt, humInt);
    
  } else if (strcmp(topic, topicExt) == 0){
    memset(tempExt, 0, STR_BUFF_SIZE);
    memset(humExt, 0, STR_BUFF_SIZE);
    readJSON(payload, tempExt, humExt);
  }
  
}

void reconnect() {
   // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("NextionClient", MQTT_USER, MQTT_PASS)) {
      Serial.println("Succesfully connected to MQTT Broker");
      // Subscribe
      mqttClient.subscribe(topicInt);
      mqttClient.subscribe(topicExt);

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(){
  // Initiate communication with display
  // 9600 Default Baudrate
  Serial.begin(9600);
  myNex.begin(9600); 
  configWifi();
  timeClient.begin();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  myNex.NextionListen();
  timeClient.update();
  timer();
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  timeClient.update();
}
