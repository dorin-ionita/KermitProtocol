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
	int start_position_of_crc = t->payload[1] - 1;
	/* t->payload[1] +1 este pozitia ultimul caracter in string
	   (pentru ca LEN incepe sa se numere de la pozitia 2, cum ar fi)
	   deci al2-lea caracter din CRC este la t->payload[1],
	   deci primul caracter din CRC este la t->payload[1] - 1 */
	char computed_string[50];
	char crc_from_structure_string[50];

	sprintf(crc_from_structure_string,
		"%hu", t->payload[t->len -3]);
	unsigned short computed_crc =
	    crc16_ccitt(t->payload, t->len - 3);
	char computer_crc_in_string[15];
	memcpy(&(computer_crc_in_string[0]), &computed_crc, 2);
	sprintf(computed_string, "%hu", computer_crc_in_string[0]);
	/*printf("\nSTRINGURILE DE CRC SUNT:\n%s\n%s\n", computed_string,
	       crc_from_structure_string);*/
return (!strcmp(computed_string, crc_from_structure_string));
/*	int start_position_of_crc = t->len - 3;
	char computed_string[50];
	char crc_from_structure_string[50];
	sprintf(crc_from_structure_string,
		"%hu", t->payload[start_position_of_crc]);
	unsigned short computed_crc =
	    crc16_ccitt(t->payload, start_position_of_crc);
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
	while (1) {
		can_send_anymore = 3;
		while (can_send_anymore) {
			r = receive_message_timeout(TIME);
			if (r)
				break;
			else{
				send_message(&t);
				printf("TIMEOUT\n");
				can_send_anymore--;
			}
		}
		if (!can_send_anymore)
			break;
		printf("Am primit payloadul\n%s\n | de tip %c | avand last_seq %c | si current_seq %c | iar CRCul este %d\n",
				r->payload, r->payload[3], last_mesage_SEQ, r->payload[2],
			 	check_crc_is_correct(r));
/*		if (last_mesage_SEQ == r->payload[2]){
			printf("Am primit un duplicat, astept alt mesaj\n");
			continue;
		}*/
		if (!check_crc_is_correct(r)) {
			printf("CRC IS INCORRENTC\n");
			create_N_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			continue;
		} else {
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			last_mesage_SEQ = r->payload[2];
		}
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
			printf("AM AJUNS AICI\n");
			printf("\n%u length is", r->len - 6);
			//printf("\n%u nenorocitul de lungime este\n",r->payload[1] - 5);
			//strncpy(un_rahat, &(r->payload[4]),r->payload[1] - 5);
			printf("AM AJUNS SI AICI\n");
			//un_rahat[r->payload[1] - 5] = '\0';
			printf("\n### %s voi scrie \n", un_rahat);
			fwrite(&(r->payload[4]), r->len - 7, 1, file_descriptor);
			break;
		case 'Z':
			fclose(file_descriptor);
			break;
		case 'B':
			break;
		}
		if (r->payload[3] == 'B'){
			printf("voi iesi din bucla infinita de primire\n");
			break;
		}
	}

	return 0;
}
