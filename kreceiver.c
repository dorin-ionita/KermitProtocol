#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

#define MAXL 250
#define TIME 5
#define NPAD 0
#define PADC 0
#define SOH 0x01
#define EOL 0x0D
#define ZERO 0x00

void create_YS_package(msg *t){
    t->payload[0] = SOH;
    t->payload[1] = 5 + 11;
    t->payload[2] = 0x01;
    t->payload[3] = 'Y';
    t->payload[4] = MAXL;
    t->payload[5] = TIME;
    t->payload[6] = NPAD;
    t->payload[7] = PADC;
    t->payload[8] = EOL;
    t->payload[9] = ZERO;
    t->payload[10] = ZERO;      //QBIN
    t->payload[11] = ZERO;      //CHKT
    t->payload[12] = ZERO;      //RPT
    t->payload[13] = ZERO;      // CAPA
    t->payload[14] = ZERO;      //R1
    memcpy(&(t->payload[15]), &(crc16_ccitt(t->payload, 15)), 2) ;  
        // CRC pe primii 9 bytes -> nu e nevoie sa verific in server, though
    t->payload[17] = EOL;       // MARK
    t->payload[18] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_Y_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    t->payload[3] = 'Y';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_N_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    t->payload[3] = 'N';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void check_crc_is_correct(msg *t){
    char received_crc[2], computed_crc[2];
    int start_position_of_crc = t->payload[1] - 1; 
    /* t->payload[1] +1 este pozitia ultimul caracter in string
     (pentru ca LEN incepe sa se numere de la pozitia 2, cum ar fi)
     deci al2-lea caracter din CRC este la t->payload[1],
     deci primul caracter din CRC este la t->payload[1] - 1 */
    memcpy(&received_crc, &(t->payload[start_position_of_crc]), 2);
    memcpy(&computed_crc, &(crc16_ccitt(t->payload, start_position_of_crc)), 2);
    return
        received_crc[1] == computed_crc[1] && received_crc[0] == computed_crc[0];
}

int main(int argc, char** argv) {
    int i;
    msg r, t;
    init(HOST, PORT);
    char SEQ = 1;
    int file_handler = -1;
    char file_name[500];
    int allready_received_S = 0;
    while(){
        for (i = 0 ; i < 3; i++){
            if (&r = receive_message_timeout(TIME), &r)
                break;
            if (allready_received_S)
                send_message(&t);
            if (i == 2 || (i == 1 && !allready_received_S) )
                return 1;
        }
        if (!check_S_is_correct(&r)){
            create_N_package(&t, SEQ);
            send_message(&t);
            SEQ++++;
            SEQ %= 64;
            continue;
        }
        switch (r.payload[3]){
            case 'S':
                allready_received_S = 1;
                create_Y_package(&t, SEQ);
                send_message(&t);
                SEQ++++;
                SEQ %= 64;
                break;
            case 'F':
                memcpy(file_name, &(t.payload[4]), t.payload[1] - 5);
                file_name[t.payload[1] - 5] = '\0';
                file_handler = open(file_name, O_APPEND);
                create_Y_package(&t, SEQ);
                send_message(&t);
                SEQ++++;
                SEQ %= 64;
                break;
            case 'D':
                write(file_handler, &(t.payload[4]), t.payload[1] - 5);
                create_Y_package(&t, SEQ);
                send_message(&t);
                SEQ++++;
                SEQ %= 64;
                break;
            case 'Z':
                close(file_handler);
                create_Y_package(&t, SEQ);
                send_message(&t);
                SEQ++++;
                SEQ %= 64;
                break;
            case 'B':
                create_Y_package(&t, SEQ);
                send_message(&t);
                SEQ++++;
                SEQ %= 64;
                break;
        }
        if (r.payload[3] == 'B')
            break;
    }

	return 0;
}
