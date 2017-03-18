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
    memcpy(&(t->payload[15]), &(crc16_ccitt(t->payload, 15)), 2) ;  // CRC pe primii 9 bytes -> nu e nevoie sa verific in server, though
    t->payload[16] = EOL;       // MARK
    t->payload[17] = '\0';
    t->len = strlen(t->payload);
}

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

void check_crc_is_correct(msg *t){
    char received_crc[2], computed_crc[2];
    int start_position_of_crc = t->payload[1] - 1; 
    /* t->payload[1] +1 este pozitia ultimul caracter in string
     deci pozitia CRC este t->payload[1] - 2 */
    memcpy(&received_crc, &(t->payload[start_position_of_crc]), 2);
    memcpy(&received_crc, &(crc16_ccitt(t->payload, start_position_of_crc)), 2);
    return
        received_crc[1] == computed_crc[1] && received_crc[0] == computed_crc[0];
}



int main(int argc, char** argv) {
    msg r, t;
    init(HOST, PORT);
    char SEQ = 1;
    int file_handler = -1;
    while(){
        if (recv_message(&r) < 0) {
            perror("Receive message");
            return -1;
        }
        if (!check_S_is_correct(r)){
            create_N_package(t);
            send_message(&t);
            SEQ++++;
            continue;
        }
        switch (r.payload[3]){
            case 'S':
                create_Y_package(&t);
                send_message(&t);
                SEQ++++;
                break;
            case 'F':
                file_handler = set_file_handler_from_F(t);
                create_Y_package(&t);
                send_message(&t);
                SEQ++++;
                break;
            case 'D':
                write_data_in_file(file_handler, t);
                create_Y_package(&t);
                send_message(&t);
                SEQ++++;
                break;
            case 'Z':
                end_writing_in_file(file_handler);
                create_Y_package(&t);
                send_message(&t);
                SEQ++++;
                break;
            case 'B':
                create_Y_package(&t);
                send_message(&t);
                SEQ++++;
                break;
        }
        if (r.payload[3] == 'B')
            break;
    }

    /*
    printf("[%s] Got msg with payload: %s\n", argv[0], r.payload);

    
    unsigned short crc = crc16_ccitt(r.payload, r.len);
    sprintf(t.payload, "CRC(%s)=0x%04X", r.payload, crc);
    t.len = strlen(t.payload);
    send_message(&t);*/

	
	return 0;
}
