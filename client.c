/*
 * TEMA 2 PC
 * Petre Daniel Marian
 * 322 CD
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFLEN 256

void error(char *msg) {
	perror(msg);
	exit(0);
}

// 0 daca nu e conectat
// 1 daca e deja conectat
int logged_client = 0;

int main(int argc, char *argv[]) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char filename[14];

	sprintf(filename, "client-%d.log", getpid());

	FILE* log = fopen(filename, "w");

	fd_set read_fds;    //multimea de citire folosita in select()
	fd_set tmp_fds;    //multime folosita temporar
	int fdmax;     //valoare maxima a unui file descriptor din multimea read_fds

	char buffer[BUFLEN];
	if (argc < 3) {
		fprintf(stderr, "Usage %s server_address server_port\n", argv[0]);
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &serv_addr.sin_addr);

	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	while (1) {

		tmp_fds = read_fds;
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

		if (FD_ISSET(0, &tmp_fds)) {

			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			char* buffer_copy = (char*) malloc(sizeof(buffer));
			char* buff_cpy;
			strcpy(buffer_copy, buffer);

			char* command_sent = strtok(buffer_copy, " ");

			fprintf(log, "%s", buffer);

			if (strcmp(command_sent, "login") == 0 && logged_client == 1) {
				fprintf(log, "%s", "-2 : Sesiune deja deschisa\n");
				printf("%s", "-2 : Sesiune deja deschisa\n");

			} else if (strncmp(buffer, "logout", 6) == 0
					&& logged_client == 1) {
				logged_client = 0;
				fprintf(log, "%s", "ATM> Deconectare de la bancomat\n");
				printf("%s", "ATM> Deconectare de la bancomat\n");

				n = send(sockfd, buffer, strlen(buffer) + 1, 0);
				if (n < 0)
					error("ERROR writing to socket");

			} else if (strncmp(buffer, "logout", 6) == 0
					&& logged_client == 0) {
				fprintf(log, "%s", "-1 : Clientul nu este autentificat\n");
				printf("%s", "-1 : Clientul nu este autentificat\n");

			} else {

				n = send(sockfd, buffer, strlen(buffer) + 1, 0);
				if (n < 0)
					error("ERROR writing to socket");

				int recieve = recv(sockfd, buffer, BUFLEN, 0);
				if (recieve < 0)
					break;
				char* last_message = &buffer[strlen(buffer) - 6];
				if (strcmp(last_message, "logged") == 0) {

					logged_client = 1;
					buffer[strlen(buffer) - 6] = '\0';
					fprintf(log, "ATM> Welcome %s\n",buffer);
					printf("ATM> Welcome %s\n",buffer);

				} else if (strncmp(buffer, "-1", 2) == 0) {
					fprintf(log, "%s", "-1 : Clientul nu este autentificat\n");
					printf("%s", "-1 : Clientul nu este autentificat\n");

				} else if (strncmp(buffer, "-2", 2) == 0) {
					fprintf(log, "%s", "ATM> -2 : Sesiune deja deschisa\n");
					printf("%s", "ATM> -2 : Sesiune deja deschisa\n");

				} else if (strncmp(buffer, "-3", 2) == 0) {
					fprintf(log, "%s", "ATM> -3 : Pin gresit\n");
					printf("%s", "ATM> -3 : Pin gresit\n");

				} else if (strncmp(buffer, "-4", 2) == 0) {
					fprintf(log, "%s", "ATM> -4 : Numar card inexistent\n");
					printf("%s", "ATM> -4 : Numar card inexistent\n");

				} else if (strncmp(buffer, "-5", 2) == 0) {
					fprintf(log, "%s", "ATM> -5 : Card blocat\n");
					printf("%s", "ATM> -5 : Card blocat\n");

				} else if (strncmp(buffer, "-6", 2) == 0) {
					fprintf(log, "%s", "ATM> -6 : Operatie esuata\n");
					printf("%s", "ATM> -6 : Operatie esuata\n");

				} else if (strncmp(buffer, "-8", 2) == 0) {
					fprintf(log, "%s", "ATM> -8 : Fonduri insuficiente\n");
					printf("%s", "ATM> -8 : Fonduri insuficiente\n");

				} else if (strncmp(buffer, "-9", 2) == 0) {
					fprintf(log, "%s", "ATM> -9 : Suma nu este multiplu de 10\n");
					printf("%s", "ATM> -9 : Suma nu este multiplu de 10\n");

				} else if (strncmp(buffer, "quit", 4) == 0) {
					logged_client = 0;
					fclose(log);
					exit(0);
				} else {
					fprintf(log, "%s\n", buffer);
					printf("%s\n", buffer);
				}
			}
		}
	}

	return 0;
}
