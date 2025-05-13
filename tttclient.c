#include <stdio.h>
#include <string.h>
#include <mosquitto.h>

#define BROKER_IP   "35.236.75.26"
#define BROKER_PORT 1883
#define INCOMING_TOPIC "cs2600/ttt/serverToClient"
#define OUTGOING_TOPIC "cs2600/ttt/clientToServer"
#define BOARD_STATE_TOPIC "cs2600/ttt/board"

// Prints out a formatted version of the board state received from ESP32
void print_board(const char *payload) {
    printf("\n");
    for (int r = 0; r < 3; ++r) {
        printf(" %c | %c | %c \n",
               payload[3*r + 0]=='E'?' ':payload[3*r + 0],
               payload[3*r + 1]=='E'?' ':payload[3*r + 1],
               payload[3*r + 2]=='E'?' ':payload[3*r + 2]);
        if (r < 2) printf("---+---+---\n");
    }
    printf("\n");
}

// Called for each message received
void on_message(struct mosquitto *mosq, void *obj,
    const struct mosquitto_message *msg) {
       
        if (strcmp(msg->topic, BOARD_STATE_TOPIC) == 0) {
            char *board_state = (char*)msg->payload;
            print_board(board_state);
        }
        else {
            printf("[Message] %.*s\n", msg->payloadlen, (char*)msg->payload);
        }
}



int main(int argc, char *argv[]){
    struct mosquitto *mosq;
    char input[256];

    mosquitto_lib_init();

    // Create a new client instance
    mosq = mosquitto_new("ttt_client", true, NULL);
    if(!mosq){
        fprintf(stderr, "Failed to create client\n");
        return 1;
    }


    mosquitto_message_callback_set(mosq, on_message);

    // Connect to broker
    if(mosquitto_connect(mosq, BROKER_IP, BROKER_PORT, 60)){
        fprintf(stderr, "Unable to connect\n");
        return 1;
    }

    mosquitto_subscribe(mosq, NULL, INCOMING_TOPIC, 0);
    mosquitto_subscribe(mosq, NULL, BOARD_STATE_TOPIC, 0);

    mosquitto_loop_start(mosq);

    printf(
      "Welcome to the Tic-Tac-Toe Player Client.\n"
      "To send a command to the server, simply enter any text.\n"
      "Type Ctrl+C to quit.\n"
    );

    while(fgets(input, sizeof(input), stdin)){
        size_t len = strlen(input);
        if(len > 0 && input[len-1]=='\n'){
            input[len-1] = '\0';
        }
        mosquitto_publish(mosq, NULL, OUTGOING_TOPIC,
                          strlen(input), input, 0, false);
    }

    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
