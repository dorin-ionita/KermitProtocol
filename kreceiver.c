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
	msg *r, t;
	init(HOST, PORT);
	// am setat conexiunea
	char SEQ = 1;
	// numarul curent de secventa
	int file_handler = -1;
	// descriptorul de fisier in care voi scrie (se va modifica)
	char file_name[500];
	// numele fisierului in care voi scrie
	char prefix[] = "recv_";
	// prefixul pentru numele fisierului
	char *full_name;
	// numele complet al fisierului in care scriu
	char last_mesage_SEQ = -1;
	// ultimul SEQ primit
	while (1) {
		// cicleaza la infinit
		while (1) {
			r = receive_message_timeout(TIME);
			if (r) {
				printf("r is %s\n", r->payload);
				printf("typ is %c\n", r->payload[3]);
				break;
			} else {
				send_message(&t);
				continue;
			}
		}
		// daca am primit un mesaj in TIME trec mai departe
		// altfel ..altfe ar trebui sa retrimit ultimul pachet
		// si sa trec la urmatoarea interatie din while
		printf("LAST_MESSAGE_SEQ %c \n", last_mesage_SEQ);
		printf("CURRENT_RECEIVED_SEQ %c", r->payload[2]);
		if (!check_crc_is_correct(r)) {
			printf("CRC IS INCORRECT %d\n",
			       check_crc_is_correct(r));
			create_N_package(&t, SEQ);
			//was_ack = 0;
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			continue;
		}
		/* daca suma de control nu este corecta
		 * inseamna ca mesajul a fost corupt, deci trimit un mesaj
		 * de tip NAK si incrementez numarul de secventa 
		 */
		switch (r->payload[3]) {
			/* Ma uit la ce fel de mesaj am */
		case 'S':
			printf("S package\n");
			create_YS_package(&t);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
			/* daca am mesaj de tip S atunci trimit un pachet YS
			si incrementez numarul de secventa */
		case 'F':
			prefix[0] = '\0';
			strcat(prefix, "recv_");
			/* daca am mesaj tip F atunci recreez prefixul
			(pentru ca ulterior se va modifica)
			si trimit un pachet Y */
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			printf("VOI DESCHIDE FISIERUL DE SCRIS\n");
			memcpy(file_name, &(r->payload[4]), r->payload[1] - 5);
			printf("PAYLOADUL ESTE %s", r->payload);
			file_name[r->payload[1] - 4] = '\0';
			printf("\n%s file name is", file_name);
			full_name = strcat(prefix, file_name);
			printf("\n%s full name\n", full_name);
			file_handler = open(full_name, O_WRONLY);
			/* Apoi recreez numele noului fisier si il deschid
			*/
			SEQ += 2;
			SEQ %= 64;
			/* in cele din urma incrementez numarul de secventa */
			break;
		case 'D':
			printf("D package\n");
			write(file_handler, &(t.payload[4]), t.payload[1] - 5);
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
			/* Daca e tip D scriu datele, trimit Y si apoi 
			 * incrementez numarul de secventa */
		case 'Z':
			printf("Z package\n");
			close(file_handler);
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
			/* Daca este Z inchid fisierul si incrementez numarul
			 * de secventa, trimit si pachet Y */
		case 'B':
			printf("B package\n");
			create_Y_package(&t, SEQ);
			send_message(&t);
			printf("SENT MESSAGE %c WITH NUMBER %C\n",
				t.payload[2],t.payload[3]);
			SEQ += 2;
			SEQ %= 64;
			break;
			/* Daca e B trimit Y, incrementez nr de secventa*/
		}
		if (r->payload[3] == 'B')
			break;
		/* Daca e b inchid whileul si termin, deci, executia programului */
	}

	return 0;
}
