#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "Hello from test AP");
}

void setup() {
  Serial.begin(115200); delay(200);
  WiFi.softAP("AmazonBox-TEST",""); // open AP
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  server.on("/", handleRoot);
  server.begin();
  Serial.println("server started");
}

void loop() {
  server.handleClient();
}