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
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_F_package(msg *t, char* file_name, char current_SEQ){
    t->payload[0] = SOH;        //SOH, mereu 0x01
    t->payload[1] = 5 + strlen(file_name);       //5 + lungime camp DATA
    t->payload[2] = current_SEQ;        //SEQ
    t->payload[3] = 'F';         //TYPE
    memcpy(&(t->payload[4]), file_name, strlen(file_name));
    // ex: pt strlen = 1 avem ca la payload[4] se gaseste sirul,
    // iar la payload[5 (si 6)] se gaseste crc
    memcpy(&(t->payload[4 + strlen(file_name)]),
        &(crc16_ccitt(t->payload, 4 + strlen(file_name))), 2);
    t->payload[4 + strlen(file_name) + 2] = EOL; // 4 +1 +2 = 7 :D
    t->payload[4 + strlen(file_name) + 3] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_D_package(msg *t, void* buffer_zone, int buffer_length, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5 + buffer_length;
    t->payload[2] = current_SEQ;
    t->payload[3] = 'D';
    memcpy((&(t->payload[4])), buffer_zone, buffer_length);
    memcpy((&(t->payload[4 + buffer_length])), &(crc16_ccitt(t->payload, 4 + buffer_length)), 2);
    t->payload[4 + payload[4] + buffer_length + 2] = EOL;
    t->payload[4 + payload[4] + buffer_length + 3] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok


void create_Z_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    /* the data part is missing in Z packages */
    t->payload[3] = 'Z';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_B_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    /* the data part is missing in Z packages */
    t->payload[3] = 'B';
    memcpy(&(t->payload[4]), &(crc16_ccitt(t->payload, 4)), 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

typedef struct {
    char npad;
    char padc;
    char eol;
} ReceiverInfo;
// CHECK: pare ok

ReceiverInfo receiver;

int is_acknowledgement(msg* answer){
    return (answer->payload[3] == 'Y');
}

void get_receiver_info_from_ack(msg* answer){
    char npad = answer[6];
    char padc = answer[7];
    char eol = answer[8];
}
// CHECK: pare ok

int send_name(char argv[], char current_SEQ){
    msg t;
    msg *answer;
    int i;
    create_F_package(&t, argv, char current_SEQ);
    do{
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            if (answer = receive_message_timeout(TIME), answer)
                break; // e garantat ca de la receptor vin doar date corecte
            if (i == 3)
                return 1; // Termina conexiunea
        }
    } while (!is_acknowledgement(answer));
    return 0;
}

int send_file_with_name(char argv[], char *current_SEQ){
    send_name(argv, current_SEQ) ? return 1 : ;;
    (*current_SEQ)++++;
    (*current_SEQ) %= 64;
    int i;
    int file_handler = open(argv, O_RDONLY); // practic am deschis fisierul pentru citire
    int no_of_bytes_read = -1;
    msg t;
    msg *answer;
    void *buffer = (void *)malloc(MAXL);
    while (no_of_bytes_read){
        no_of_bytes_read = read (file_handler, buffer, MAXL);
        if (no_of_bytes_read) {
            create_D_package(&t, buffer_zone, MAXL, *current_SEQ);
            do{
                if (send_message(&t) < 0)
                    return 1;
                for (i = 0 ; i < 3 ; i++){
                    if (answer = receive_message_timeout(TIME), answer)
                        break; // e garantat ca de la receptor vin doar date corecte
                    if (i == 3)
                        return 1; // Termina conexiunea
                }
            } while (!is_acknowledgement(answer));
            (*current_SEQ)++++;
            (*current_SEQ) %= 64;
        }
    }
    create_Z_package(&t, *current_SEQ);
    do{
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            if (answer = receive_message_timeout(TIME), answer)
                break; // e garantat ca de la receptor vin doar date corecte
            if (i == 3)
                return 1; // Termina conexiunea
        }
    } while (!is_acknowledgement(answer));
    (*current_SEQ)++++;
    (*current_SEQ) %= 64;
    close(file_handler);
}

int main(int argc, char** argv) {
    // argc = nr de argumente + 1 (pentru numele executabilului)
    int i;
    msg t;
    msg *answer;
    init(HOST, PORT);
    char current_SEQ = 0; // plec cu nr de secventa initial 0
    msg t;
    create_S_package(t);
    t.len = strlen(t.payload);
    do{
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            if (answer = receive_message_timeout(TIME), answer)
                break; // e garantat ca de la receptor vin doar date corecte
            if (i == 3)
                return 1; // Termina conexiunea
        }
    } while (!is_acknowledgement(answer));
    current_SEQ++++;
    current_SEQ %= 64;
    get_receiver_info_from_ack(answer);
    for (i = 1 ; i < argc ; i++){
        if (send_file_with_name(argv[i], &current_SEQ))
            return 1; // daca intampin eroare, intorc eroare
    }
    create_B_package(&t, current_SEQ);
    do{
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            if (answer = receive_message_timeout(TIME), answer)
                break; // e garantat ca de la receptor vin doar date corecte
            if (i == 3)
                return 1; // Termina conexiunea
        }
    } while (!is_acknowledgement(answer));
    current_SEQ++++;
    current_SEQ %= 64;
    return 0;
}