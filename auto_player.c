

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mosquitto.h>

#define HOST   "34.102.16.54"
#define PORT   1883
#define BOARD  "cs2600/ttt/board"
#define MSG    "cs2600/ttt/serverToClient"
#define OUT    "cs2600/ttt/clientToServer"

struct mosquitto *m;

void on_message(struct mosquitto *m, void *obj, const struct mosquitto_message *msg) {

    if (strcmp(msg->topic, BOARD) == 0 && msg->payloadlen == 9) {
        char b[10];
        memcpy(b, msg->payload, 9);
        b[9] = '\0';


        int x=0, o=0;
        for (int i=0; i<9; i++) {
            if (b[i]=='X') x++;
            else if (b[i]=='O') o++;
        }

        if (x == o) {
            int empties[9], n=0;
            for (int i=0; i<9; i++)
                if (b[i]=='E') empties[n++] = i;
            if (n>0) {
                int p = empties[rand() % n];
                char file = 'A' + (p % 3);
                char rank = '1' + (p / 3);
                char buf[16];
                snprintf(buf, sizeof(buf), "play %c%c", file, rank);
                // after computing file and rank…
                printf("[CAI] publishing move: play %c%c\n", file, rank);
                mosquitto_publish(m, NULL, OUT, strlen(buf), buf, 0, false);
            }
        }
    }
   
    else if (strcmp(msg->topic, MSG) == 0) {
        if (strncmp((char*)msg->payload, "Congrats", 7)==0 ||
            strstr((char*)msg->payload, "draw")) {
            mosquitto_disconnect(m);
        }
    }
}

int main() {
    srand(time(NULL));
    mosquitto_lib_init();
    m = mosquitto_new(NULL, true, NULL);
    mosquitto_connect(m, HOST, PORT, 60);


    mosquitto_subscribe(m, NULL, BOARD, 0);
    mosquitto_subscribe(m, NULL, MSG,   0);

    mosquitto_message_callback_set(m, on_message);
    mosquitto_loop_forever(m, -1, 1);

    mosquitto_destroy(m);
    mosquitto_lib_cleanup();
    return 0;
}
