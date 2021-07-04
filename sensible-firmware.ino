 // boards manager: install AsyncElegantOTA. Download these two additional libraries
// https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip
// https://github.com/me-no-dev/AsyncTCP/archive/master.zip
// don't forget ESP32 wifi: cannot use pins 0,2,4,12-15,25-27!

/*
Documentation
-----------------------

To do
- Implement MQTT for decay and repeat times
- Ability to turn off interim messages by setting ar to 0

Web Serial
---
Go to http://ipaddress/webserial to see serial updates.

Tele messages (MQTT)
---
Every 60 seconds
fw = firmware version
at = audio threshold setting
ad = audio decay setting
ar = audio repeat time (for interim audio messages)

Calibration mode:
---
Send an MQTT message to the calibration topic containing a non-zero number. The sketch will
take max and min readings of audio level over the number of seconds specified and return this
info over MQTT to the root topic.

*/


#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebSerial.h>
#include "time.h"


#define CLIENTID "ESPClient-"
#define mqttroot "sensors/multisensor-002"
#define mqttcommand "sensors/multisensor-002/cmnd/#" // we subscribe to this
#define mqttthresholdset "sensors/multisensor-002/cmnd/audiothreshold"
#define mqttcalibrate "sensors/multisensor-002/cmnd/calibrate" // 0 = off, anything else = on
#define mqttdecay "sensors/multisensor-002/cmnd/decay"
#define mqttrepeat "sensors/multisensor-002/cmnd/repeat"
#define audiopin 34 // This pin is in the ADC channel 1. Pins in ADC channel 2 don't work with wifi
#define firmware "0.9"


const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "MQTT_SERVER_IP";

// timers
unsigned long calibrationTimer;  // millis timer for calibration
int calibrationTime = 5; // duration of calibration in secs. Overwritten if configured over MQTT
unsigned long audioDelayTimer;
int decay = 2000; // decay time for audio
int repeatTime = 2;
unsigned long repeatTimer;
unsigned long teleTimer;

// variables
int audio = 0;
int audiothreshold = 1600; // will overwrite from settings over MQTT - 1600 works for SMD diode
bool audiostate = false;
bool prevaudiostate = false;
bool sendCalibration = false;
int calibrationmaxreading;
int calibrationminreading;
int repeatmaxreading;

// vars for timekeeping
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// general buffer for json / MQTT
char jsonbuff[192]; // buffer to store JSON doc

WiFiClient espClient;
PubSubClient client(espClient);

AsyncWebServer server(80);

// Web serial receive message
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}


void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    WebSerial.println("Failed to obtain time");
    return;
  }
  char buff[23];
  strftime (buff,23,"[%F %H:%M:%S] ",&timeinfo);
  WebSerial.print(buff);
  Serial.print(buff);
}

// MQTT receive message
void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic, mqttthresholdset) == 0) {
    payload[length] = '\0'; // null terminate it
    int value = atoi((char *)payload);     // store payload in temp var
    if (value > 0) audiothreshold = value; // check it was indeed a number
    Serial.print("Audio threshold: "); Serial.println(value);
    WebSerial.print("Audio threshold: "); WebSerial.println(value);
  } else if (strcmp(topic, mqttcalibrate) == 0) {
    // calibration messages should contain a number, which corresponds to
    // the number of seconds you'd like to calibrate for
    payload[length] = '\0'; // null terminate it
    int value = atoi((char *)payload);     // store payload in temp var
    if (value > 0) {
      calibrationTime = value;
      calibrationTimer = millis();
      sendCalibration = true;
      calibrationmaxreading = 0;
      calibrationminreading = 2000;
      printLocalTime();
      Serial.print("Calibration request: "); Serial.println(calibrationTime);
      WebSerial.print("Calibration request: "); WebSerial.println(calibrationTime);
    }
  } else if (strcmp(topic, mqttdecay) == 0) {
    payload[length] = '\0'; // null terminate it
    int value = atoi((char *)payload);     // store payload in temp var
    if (value > 0) {
      decay = value;
    }
  } else if (strcmp(topic, mqttrepeat) == 0) {
    payload[length] = '\0'; // null terminate it
    int value = atoi((char *)payload);     // store payload in temp var
    if (value > 0) {
      repeatTime = value;
    }
  }
//  StaticJsonDocument<256> doc;
//  deserializeJson(doc, payload, length);

  // React to JSON messages
  //  if (doc.containsKey("light")) {
  //    lightingtype = doc["light"];
  //    solidcolour[0] = doc["rgbw"][0];
  //    solidcolour[1] = doc["rgbw"][1];
  //    solidcolour[2] = doc["rgbw"][2];
  //    solidcolour[3] = doc["rgbw"][3];
  //  }
}

