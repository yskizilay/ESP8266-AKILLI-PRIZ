#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

const char* ssid = "KIZILAY_HOME";
const char* password = "Kizilay_Home_58_Engineers";

WiFiUDP Udp;
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "220vControl";

unsigned int localUdpPort = 5000;  // local port to listen on
char incomingPacket[255];  // buffer for incoming packets
char  replyPacekt[] = "Succes";  // a reply string to send back
int light = 2;
int socket = 16;
void setup()
{
  Serial.begin(115200);
  Serial.println();
  pinMode(socket, OUTPUT);
  pinMode(light, OUTPUT);

  Serial.printf("Connecting to %s ", ssid);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

  digitalWrite(socket, HIGH);
  delay(500);
  digitalWrite(socket,LOW);
  delay(100);
    digitalWrite(socket, HIGH);
  delay(500);
  digitalWrite(socket,LOW);
  delay(100);
  digitalWrite(light, HIGH);
}

void loop()
{
  httpServer.handleClient();
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // receive incoming UDP packets
    Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    Serial.printf("UDP packet contents: %s\n", incomingPacket);
    String incommingstringmessage(incomingPacket);
    StaticJsonBuffer<250> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(incommingstringmessage);
    String socketstatus = root["socketstatus"];
    

    if(incomingPacket[0] == '1'){
    digitalWrite(socket, HIGH);
    digitalWrite(light, LOW);
    delay(100);
    digitalWrite(light,HIGH);
    Serial.println("Socket_turned_on");
    }
  else if(incomingPacket[0] == '0'){
    digitalWrite(socket, LOW);
   
        Serial.println("Socket_turned_off");
    }
  else {
    digitalWrite(socket, LOW);
        Serial.println("Else_Socket_turned_off");
    }
 
    // send back a reply, to the IP address and port we got the packet from
    //Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    //Udp.write(replyPacekt);
    //Udp.endPacket();
  }
}
