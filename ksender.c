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
#define TIME 5000
#define NPAD 0
#define PADC 0
#define SOH 0x01
#define EOL 0x0D
#define ZERO 0x00

char last_SEQ;

void create_S_package(msg* t){
    t->payload[0] = SOH;        //SOH, mereu 0x01
    t->payload[1] = 5 + 11;       //5 + lungime camp DATA
    t->payload[2] = 0x00; //SEQ
    //printf("SEQ SENT FOR S IS%c\n", t->payload[2]);
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
    unsigned short crc = crc16_ccitt(t->payload, 15);
    memcpy(&(t->payload[15]), &crc, 2) ;  // CRC pe primii 9 bytes
    t->payload[16] = EOL;       // MARK
    t->payload[17] = '\0';
    t->len = strlen(t->payload);
}

void create_F_package(msg *t, char* file_name, char current_SEQ){
    t->payload[0] = SOH;        //SOH, mereu 0x01
    t->payload[1] = 5 + strlen(file_name);       //5 + lungime camp DATA
    t->payload[2] = current_SEQ;        //SEQ
    //printf("SEQ SENT FOR F IS%c\n", t->payload[2]);
    t->payload[3] = 'F';         //TYPE
    memcpy(&(t->payload[4]), file_name, strlen(file_name));
    unsigned short crc = crc16_ccitt(t->payload, 4 + strlen(file_name)); 
    memcpy(&(t->payload[4 + strlen(file_name)]), &crc, 2);
    t->payload[4 + strlen(file_name) + 2] = EOL; // 4 +1 +2 = 7 :D
    t->payload[4 + strlen(file_name) + 3] = '\0';
    t->len = strlen(t->payload);
}

void create_D_package(msg *t, void* buffer_zone, int buffer_length, char current_SEQ){
    t->payload[0] = SOH;
    //printf("Buffer length\n%u", buffer_length);
    t->payload[1] = 5 + buffer_length;
    t->payload[2] = current_SEQ;
    //printf("SEQ SENT FOR D IS%c\n", t->payload[2]);
    t->payload[3] = 'D';
    memcpy((&(t->payload[4])), buffer_zone, buffer_length);
    unsigned short crc = crc16_ccitt(t->payload, 4 + buffer_length);
    memcpy((&(t->payload[4 + buffer_length])), &crc, 2);
    t->payload[4 + buffer_length + 2] = EOL;
 //   int i;
/*    for (i = 0 ; i < 4 + buffer_length + 2; i++)
        if (t->payload[i] == '\0')
            printf("AI BELIT PULA\n");*/
    t->payload[4 + buffer_length + 3] = '\0';
    t->len = 4 + buffer_length + 3;
}

void create_Z_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    //printf("SEQ SENT FOR Z IS%c\n", t->payload[2]);
    t->payload[3] = 'Z';
    unsigned short crc = crc16_ccitt(t->payload, 4);
    memcpy(&(t->payload[4]), &crc, 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}
// CHECK: pare ok

void create_B_package(msg *t, char current_SEQ){
    t->payload[0] = SOH;
    t->payload[1] = 5;
    t->payload[2] = current_SEQ;
    //printf("SEQ SENT FOR B IS%c\n", t->payload[2]);
    t->payload[3] = 'B';
    unsigned short crc = crc16_ccitt(t->payload, 4);
    memcpy(&(t->payload[4]), &crc, 2);
    t->payload[6] = EOL;
    t->payload[7] = '\0';
    t->len = strlen(t->payload);
}

int is_acknowledgement(msg* answer){
    return (answer->payload[3] == 'Y');
}

void get_receiver_info_from_ack(msg* answer){
    char npad = answer->payload[6];
    char padc = answer->payload[7];
    char eol = answer->payload[8];
}

int send_name(char argv[], char* current_SEQ){
    msg t;
    msg *answer;
    int i; 
    do{
        create_F_package(&t, argv, *current_SEQ);
        //printf("Voi trimite un F\n");
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            answer = receive_message_timeout(TIME);
            if (answer && answer->payload[2] == last_SEQ){
                //printf("Discard answer for F\n");
                i = 0;
                send_message(&t);
                continue;
            }
            else {
                if (answer){
                    last_SEQ = answer->payload[2];
                    //printf("Am primit un raspuns pt F\n");
                }
            }
            if (answer)
                break;
            //printf("Voi trimite un F\n");
            send_message(&t);
            if (i == 2)
                return 1; // Termina conexiunea
        }
/*        printf("RECEIVED ANSWER %c WITH NUMBER %c\n", answer->payload[2],
            answer->payload[3]);*/
        (*current_SEQ) += 2;
        (*current_SEQ) %= 64;
    } while (!is_acknowledgement(answer));
    return 0;
}