// MQTT reconnect
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = CLIENTID;
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...

      const int capacity = JSON_OBJECT_SIZE(3);
      StaticJsonDocument<capacity> doc;
      doc["MAC"] = "hello";
      doc["lat"] = 48.748010;
      doc["lon"] = 2.293491;
      serializeJson(doc, jsonbuff);
      client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json
      client.subscribe(mqttcommand);
    } else {

      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Wifi setup
void setup_wifi() {
    delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/********************************************
 *                                          *
 *     MAIN SETUP                           *
 *                                          *
 *                                          *
 ********************************************/
void setup(void) {
  Serial.begin(115200);
  setup_wifi();

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Get time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Multisensor");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  byte mac[6];                     // the MAC address of your Wifi shield
  WiFi.macAddress(mac);Serial.print("MAC: ");Serial.print(mac[5],HEX);Serial.print(":");Serial.print(mac[4],HEX);Serial.print(":");Serial.print(mac[3],HEX);Serial.print(":");Serial.print(mac[2],HEX);Serial.print(":");Serial.print(mac[1],HEX);Serial.print(":");Serial.println(mac[0],HEX);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);

}



/********************************************
 *                                          *
 *     MAIN LOOP                            *
 *                                          *
 *                                          *
 ********************************************/
 
void loop(void) {

  if (millis() - calibrationTimer < calibrationTime * 1000) {
    audio = analogRead(audiopin);
    if (audio > calibrationmaxreading) calibrationmaxreading = audio;
    if (audio < calibrationminreading) calibrationminreading = audio;     
  } else {
    // we are NOT in calibration mode
    // BUT, if flag set to send the results of calibration, then do that
    if (sendCalibration) {
      const int capacity = JSON_OBJECT_SIZE(3);
      StaticJsonDocument<capacity> doc;
      doc["result"] = "calibration";
      doc["max"] = calibrationmaxreading;
      doc["min"] = calibrationminreading;
      serializeJson(doc, jsonbuff);
      client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json
      printLocalTime();
      Serial.print("Calibration Result: Max="); Serial.print(calibrationmaxreading); Serial.print(" Min="); Serial.println(calibrationminreading);
      WebSerial.print("Calibration Result: Max="); WebSerial.print(calibrationmaxreading); WebSerial.print(" Min="); WebSerial.println(calibrationminreading);
      sendCalibration = false;
    }
    // Do audio presence sensor. High when audio above threshold. Low if decaytime has passed and audio below threshold
    audio = analogRead(audiopin);
    if (audio > audiothreshold) {
      audiostate = true;
      audioDelayTimer = millis();
    } else {
      if (millis() - audioDelayTimer > decay) {
        // if no audio, assuming decay time has elapsed
        audiostate = false;
      }
    }
    // Track state to get rising / falling edges
    if (audiostate && !prevaudiostate) {
      // rising edge
      printLocalTime();
      Serial.println("Audio start  .... ");
      WebSerial.println("Audio start .... ");
      StaticJsonDocument<192> doc;
      doc["audio"] = "start";
      serializeJson(doc, jsonbuff);
      client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json

      repeatTimer = millis();
      prevaudiostate = true;
      repeatmaxreading = 0; // reset the max and min audio levels for repeat messages
    } else if (!audiostate && prevaudiostate) {
      // falling edge
      printLocalTime();
      Serial.println("Audio end");
      WebSerial.println("Audio end");
      StaticJsonDocument<192> doc;
      doc["audio"] = "end";
      serializeJson(doc, jsonbuff);
      client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json

      prevaudiostate = false;
    }
    if (audiostate && millis() - repeatTimer > repeatTime * 1000) {
      // audio has continued, send a repeat message
      printLocalTime();
      Serial.print("Audio continues .... max value: "); Serial.println(repeatmaxreading);
      WebSerial.print("Audio continues .... max value: "); WebSerial.println(repeatmaxreading);
      StaticJsonDocument<192> doc;
      doc["audio"] = "cont";
      doc["max"] = repeatmaxreading;
      serializeJson(doc, jsonbuff);
      client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json

      repeatmaxreading = 0; // reset the max and min audio levels for repeat messages
      repeatTimer = millis(); // start the timer again
    } else if (audiostate && millis() - repeatTimer <= repeatTime * 1000) {
      // collect max and min for the short period of time
      if (audio > repeatmaxreading) repeatmaxreading = audio;
    }



  } // end if calibration mode off

  if (millis() - teleTimer > 60000) {
    // send tele update
    printLocalTime();
    Serial.println("Tele update");
    WebSerial.println("Tele update");
    long rssi = WiFi.RSSI();
    String ipaddress = WiFi.localIP().toString();
//    char ipchar[ipaddress.length()+1];
//    ipaddress.toCharArray(ipchar,ipaddress.length()+1);
    StaticJsonDocument<192> doc;
    doc["rssi"] = rssi;
    doc["ip"] = ipaddress;
    doc["ssid"] = ssid;
    doc["fw"] = firmware;
    doc["at"] = audiothreshold;
    doc["ad"] = decay;
    doc["ar"] = repeatTime;
    serializeJson(doc, jsonbuff);
    client.publish(mqttroot, jsonbuff);  // buff = topic, jsonbuff = serialized json
    teleTimer = millis();
  }
  // Keep MQTT connected
  if (!client.connected()) reconnect();
  client.loop();
}
