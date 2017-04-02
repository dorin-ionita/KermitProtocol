#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

#define MAXL 250
#define TIME 5000
#define NPAD 0
#define PADC 0
#define SOH 0x01
#define EOL 0x0D
#define ZERO 0x00

void create_YS_package(msg * t)
{
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
	t->payload[10] = ZERO;	//QBIN
	t->payload[11] = ZERO;	//CHKT
	t->payload[12] = ZERO;	//RPT
	t->payload[13] = ZERO;	// CAPA
	t->payload[14] = ZERO;	//R1
	unsigned short crc = crc16_ccitt(t->payload, 15);
	memcpy(&(t->payload[15]), &crc, 2);
	t->payload[17] = EOL;	// MARK
	t->payload[18] = '\0';
	t->len = strlen(t->payload);
}

void create_Y_package(msg * t, char current_SEQ)
{
	t->payload[0] = SOH;
	t->payload[1] = 5;
	t->payload[2] = current_SEQ;
	t->payload[3] = 'Y';
	unsigned short crc = crc16_ccitt(t->payload, 4);
	memcpy(&(t->payload[4]), &crc, 2);
	t->payload[6] = EOL;
	t->payload[7] = '\0';
	t->len = strlen(t->payload);
}

void create_N_package(msg * t, char current_SEQ)
{
	t->payload[0] = SOH;
	t->payload[1] = 5;
	t->payload[2] = current_SEQ;
	t->payload[3] = 'N';
	unsigned short crc = crc16_ccitt(t->payload, 4);
	memcpy(&(t->payload[4]), &crc, 2);
	t->payload[6] = EOL;
	t->payload[7] = '\0';
	t->len = strlen(t->payload);
}

int check_crc_is_correct(msg * t)
{
	char computed_string[50];
	char crc_from_structure_string[50];

	sprintf(crc_from_structure_string,
		"%hu", t->payload[t->len -3]);
	unsigned short computed_crc =
	    crc16_ccitt(t->payload, t->len - 3);
	char computer_crc_in_string[15];
	memcpy(&(computer_crc_in_string[0]), &computed_crc, 2);
	sprintf(computed_string, "%hu", computer_crc_in_string[0]);
	return (!strcmp(computed_string, crc_from_structure_string));
}

int is_the_right_message(msg *r, char SEQ){
	return r->payload[2] == SEQ - 1;
}

int main(int argc, char **argv)
{
	msg *r, t;
	init(HOST, PORT);
	char SEQ = 1;
	char file_name[500];
	char prefix[] = "recv_";
	char last_mesage_SEQ = -1;
	FILE* file_descriptor;
	int can_send_anymore;
	char un_rahat[5000];
	while (1) {
		can_send_anymore = 3;
		while (can_send_anymore) {
			r = receive_message_timeout(TIME);
			if (r)
				break;
			else{
				can_send_anymore--;
			}
		}
		// Daca nu primesc un mesaj in timp util trimit ultimul
		// mesaj ACK/NAK
		if (!can_send_anymore)
			return 1;
		// Daca am asteptat de 3 ori inchei conexiunea
		printf("Am primit payloadul\n%s\n | de tip %c | avand last_seq %c | si current_seq %c | iar CRCul este %d\n",
				r->payload, r->payload[3], last_mesage_SEQ, r->payload[2],
			 	check_crc_is_correct(r));
		if (!check_crc_is_correct(r)) {
			printf("CRC IS INCORRENTC\n");
			create_N_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			continue;
		} else if (last_mesage_SEQ == r->payload[2]){
			printf("Am primit un duplicat, astept alt mesaj\n");
			if (check_crc_is_correct(r)){
				printf("And it has right CRC\n");
				send_message(&t);
				continue;
			}
			printf("And it doesn't have right CRC\n");
		}//???
 		else {
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			last_mesage_SEQ = r->payload[2];
		}
		// Daca mesajul nu este corect atunci trimit un NAK
		// si astept primirea altui mesaj.
		// Daca acest mesaj pentru care am trimis NAK avea bitul
		// de secventa corupt atunci eu practic am trimis NAK
		// pentru un mesaj duplicat. Serverul oare cum se va comporta?
		// PROBLEMA ESTE, DECI, MAI SUS!*/
/*		can_send_anymore = 3;
		while (can_send_anymore){
			r = receive_message_timeout(TIME);
			if (r){
				break; // <=> mergi mai departe
			} else{
				can_send_anymore--;
				if (SEQ != 1)
					send_message(&t);
			}
		}
		if (!check_crc_is_correct(r)){
			create_N_package(&t, SEQ);
			send_message(&t);
			SEQ += 2;
			SEQ %= 64;
			continue;
		}
		if (check_crc_is_correct(r) && r->payload[2] == last_mesage_SEQ){
			send_message(&t);
			continue;
		}
		printf("\n Am acceptat payloadul %s\n", r->payload);
		last_mesage_SEQ = r->payload[2];
		create_Y_package(&t, SEQ);
		send_message(&t);
		SEQ += 2;
		SEQ %= 64;*/
		switch (r->payload[3]) {
		case 'S':
			break;
		case 'F':
			strcpy(prefix, "recv_");
			//printf("\n%u nenorocitul de lungime este\n",((unsigned int) r->payload[1]) - 5);
			memcpy(file_name, &(r->payload[4]), r->payload[1] - 5);
			file_name[r->payload[1] - 5] = '\0';
			strcat(prefix, file_name);
			file_descriptor = fopen(prefix,"wb");
			break;
		case 'D':
			fwrite(&(r->payload[4]), r->len - 7, 1, file_descriptor);
			break;
		case 'Z':
			fclose(file_descriptor);
			break;
		case 'B':
			break;
		}
		if (r->payload[3] == 'B'){
/*			printf("voi iesi din bucla infinita de primire\n");*/
			break;
		}
	}

	return 0;
}
