#include <LittleFS.h>
#include <WiFi.h>
#include <PicoMQTT.h>

String WIFI_SSID;
String WIFI_PASS;

const char* MQTT_BROKER_IP = "35.236.75.26";

PicoMQTT::Client mqttClient(MQTT_BROKER_IP);

// All reactions to incoming messages are handled here, for simplicity.
void onReceiveMessage(const char* topic, const char* payload) {
    if (strcasecmp(payload, "startgame") == 0) {
      setupGame();
    }
}

// On call, resets the current game state, if any, and starts a new game.
void setupGame() {
  mqttClient.publish("cs2600/ttt", "Game started!");

  char board[3][3];

  for (int row = 0; row <3; ++row)  {
    for (int column = 0; column < 3; ++column) {
      board[row][column] = 'E';
    }
  }
}

// Here I handle MQTT setup, LittleFS setup for loading credentials (no wifi password for you!), and basic wifi setup.
void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed! Oops!");
    return;
  }

// This file lives on the ESP32 but is not included in the repository.
  File credentialFile = LittleFS.open("/credentials.txt", FILE_READ);

  if (!credentialFile) {
    Serial.println("Could not open credentials file. Something is definitely wrong here.");
    return;
  }

  WIFI_SSID = credentialFile.readStringUntil('\n');
  WIFI_SSID.trim();
  WIFI_PASS = credentialFile.readStringUntil('\n');
  WIFI_PASS.trim();

  Serial.print("Loaded SSID: '"); Serial.print(WIFI_SSID); Serial.println("'");
  credentialFile.close();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connection successful. Hello World!");

  mqttClient.subscribe("cs2600/ttt", onReceiveMessage);
  mqttClient.begin();
}

void loop() {
  mqttClient.loop();
}
