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
	// CRC pe primii 9 bytes -> nu e nevoie sa verific in server, though
	t->payload[17] = EOL;	// MARK
	t->payload[18] = '\0';
	t->len = strlen(t->payload);
}

// CHECK: pare ok

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

// CHECK: pare ok

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

// CHECK: pare ok

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
		"%hu", t->payload[start_position_of_crc]);
	unsigned short computed_crc =
	    crc16_ccitt(t->payload, start_position_of_crc);
	char computer_crc_in_string[15];
	memcpy(&(computer_crc_in_string[0]), &computed_crc, 2);
	sprintf(computed_string, "%hu", computer_crc_in_string[0]);
	printf("\nSTRINGURILE DE CRC SUNT:\n%s\n%s\n", computed_string,
	       crc_from_structure_string);
	return (!strcmp(computed_string, crc_from_structure_string));
}
// CHECK: pare ok

int is_the_right_message(msg *r, char SEQ){
	return r->payload[2] == SEQ - 1;
}

int abs_val(char x, char y){
	return x - y < 0 ? y -x : x - y;
}

int main(int argc, char **argv)
{
	printf("FIRST LINE OF RECEIVER\n");
	int i;
	msg *r, t;
	init(HOST, PORT);
	char SEQ = 1;
	int file_handler = -1;
	char file_name[500];
	int allready_received_S = 0;
	char prefix[] = "recv_";
	char *full_name;
	char last_mesage_SEQ = -1;
	int was_ack = 0;
	while (1) {
		printf("FIRST LINE IN WHILE\n");
		while (1) {
			r = receive_message_timeout(TIME);
			if (r) {
				printf("r is %s\n", r->payload);
				printf("typ is %c\n", r->payload[3]);
				break;
			}
		}
		printf("2  %c \n", r->payload[2]);
		// daca mesajul nu e corupt atunci
		// zi ca ai primit si e corupt si sa dea din nou
		printf("LAST_MESSAGE_SEQ %c", last_mesage_SEQ);
		printf("CURRENT_RECEIVED_SEQ %c", r->payload[2]);
/*		if (abs_val(r->payload[2], last_mesage_SEQ) > 2){
			printf("missed a pack");
			continue;
		}*/
		if (last_mesage_SEQ == r->payload[2]){
			printf("IGNORE\n");
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			continue;
		}
		if (!check_crc_is_correct(r)) {
			printf("CRC IS INCORRECT %d\n",
			       check_crc_is_correct(r));
			create_N_package(&t, SEQ);
			was_ack = 0;
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			continue;
		}
		last_mesage_SEQ = r->payload[2];
		switch (r->payload[3]) {
		case 'S':
			printf("S package\n");
			allready_received_S = 1;
			was_ack = 1;
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
		case 'F':
			printf("VOI DESCHIDE FISIERUL DE SCRIS\n");
			memcpy(file_name, &(t.payload[4]), t.payload[1] - 5);
			file_name[t.payload[1] - 5] = '\0';
			full_name = strcpy(prefix, file_name);
			file_handler = open(full_name, O_APPEND);
			was_ack = 1;
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
		case 'D':
			printf("D package\n");
			write(file_handler, &(t.payload[4]), t.payload[1] - 5);
			was_ack = 1;
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
		case 'Z':
			printf("Z package\n");
			close(file_handler);
			was_ack = 1;
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
		case 'B':
			printf("B package\n");
			was_ack = 1;
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
		}
		if (r->payload[3] == 'B')
			break;
	}

	return 0;
}
