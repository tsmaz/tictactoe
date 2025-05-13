#include <LittleFS.h>
#include <WiFi.h>
#include <PicoMQTT.h>

#define SENDMSG(x) mqttClient.publish("cs2600/ttt/serverToClient", x)

String WIFI_SSID;
String WIFI_PASS;

enum GameState {
  IDLE,
  WAITING_FOR_MODE,
  IN_GAME_SINGLE,
  IN_GAME_MULTI
};

char board[3][3];
GameState gameState = IDLE;

const char* MQTT_BROKER_IP = "35.236.75.26";

PicoMQTT::Client mqttClient(MQTT_BROKER_IP);

// All reactions to incoming messages are handled here, for simplicity.
void onReceiveMessage(const char* topic, const char* payload) {
  if (strcasecmp(payload, "startgame") == 0) {
    setupGame();
    return;
  }

  if (gameState == WAITING_FOR_MODE) {
    
    if (strcmp(payload, "1") == 0) {
      startSingleplayer();
    }
    else if (strcmp(payload, "2") == 0) {
      startMultiplayer();
    }
    else {
      SENDMSG("Invalid choice. Enter 1 or 2.");
      return;
    }

    return;
  }
}

// On call, resets the current game state, if any, and starts a new game.
void setupGame() {

  for (int row = 0; row <3; ++row)  {
    for (int column = 0; column < 3; ++column) {
      board[row][column] = 'E';
    }
  }

  SENDMSG("Game started!");
  SENDMSG("Enter 1 for 1-player mode, or 2 for 2-player mode");
  gameState = WAITING_FOR_MODE;
}

// Starts the game loop for 1-player mode
void startSingleplayer() {
  SENDMSG("Starting 1-player game. You will go first, and then the AI second.");
  gameState = IN_GAME_SINGLE;
}

// Starts the game loop for 2-player mode
void startMultiplayer() {
  SENDMSG("Starting 2-player local game. X will go first, and O second.");
  gameState = IN_GAME_MULTI;
  broadcastBoardState();
}

// Publishes the board state as it is when the function is called
void broadcastBoardState() {
  char payload[10];
  int idx = 0;
  for (int row = 0; row < 3; ++row) {
    for (int col = 0; col < 3; ++col) {
      payload[idx++] = board[row][col];
    }
  }
  payload[idx] = '\0';
  mqttClient.publish("cs2600/ttt/board", payload);
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

  mqttClient.subscribe("cs2600/ttt/clientToServer", onReceiveMessage);
  mqttClient.begin();
}

void loop() {
  mqttClient.loop();
}
