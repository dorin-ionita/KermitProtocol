#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

#define MAXL 250
#define TIME 5
#define NPAD 0
#define PADC 0
#define SOH 0x01
#define EOL 0x0D
#define ZERO 0x00

void create_S_package(msg* t){
    t->payload[0] = SOH;        //SOH, mereu 0x01
    t->payload[1] = 5 + 11;       //5 + lungime camp DATA
    t->payload[2] = 0x00; //SEQ
    t->payload[3] = 'S';         //TYPE
    t->payload[4] = MAXL;        //MAXL
    t->payload[5] = TIME;        //TIME
    t->payload[6] = NPAD;        //NPAD
    t->payload[7] = PADC;
    t->payload[8] = EOL;        //EOL
    t->payload[9] = ZERO;       //QCTL
    t->payload[10] = ZERO;      //QBIN
    t->payload[11] = ZERO;      //CHKT
    t->payload[12] = ZERO;      //RPT
    t->payload[13] = ZERO;      // CAPA
    t->payload[14] = ZERO;      //R
    memcpy(&(t->payload[15]), &(crc16_ccitt(t->payload, 15)), 2) ;  // CRC pe primii 9 bytes
    t->payload[16] = EOL;       // MARK
    t->payload[17] = '\0';
}
void create_F_package(msg *t, char* file_name, int current_SEQ){
    t->payload[0] = SOH;        //SOH, mereu 0x01
    t->payload[1] = 5 + strlen(file_name) + 1;       //5 + lungime camp DATA + \0 pentru numele din data
    t->payload[2] = current_SEQ;        //SEQ
    t->payload[3] = 'F';         //TYPE
    memcpy(&(t->payload[4]), file_name, strlen(file_name) + 1);
    memcpy(&(t->payload[4 + strlen(file_name) + 1 + 1]),
        &(crc16_ccitt(t->payload, 4 + strlen(file_name) + 1 + 1)), 2);
    t->payload[4 + strlen(file_name) + 1 + 3] = EOL;
    t->payload[4 + strlen(file_name) + 1 + 4] = '\0';
}

void create_D_package(msg *t, void* buffer_zone, int buffer_length, int current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5 + buffer_length;
    t->payload[2] = current_SEQ;
    t->payload[3] = 'D';
    memcpy((&(t->payload[4])), buffer_zone, buffer_length);
    memcpy((&(t->payload[4] + buffer_length)), &(crc16_ccitt(t->payload, 4 + buffer_length)), 2);
    t->payload[4 + payload[4] + buffer_length + 2] = EOL;
    t->payload[4 + payload[4] + buffer_length + 3] = '\0';
}

void create_Z_package(msg *t, int current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    /* the data part is missing in Z packages */
    t->payload[3] = 'Z';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
}

void create_B_package(msg *t, int current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    /* the data part is missing in Z packages */
    t->payload[3] = 'B';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
}

typedef struct {
    char npad;
    char padc;
    char eol;
} ReceiverInfo;

ReceiverInfo receiver;

void get_receiver_info_from_ack(msg* answer){
    char npad = answer[6];
    char padc = answer[7];
    char eol = answer[8];
}

void send_file_with_name(char argv[], int current_SEQ){
    int file_handler = open(argv, O_RDONLY); // practic am deschis fisierul pentru citire
    int no_of_bytes_read = -1;
    void *buffer = (void *)malloc(250);
    // in primul trebuie trimis un pachet de tip S
    while (no_of_bytes_read){
        no_of_bytes_read = read (file_handler, buffer, 250);
        if (no_of_bytes_read) {
            // atunci creeaza pachetul
        }
    }
    close(file_handler);
}

int main(int argc, char** argv) {
    // argc = nr de argumente + 1 (pentru numele executabilului)
    msg t;
    msg *answer;
    init(HOST, PORT);
    // vrajeli ^

/*    sprintf(t.payload, "Hello World of PC");
    t.len = strlen(t.payload);
    send_message(&t);*/

    // send files
    msg t;
    create_S_package(t);
    t.len = strlen(t.payload);
    send_message(&t);
    // TODO asteapta ACK
    int i;
    for (i = 0 ; i < 3 ; i++){
        if (answer = receive_message_timeout(5), answer)
            break; // e garantat ca de la receptor vin doar date corecte
        if (i == 3)
            return 1; // Termina conexiunea
    }
    get_receiver_info_from_ack(answer);
    for (i = 1 ; i < argc ; i++){
        if (send_file_with_name(argv[i]))
            return 1; // daca intampin eroare, intorc eroare
    }


    msg *y = receive_message_timeout(5000);
    if (y == NULL) {
        perror("receive error");
    } else {
        printf("[%s] Got reply with payload: %s\n", argv[0], y->payload);
    }

    
    return 0;
}
