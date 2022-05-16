#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets_smartsocket.h"  

unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
 
#define AWS_IOT_PUBLISH_TOPIC   "esp8266/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp8266/sub"

String TIMESTAMP = " ";

time_t now;
time_t nowish = 1510592825;

#define DEBUG true
int ROLE_PIN = 16;

int wifi_error_blink_count = 2;
int aws_error_blink_count = 5;
String ROLE_STATUS = " ";

 
WiFiClientSecure net;
 
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
 
PubSubClient client(net);
 

 
 
void NTPConnect(void)
{
  if(DEBUG){Serial.print("Setting time using SNTP"); }
  configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);
  while (now < nowish)
  {
    delay(500);
    if(DEBUG){Serial.print(".");}
    now = time(nullptr);
  }
  if(DEBUG){Serial.println("done!");}
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  if(DEBUG){
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
  }
  TIMESTAMP = asctime(&timeinfo);
}
 
 
void messageReceived(char *topic, byte *payload, unsigned int length)
{
  if(DEBUG){
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: "); }
  String incoming_packet;
  for (int i = 0; i < length; i++)
  {
    if(DEBUG){Serial.print((char)payload[i]);}
    incoming_packet +=(char)payload[i];
  }
  StaticJsonDocument<200> r_json;
  deserializeJson(r_json,incoming_packet);
  ROLE_STATUS = r_json["role_status"].as<String>();
  if(ROLE_STATUS == "HIGH" ){
    digitalWrite(16, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    }
  if(ROLE_STATUS == "LOW" ){
    digitalWrite(16, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
    }
    publishMessage("after_received");  
}
 
 
void connectAWS()
{
  delay(1000);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED)
  {
    for (byte x = 0; x < (sizeof(WIFI_SSID) / sizeof(WIFI_SSID[0])); x++){
      if(DEBUG){Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID[x]) +" " +String(WIFI_PASSWORD[x]));}
         delay(200);
        WiFi.begin(WIFI_SSID[x], WIFI_PASSWORD[x]);
        delay(5000);
        if(WiFi.status() == WL_CONNECTED){
          break;
          }
    }
    if(WiFi.status() == WL_CONNECTED){
          break;
          }
    if(DEBUG){Serial.println(".");}
    delay(1000);
    for(int i =1 ; i<=wifi_error_blink_count; i++){
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      }
    
    
  }
  if(DEBUG){Serial.println(String("Connected to SSID: ") + String(WiFi.SSID()));}
  
 
  NTPConnect();
 
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);
 
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);
 
 
  if(DEBUG){Serial.println("Connecting to AWS IOT");}
 
  while (!client.connect(THINGNAME))
  {
    if(DEBUG){Serial.print(".");}
    for(int i =1 ; i<=aws_error_blink_count; i++){
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      }
  }
 
  if (!client.connected()) {
    if(DEBUG){Serial.println("AWS IoT Timeout!");}
    return;
  }
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  if(DEBUG){Serial.println("AWS IoT Connected!");}
}
 
 
void publishMessage(String connection_type)
{
  digitalWrite(LED_BUILTIN, LOW);
  NTPConnect();
  StaticJsonDocument<200> doc;
  doc["time"] = TIMESTAMP;
  doc["pub_reason"] = connection_type;
  doc["thingname"] = THINGNAME;
  doc["current_pin_status"] = digitalRead(16);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);

}
 
 
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  connectAWS();
 pinMode(16,OUTPUT);
 publishMessage("on_setupLoop");
 
}
 
 
void loop()
{
 
  now = time(nullptr);
 
  if (!client.connected())
  {
    connectAWS();
    publishMessage("reconnected_aws");
  }
  else
  {
    client.loop();
    if (millis() - lastMillis > 360000)
    {
      lastMillis = millis();
      publishMessage("heartbit");
    }
  }
}