int send_file_with_name(char argv[], char *current_SEQ){
    if (send_name(argv, current_SEQ))
        return 1;
    int i;
    int file_handler = open(argv, O_RDONLY); // practic am deschis fisierul pentru citire
    int no_of_bytes_read = -1;
    msg t;
    msg *answer;
    void *buffer = (void *)malloc(MAXL);
    while (no_of_bytes_read){
        no_of_bytes_read = read (file_handler, buffer, MAXL);
        // seems ok
        if (no_of_bytes_read) {
            // pana cand se primeste ACK
            do{
                create_D_package(&t, buffer, no_of_bytes_read, *current_SEQ);
                // creez un pachet D
                //printf("Voi trimite un D\n");
                if (send_message(&t) < 0)
                    return 1;
                //printf("Am trimis D-ul %s\n", t.payload);
                //printf("In sender D %u length is", t.len);
                // de 3 ori voi astepta cate TIME pentru un raspuns
                for (i = 0 ; i < 3 ; i++){
                    answer = receive_message_timeout(TIME);
                    if (answer && answer->payload[2] == last_SEQ){
                        //printf("Discard answer for D\n");
                        printf("Am primit un ACK/NAK duplicat.\n");
                        i = 0;
                        send_message(&t);
                        continue;
                    }
                    // Daca am primit confirmarea pe care am primit-o si inainte
                    // atunci retrimit mesajul curent si re astept de 3 ori.
                    // Cand se intampla asta: 
                    // Sender trimite un mesaj, care nu ajunge la destinatie.
                    // In aceasta situatie receiverul trimite ultimul mesaj
                    // de confirmare. Necesitatea retrimiterii mesajului din sender
                    // este evidenta.
                    else{
                        if (answer){
                            last_SEQ = answer->payload[2];
                            //printf("Am primit un raspuns pt D\n");
                        }
                        //altfel, daca am primit un mesaj nou
                        // atunci modific numarul de secventa ultim
                    }
                    if (answer)
                        break;
                        // ies din for daca am primit un mesaj nou
/*                    printf("Trimit D\n");
                    printf("Voi trimite un D\n");*/
                    send_message(&t);
                    // Altfel retrimit mesajul
                    if (i == 2)
                        return 1; // Termina conexiunea
                }
                //printf("RECEIVED ANSWER %c WITH NUMBER %c\n", answer->payload[2],
                //    answer->payload[3]);
                (*current_SEQ) += 2;
                (*current_SEQ) %= 64;
            } while (!is_acknowledgement(answer));
            // IT SEEMS SO OK!
        }
    }
    do{
        create_Z_package(&t, *current_SEQ);
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            answer = receive_message_timeout(TIME);
            if (answer && answer->payload[2] == last_SEQ){
                //printf("Discard answer for Z\n");
                i = 0;
                send_message(&t);
                continue;
            }
            else
                if (answer){
                    last_SEQ = answer->payload[2];
                    //printf("Am primit un raspuns pt Z\n");
                }
            if (answer)
                break;
            //printf("Voi trimite un Z\n");
            send_message(&t);
            if (i == 2)
                return 1; // Termina conexiunea
        }
        //printf("RECEIVED ANSWER %c WITH NUMBER %c\n", answer->payload[2],
        //    answer->payload[3]);
        (*current_SEQ) += 2;
        (*current_SEQ) %= 64;
    } while (!is_acknowledgement(answer));
    close(file_handler);
    return 0;
}

int main(int argc, char** argv) {
    // argc = nr de argumente + 1 (pentru numele executabilului)
    int i;
    msg t;
    msg *answer;
    init(HOST, PORT);
    char current_SEQ = 0; // plec cu nr de secventa initial 0
    do{
        //printf("SENDER: am trimis un S\n");
        create_S_package(&t);
        t.len = strlen(t.payload);
        //printf("Voi trimite un S\n");
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            //printf("SENDER: astept raspuns pt S, i este %d\n",i);
            answer = receive_message_timeout(TIME * 3);
            if (answer && answer->payload[2] == last_SEQ){ 
                //printf("Discard answer for S\n");
                i = 0;
                send_message(&t);
                continue;
            }
            else {
                if (answer){
                    last_SEQ = answer->payload[2];
                    //printf("SENDER: a venit raspunsul pt S\n");
                }
            }
            if (answer)
                break;
            //printf("Voi trimite un S\n");
            send_message(&t);
            if (i == 2)
                return 1; // Termina conexiunea
        }
        // printf("RECEIVED ANSWER %c WITH NUMBER %c\n", answer->payload[2],
        //     answer->payload[3]);
        current_SEQ += 2;
        current_SEQ %= 64;
    } while (!is_acknowledgement(answer));
    for (i = 1 ; i < argc ; i++){
        if (send_file_with_name(argv[i], &current_SEQ))
            return 1; // daca intampin eroare, intorc eroare
    }
    do{
        create_B_package(&t, current_SEQ);
        if (send_message(&t) < 0)
            return 1;
        for (i = 0 ; i < 3 ; i++){
            answer = receive_message_timeout(TIME);
            if (answer && answer->payload[2] == last_SEQ){
                // printf("Discard answer for Z\n");
                i = 0;
                send_message(&t);
                continue;
            }
            else {
                if (answer){
                    last_SEQ = answer->payload[2];
                   // printf("AM PRIMIT LA B ACK CU NR.ORD. %c", answer->payload[2]);
                }
            }
            if (answer)
                break;
            //printf("Voi trimite un B\n");
            send_message(&t);
            if (i == 2)
                return 1; // Termina conexiunea
        }
/*        printf("RECEIVED ANSWER %c WITH NUMBER %c\n", answer->payload[2],
            answer->payload[3]);*/
        current_SEQ += 2;
        current_SEQ %= 64;
    } while (!is_acknowledgement(answer));
    return 0;
}