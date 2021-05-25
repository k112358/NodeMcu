#include <stdio.h>
//SPIFFS
#include <FS.h>
#include <ESP8266WiFi.h>
//MQTT
#include <PubSubClient.h>
// for DS18B20
//#include <OneWire.h>
//#include <DallasTemperature.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// for DHT11
#include "DHT.h"


// DHT Sensor
uint8_t LEDPin = 16;

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// DHT Sensor
uint8_t DHTPin = D5;
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);                

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On a  ESP8266_NodeMcu:   D2(SDA), D1(SCL)  <-----
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin) ## change 4 to -1
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 ## change 0X3D to 0x3C for 12864
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String wifiFile = "/wifiFile.txt"; 
const char* mqtt_server = "";
const int   mqtt_port = 1884;
const char* mqtt_usr = "";
const char* mqtt_pwd = "";
const char* mqtt_out_topic = "MQTT-DHT11";
const char* mqtt_in_topic = "MQTT-LED";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_usr, mqtt_pwd)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_out_topic, "hello world");
      // ... and resubscribe
      client.subscribe(mqtt_in_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void saveWiFiData(String wifi_ssid, String wifi_psk) {
  File wFile = SPIFFS.open(wifiFile, "w");
  if (wFile) {
    //wFile.print("SSID:");
    wFile.print(wifi_ssid);
    wFile.print(" ");
    //wFile.print(" PSK:");
    wFile.println(wifi_psk);
    wFile.close();
  }
}

String getWiFiSsid() {
  String wifiSsid = "";
  File rFile = SPIFFS.open(wifiFile, "r");
  if (rFile) {
    //char ssid[50];
    //char psk[50];
    String data = rFile.readString();
    int pos = data.indexOf(' ');
    if(pos >= 0){
      wifiSsid = data.substring(0, pos);
      wifiSsid.trim();
      //Serial.printf("--->get SSID:%s\n", wifiSsid.c_str());
    }
    //sscanf(data.c_str(),"%s %s:", ssid, psk);
    //Serial.printf("--->get SSID:%s\n", ssid);
    //wifiSsid = ssid;
    rFile.close();
  }
  return wifiSsid;
}

String getWiFiPsk() {
  String wifiPsk = "";
  File rFile = SPIFFS.open(wifiFile, "r");
  if (rFile) {
    //char ssid[50];
    //char psk[50];
    String data = rFile.readString();
    int pos = data.indexOf(' ');
    if(pos >= 0){
      wifiPsk = data.substring(pos + 1);
      wifiPsk.trim(); // remove '\n' in the end
      //Serial.printf("--->get PSK:%s\n", wifiPsk.c_str());
    }
    //sscanf(data.c_str(),"%s %s:", ssid, psk);
    //Serial.printf("--->get PSK:%s\n", psk);
    //wifiPsk = psk;
    rFile.close();
  }
  return wifiPsk;
}

void toggleLED() {
  int val = digitalRead(LED_BUILTIN); 
  if (val == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);  
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void wifiSmartConfig() {
  Serial.println("");
  Serial.println("Waiting for SmartConfig.");
  
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  boolean isConfigDone = false;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    toggleLED();
    if(!isConfigDone && WiFi.smartConfigDone()){
      isConfigDone = true;
      Serial.println("");
      Serial.println("SmartConfig received."); 
      Serial.println("Waiting for WiFi..."); 
    }
  }
  Serial.println("");
  Serial.println("WiFi Connected.");
  
  String wifi_ssid = WiFi.SSID();
  String wifi_psk = WiFi.psk();
  Serial.print("SSID: ");
  Serial.println(wifi_ssid);
  Serial.print("PSK: ");
  Serial.println(wifi_psk);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  saveWiFiData(wifi_ssid, wifi_psk);
  
  int cnt = 0;
  while (cnt++ <= 50) {
    toggleLED();
    delay(100);
  }
  digitalWrite(LED_BUILTIN, HIGH);// turn off
}

void setup_wifi() {
  boolean isWiFiConnected = false;
  String wifi_ssid = getWiFiSsid();
  String wifi_psk = getWiFiPsk();
  if (wifi_ssid.length() > 0 && wifi_psk.length() > 0) {
    Serial.printf("===>>>SSID:[%s]\n", wifi_ssid.c_str());
    Serial.printf("===>>> PSK:[%s]\n", wifi_psk.c_str());

    int try_cnt = 0;
    Serial.printf("Connecting to [%s]\n", wifi_ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_psk.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      if (try_cnt++ >= 40) { //try 20 seconds
        break;
      }
      toggleLED();
      delay(500);
      Serial.print("*");
    }

    Serial.println("");
    if(WiFi.status() == WL_CONNECTED) {
      isWiFiConnected = true;
      Serial.println("WiFi connect success!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      int cnt = 0;
      while (cnt++ <= 50) {
        toggleLED();
        delay(100);
      }
      digitalWrite(LED_BUILTIN, HIGH);// turn off
    }
  }

  if (!isWiFiConnected) {
    wifiSmartConfig();
  }
}

void showOnScreen(float temperature, float humidity){
  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.print(F("Temp:"));
  display.print((int)temperature);
  display.drawCircle(display.getCursorX() + 1, display.getCursorY(), 2, SSD1306_WHITE);
  display.setCursor(display.getCursorX() + 3, display.getCursorY());
  display.println("C");
  display.print(F("Humi:"));
  display.print((int)humidity);
  display.println("%");
  display.display();      // Show initial text 
}

void printDports(){
  Serial.printf("---LED_BUILTIN--- %d\n", LED_BUILTIN);
  Serial.printf("--- D0 --- %d\n", D0);
  Serial.printf("--- D1 --- %d\n", D1);
  Serial.printf("--- D2 --- %d\n", D2);
  Serial.printf("--- D3 --- %d\n", D3);
  Serial.printf("--- D4 --- %d\n", D4);
  Serial.printf("--- D5 --- %d\n", D5);
  Serial.printf("--- D6 --- %d\n", D6);
  Serial.printf("--- D7 --- %d\n", D7);
  Serial.printf("--- D8 --- %d\n", D8); 
}

String getLightStatus() {
  String ret = "";
  int val = digitalRead(LEDPin); 
  if (val == LOW) {
    ret = "on";
  } else {
    ret = "off";
  }
  return ret;
}

void turnOffLight() {
  //int val = digitalRead(LEDPin); 
  //if (val == LOW) {
    digitalWrite(LEDPin, HIGH);  
  //} else {
  //  digitalWrite(LEDPin, LOW);
  //}
}

void turnOnLight() {
  digitalWrite(LEDPin, LOW);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  Serial.begin(115200);

  turnOffLight();
  pinMode(DHTPin, INPUT);
  dht.begin();
  delay(1000);
  Serial.println("");
  
  printDports();
  if(SPIFFS.begin()){ // 启动SPIFFS
    Serial.println("SPIFFS Started.");
  } else {
    Serial.println("SPIFFS Failed to Start.");
  }

  Serial.println("SSD1306 OLED Init..."); 
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Clear the buffer
  display.clearDisplay();
  display.display();
  Serial.println("SSD1306 OLED OK!");
  
  setup_wifi();
  
  client.setServer(mqtt_server, 1884);//mqtt_port
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    ++value;

    float t = dht.readTemperature(); // Gets the values of the temperature
    float h = dht.readHumidity(); // Gets the values of the humidity
    showOnScreen(t, h);
    String light = getLightStatus();
    snprintf (msg, MSG_BUFFER_SIZE, "DHT11(%ld)#%3.1f#%2.0f#%s", value, t, h, light.c_str());
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(mqtt_out_topic, msg);

    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
