#include <LittleFS.h>
#include <WiFi.h>
#include <PicoMQTT.h>

#define SENDMSG(x) mqttClient.publish("cs2600/ttt/serverToClient", x)

String WIFI_SSID;
String WIFI_PASS;
char currentPlayer;


enum GameState {
        IDLE,
        WAITING_FOR_MODE,
        IN_GAME_SINGLE,
        IN_GAME_MULTI,
        GAME_ENDED
};

char board[3][3];
GameState gameState = IDLE;

const char* MQTT_BROKER_IP = "35.236.75.26";

PicoMQTT::Client mqttClient(MQTT_BROKER_IP);

void announceWinner(char winner) {
        char winMessage[32];

        if (winner == 'D') {
                SENDMSG("It's a draw!");
        } else {
                snprintf(winMessage, size_t(winMessage), "Player %c is the winner!", winner);
                SENDMSG(winMessage);
        }
        gameState = GAME_ENDED;
        SENDMSG("Play again? (yes/no)");
}

// This function just brute-force checks if there are any three-in-a-rows.
void checkWinCondition() {

        for (int i = 0; i < 3; ++i) {
                if (board[i][0] != 'E' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
                        announceWinner(board[i][0]);
                        return;
                }
                if (board[0][i] != 'E' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
                        announceWinner(board[0][i]);
                        return;
                }
        }

        if (board[0][0] != 'E' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
                announceWinner(board[0][0]);
                return;
        }

        if (board[0][2] != 'E' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
                announceWinner(board[0][2]);
                return;
        }
        // 3) Draw?
        bool full = true;
        for (int r = 0; r < 3 && full; ++r)
                for (int c = 0; c < 3; ++c)
                        if (board[r][c] == 'E') {
                                full = false;
                                break;
                        }
        if (full) {
                announceWinner('D');
        }
}

void onReceiveMessage(const char* topic, const char* payload) {
        if (strcasecmp(payload, "startgame") == 0) {
                setupGame();
                return;
        }

        // Handling setup messages
        if (gameState == WAITING_FOR_MODE) {
                if (strcmp(payload, "1") == 0) {
                        startSingleplayer();
                } else if (strcmp(payload, "2") == 0) {
                        startMultiplayer();
                } else {
                        SENDMSG("Invalid choice. Enter 1 or 2.");
                }
                return;
        }

        // Handling 2-player game messages
        if (gameState == IN_GAME_MULTI) {
                if (strncasecmp(payload, "play ", 5) == 0 && strlen(payload) == 7) {
                        char file = toupper(payload[5]);
                        char rank = payload[6];
                        int col = file - 'A';
                        int row = rank - '1';

                        if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                                if (board[row][col] == 'E') {
                                        board[row][col] = currentPlayer;
                                        broadcastBoardState();
                                        currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
                                        checkWinCondition();
                                } else {
                                        SENDMSG("Invalid move: cell occupied");
                                }
                        } else {
                                SENDMSG("Invalid coordinate. Use A1..C3");
                        }
                } else {
                        SENDMSG("Invalid command. Use play <A1..C3>");
                }
                return;
        }

        if (gameState == GAME_ENDED) {
                if (strcasecmp(payload, "yes") == 0) {
                        setupGame();
                } else if (strcasecmp(payload, "no") == 0) {
                        SENDMSG("Thanks for playing! Goodbye.");
                        gameState = IDLE;
                } else {
                        SENDMSG("Please answer 'yes' or 'no'.");
                }
                return;
        }
}

// On call, resets the current game state, if any, and starts a new game.
void setupGame() {

        for (int row = 0; row < 3; ++row) {
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
        SENDMSG("To choose a tile, type: play followed by a coordinate. Example: play A2");
        gameState = IN_GAME_SINGLE;
        currentPlayer = 'X';
        broadcastBoardState();
}

// Starts the game loop for 2-player mode
void startMultiplayer() {
        SENDMSG("Starting 2-player local game. X will go first, and O second.");
        SENDMSG("To choose a tile, type: play followed by a coordinate. Example: play A2");
        gameState = IN_GAME_MULTI;
        currentPlayer = 'X';
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

        Serial.print("Loaded SSID: '");
        Serial.print(WIFI_SSID);
        Serial.println("'");
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
