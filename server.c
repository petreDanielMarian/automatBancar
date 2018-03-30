//Tema 2 PC

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

#define MAX_CLIENTS	5
#define BUFLEN 256

struct user {
	char first_name[12];
	char last_name[12];
	int card_id;
	int card_pin;
	char secret_password[16];
	double balance;
	// 0 = nelogat
	// 1 = logat
	// 2 = blocat
	int condition;
	int tries;
	int socket;
};

void error(char *msg) {
	perror(msg);
	exit(0);
}

struct user* users;

void readFromFile(char* filename) {

	FILE *file;

	file = fopen(filename, "r");

	int number_of_users, i;
	char line[100];

	fgets(line, 4, file);
	char* word = strtok(line, " ");

	number_of_users = atoi(word);

	users = malloc(number_of_users * sizeof(struct user));

	for (i = 0; i < number_of_users; i++) {
		fgets(line, sizeof(line), file);

		word = strtok(line, " ");
		strncpy(users[i].first_name, word, 12);

		word = strtok(NULL, " ");
		strncpy(users[i].last_name, word, 12);

		word = strtok(NULL, " ");
		users[i].card_id = atoi(word);

		word = strtok(NULL, " ");
		users[i].card_pin = atoi(word);

		word = strtok(NULL, " ");
		strncpy(users[i].secret_password, word, 16);

		word = strtok(NULL, " ");
		users[i].balance = atof(word);

		users[i].condition = 0;
		users[i].tries = 0;
		users[i].socket = 0;

	}
	fclose(file);
	return;
}

int login(int c_number, int c_pin, int n, int socket) {
	int i;

	for (i = 0; i < n; i++) {
		if (users[i].card_id == c_number) {
			if (users[i].condition == 1) {
				return -2;
			} else if (users[i].card_pin == c_pin && users[i].condition == 0) {
				users[i].condition = 1;
				users[i].socket = socket;
				users[i].tries = 0;
				return i;
			} else {
				if (users[i].tries == 2) {
					users[i].condition = 2;
					return -5;
				} else {
					users[i].tries++;
					return -3;
				}
			}
		}
	}
	return -4;
}

int getClient(int socket, int n) {
	int i;
	for (i = 0; i < n; i++) {
		if (users[i].socket == socket) {
			return i;
		}
	}
	return -1;
}

int getMoney(int i, int money, int n) {
	if (money % 10 == 0) {
		if (users[getClient(i, n)].balance >= (double) money) {
			users[getClient(i, n)].balance = users[getClient(i, n)].balance
					- (double) money;
		} else {
			return -8;
		}
	} else {
		return -9;
	}
	return 1;
}

int main(int argc, char *argv[]) {
	FILE *file;
	file = fopen(argv[2], "r");
	int number_of_users;
	char line[100];
	char* number;
	int money;
	char buffer_temp;

	fgets(line, 4, file);
	char* word = strtok(line, " ");

	number_of_users = atoi(word);
	fclose(file);

	readFromFile(argv[2]);

	int sockfd, newsockfd, portno, clilen;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, j;
	int s;

	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;	//multime folosita temporar
	int fdmax;		//valoare maxima file descriptor din multimea read_fds

	if (argc < 3) {
		fprintf(stderr, "Usage : %s port\n", argv[0]);
		exit(1);
	}

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	char* command;
	portno = atoi(argv[1]);

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr))
			< 0)
		error("ERROR on binding");

	listen(sockfd, MAX_CLIENTS);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds;

		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			error("ERROR in select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if (i == sockfd) {
					// a venit ceva pe socketul inactiv = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd,
							(struct sockaddr *) &cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) {
							fdmax = newsockfd;
						}
					}
					printf(
							"Noua conexiune de la %s, port %d, socket_client %d\n",
							inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port), newsockfd);
				} else {

					memset(buffer, 0, BUFLEN);
					if ((n = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
						if (n == 0) {
							//conexiunea s-a inchis
							printf("selectserver: socket %d hung up\n", i);
						} else {
							error("ERROR in recv");
						}
						close(i);
						FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
					} else {
						// am folosit strncmp pentru a evita cazul in care se citeste si \n
						// in buffer
						if (strncmp(buffer, "quit", 4) == 0) {
							users[getClient(i, number_of_users)].condition = 0;
							users[getClient(i, number_of_users)].socket = 0;
							send(i, "quit", strlen("quit") + 1, 0);
						} else if ((strncmp(buffer, "login", 5) != 0)
								&& getClient(i, number_of_users) == -1) {
							send(i, "-1", strlen("-1") + 1, 0);

						} else if (strncmp(buffer, "login", 5) == 0) {
							command = strtok(buffer, " ");

							command = strtok(NULL, " ");
							int c_number = atoi(command);

							command = strtok(NULL, " ");
							int c_pin = atoi(command);

							int l = login(c_number, c_pin, number_of_users, i);

							if (l == -2) {
								send(i, "-2", strlen("-2") + 1, 0);
							} else if (l == -3) {
								send(i, "-3", strlen("-3") + 1, 0);
							} else if (l == -4) {
								send(i, "-4", strlen("-4") + 1, 0);
							} else if (l == -5) {
								send(i, "-5", strlen("-5") + 1, 0);
							} else {

								char* firstname =
										(char*) malloc(
												sizeof(users[getClient(i,
														number_of_users)].first_name
														+ sizeof(users[getClient(
																i,
																number_of_users)].last_name
																+ sizeof("logged")
																+ 1)));
								sprintf(firstname, "%s ", users[l].first_name);
								strcat(firstname, users[l].last_name);
								strcat(firstname, "logged");
								send(i, firstname, strlen(firstname) + 1, 0);

							}

						} else if ((strncmp(buffer, "logout", 6) == 0)
								&& (getClient(i, number_of_users) != -1)) {

							users[getClient(i, number_of_users)].condition = 0;
							users[getClient(i, number_of_users)].socket = 0;
							send(i, "", strlen(""), 0);

						} else if (strncmp(buffer, "listsold", 8) == 0) {
							number = malloc(sizeof("ATM> ") + 8);
							sprintf(number, "ATM> %.2f",
									users[getClient(i, number_of_users)].balance);
							send(i, number, strlen(number) + 1, 0);

						} else if (strncmp(buffer, "getmoney", 8) == 0) {
							command = strtok(buffer, " ");

							command = strtok(NULL, " ");
							money = atoi(command);
							int g = getMoney(i, money, number_of_users);

							if (g == -8) {
								send(i, "-8", strlen("-8") + 1, 0);
							} else if (g == -9) {
								send(i, "-9", strlen("-9") + 1, 0);
							} else if (g == 1) {
								number =
										malloc(
												sizeof("ATM> Suma  a fost retrasa cu succes")
														+ 8);
								sprintf(number,
										"ATM> Suma %d retrasa cu succes",
										money);
								send(i, number, strlen(number) + 1, 0);
							}

						} else if (strncmp(buffer, "putmoney", 8) == 0) {
							command = strtok(buffer, " ");

							command = strtok(NULL, " ");
							double dineros = atof(command);

							users[getClient(i, number_of_users)].balance +=
									dineros;
							send(i, "ATM> Suma depusa cu succes",
									strlen("ATM> Suma depusa cu succes") + 1,
									0);

						} else {
							send(i, "-6", strlen("-6") + 1, 0);

						}
					}
				}
			}
		}
	}

	close(sockfd);
	return 0;
}
