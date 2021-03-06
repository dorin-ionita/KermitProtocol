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
	unsigned short crc_from_file;
	char aux[2];
/*	memcpy(&crc_from_file,
		&(t->payload[15]), 2);*/
	if (t->payload[3] == 'S'){
		unsigned short computed_crc = crc16_ccitt(t->payload, 15);
		aux[0] = computed_crc & 0xff;
		aux[1] = (computed_crc >> 8) & 0xff;
/*		printf("computer crc is%hu\n", computed_crc);
		printf("crc from file is %hu\n", crc_from_file);*/
		if (aux[0] == t->payload[15] && aux[1] == t->payload[16] ){
			printf("good to go");
			return 1;
		}
		return 0;
	} else {
		unsigned short computed_crc = crc16_ccitt(t->payload, t->len  - 3);
		aux[0] = computed_crc & 0xff;
		aux[1] = (computed_crc >> 8) & 0xff;
		if (aux[0] == t->payload[t->len - 3] &&
			aux[1] == t->payload[t->len - 2]){
			printf("good to go");
			return 1;
		}
		return 0;
	}

/*	sprintf(crc_from_structure_string,
		"%hu", t->payload[t->len -3]);//CRCul din fisier
	memcpy(&(crc_from_structure_string), &(t->payload[t->len - 3]), 
		2);
	printf("%hu CRC IS \n", crc_from_structure_string);
	unsigned short computed_crc =
	    crc16_ccitt(t->payload, t->len - 3);
	char computer_crc_in_string[15];
	memcpy(&(computer_crc_in_string[0]), &computed_crc, 2);
	sprintf(computed_string, "%hu", computer_crc_in_string[0]);
	return (!strcmp(computed_string, crc_from_structure_string));*/
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
	int intra_de_ori = 0;
	while (1) {
		int intra_de_ori = 0;
		can_send_anymore = 3;
		while (can_send_anymore) {
			r = receive_message_timeout(TIME);
			if (r)
				break;
			else{
				can_send_anymore--;
			}
		}
		if (!can_send_anymore)
			return 1;
		printf("Am primit payloadul\n%s\n | de tip %c | avand last_seq %c | si current_seq %c | iar CRCul este %d\n",
				r->payload, r->payload[3], last_mesage_SEQ, r->payload[2],
			 	check_crc_is_correct(r));
		if (!check_crc_is_correct(r)) {
			//return 1;
			printf("TRIMIT NAK");
			int intra_de_ori = 1;
			create_N_package(&t, SEQ);
			send_message(&t);
			SEQ += 2;
			SEQ %= 64;
			continue;
		} else { if (last_mesage_SEQ == r->payload[2]){
				send_message(&t);
				continue;
		} else {
			if (intra_de_ori)
				printf("Am intrat de 2 ori\n");
			create_Y_package(&t, SEQ);
			send_message(&t);
			SEQ += 2;
			SEQ %= 64;
			last_mesage_SEQ = r->payload[2];
		}}
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
			fwrite(&(r->payload[4]), 1, r->len - 7, file_descriptor);
			// ESTE OK, PENTRU CA SCRIU LUNGIMEA INTREGULUI MESAJ CARE E 
			// 4+buffer_length+3 - 7 = buffer_length
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
