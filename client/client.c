#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>		
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>





int sockstate;

struct command {
	char code[5];
        char msg[256];
};



int client_command(char* buf, int size, struct command *ccommand)
{
	
        printf("client> ");	
	fflush(stdout);
        memset(ccommand->code, 0, sizeof(ccommand->code));
	memset(ccommand->msg, 0, sizeof(ccommand->msg));
        char *cl=NULL ;
	memset(buf, 0, size);
        if ( fgets(buf, size, stdin) != NULL ) {
		cl = strchr(buf, '\n');
		if (cl) *cl = '\0';
	}
	char *message = NULL;
	message = strtok (buf," ");
	message = strtok (NULL, " ");
        if (message != NULL){
		strncpy(ccommand->msg, message, strlen(message));
	}

	if (strcmp(buf, "ls") == 0) {
		strcpy(ccommand->code, "LIST");
	}
	else if (strcmp(buf, "get") == 0) {
		strcpy(ccommand->code, "RETR");
	}
	else if (strcmp(buf, "logout") == 0||strcmp(buf, "exit") == 0) {
		strcpy(ccommand->code, "QUIT");
	}
        else if (strcmp(buf, "put") == 0) {
        strcpy(ccommand->code, "PUT");
        }
        else if (strcmp(buf, "cd") == 0) {
        strcpy(ccommand->code, "CD");
        }
        else if (strcmp(buf, "date") == 0) {
        strcpy(ccommand->code, "DATE");
        }
        else if (strcmp(buf, "whoami") == 0) {
        strcpy(ccommand->code, "WHO");
        }
        else if (strcmp(buf, "w") == 0) {
        strcpy(ccommand->code, "W");
        }
        else if (strcmp(buf, "ping") == 0) {
        strcpy(ccommand->code, "PING");
        }
        else if (strcmp(buf, "login") == 0) {
        strcpy(ccommand->code, "USER");
        }
        else if (strcmp(buf, "pass") == 0) {
        strcpy(ccommand->code, "PASS");
        }

    else {  
	return -1;
    }
        memset(buf, 0, 512);
	strcpy(buf, ccommand->code);
        int len=strlen(ccommand->msg);
	if (message != NULL) {
		strcat(buf, " ");
		strncat(buf, ccommand->msg, len);
	}

	return 0;
}


int client_get(int sockdata, int sockstate, char* msg)
{
    char buf[512];
    int reply;
    FILE* fd = fopen(msg, "w");
    while ((reply = recv(sockdata, buf, 512, 0)) > 0) {
        fwrite(buf, 1, reply, fd);
    }
    fclose(fd);
    return 0;
}


int acceptsocket(int sock_num)
{
	int sockfd;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	sockfd = accept(sock_num, (struct sockaddr *) &client_addr, &len);

	if (sockfd < 0) {
		perror("can not accept");
		return -1;
	}
	return sockfd;
}





int client_connection(int sock_num)
{
        int sockfd;
	struct sockaddr_in sock_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("can not create socket");
		return -1;
	}
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(30020);
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
		close(sockfd);
		perror("can not bind socket");
		return -1;
	}

	if (listen(sockfd, 5) < 0) {
		close(sockfd);
		perror("can not listen");
		return -1;
	}
        int sock_listen=sockfd;
	int ack = 1;
	if ((send(sock_num, (char*) &ack, sizeof(ack), 0)) < 0) {
		printf("client can not write ack:%d\n", errno);
		exit(1);
	}

	int sock_cnnt = acceptsocket(sock_listen);
	close(sock_listen);
	return sock_cnnt;
}





int client_login(char* user, char* pass)
{
	struct command cmd;
	strcpy(cmd.code, "USER");
	strcpy(cmd.msg, user);
        char userbuffer[500];
	int userrc;
        sprintf(userbuffer, "%s %s", cmd.code, cmd.msg);
	userrc = send(sockstate, userbuffer, (int)strlen(userbuffer), 0);
	if (userrc < 0) {
		perror("Can not send command to server");
		return -1;
	}
	int wait_pass;
	recv(sockstate, &wait_pass, sizeof wait_pass, 0);
	fflush(stdout);
	strcpy(cmd.code, "PASS");
	strcpy(cmd.msg, pass);
	char passbuffer[500];
	int passrc;
        sprintf(passbuffer, "%s %s", cmd.code, cmd.msg);
	passrc = send(sockstate, passbuffer, (int)strlen(passbuffer), 0);
	if (passrc < 0) {
		perror("Can not send command to server");
		return -1;
	}
	int returncode = 0;
        if (recv(sockstate, &returncode, sizeof returncode, 0) < 0) {
		perror("Can not get message\n");
		return -1;
	}
	returncode=ntohl(returncode);
	switch (returncode) {
		case 430:
			printf("Invalid username or password.\n");
			exit(0);
		case 230:
			printf("Successful login !\n");
                        return 1;
			break;
		default:
			perror("Can not get message");
			exit(1);
			break;
	}
    return 0;
}



int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("please input: ./client hostname port\n");
		exit(0);
	}
        printf("we have 2 modes ,if you want to choose default mode ,just login, if you want to use auto mode ,please login and then exit and use : ./client localhost 34567 <autonomous to run client\n");
        char *host = argv[1];
	char *port = argv[2];
	
        struct command cmd;
	struct addrinfo hints; 
        int sockdata, retcode;
	char buffer[500];
	int flag = 0;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *address, *p;
	if (getaddrinfo(host, port, &hints, &address)!= 0) {
		printf("can not get address information");
		exit(1);
	}
	for (p = address; p != NULL; p = p->ai_next) {
		sockstate = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sockstate < 0)
			continue;

		if(connect(sockstate, address->ai_addr, address->ai_addrlen)==0) {
			break;
		} else {
			perror("connecting stream socket");
			exit(1);
		}
		close(sockstate);
	}
	freeaddrinfo(p);
	printf("Connected to %s.\n", host);
         if (recv(sockstate, &retcode, sizeof retcode, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}
        retcode=ntohl(retcode);
        switch (retcode) {
		case 220:
			printf("Welcome, successful connection.\n");
			break;
		case 221:
			printf("Goodbye! Hope to see you again. \n");
			break;
		case 226:
			printf("Closing data connection. Requested file action successful.\n");
			break;
		case 550:
			printf("Requested action not taken. File unavailable.\n");
			break;}


    char user[100];
    memset(user, 0, sizeof user);

	while (1) { 

		if (client_command(buffer, sizeof buffer, &cmd) < 0) {
			printf("Invalid command\n");
			continue;	
		}
        if (strcmp(cmd.code, "USER") == 0) {
            strcpy(user, cmd.msg);
            continue;
        }
        else if (strcmp(cmd.code, "PASS") == 0) {
            if (strlen(user) == 0) {
                perror("Submit username before password.\n");
                continue;
            }
            flag = client_login(user, cmd.msg);
            continue;
        }
        else if(strcmp(cmd.code, "WHO") == 0) {
            if (!flag) {
                printf("Login is required.\n");
                continue;
            }
            printf(user);
            printf("\n");
            continue;
        }
	
		if (send(sockstate, buffer, (int)strlen(buffer), 0) < 0 ) {
			close(sockstate);
			exit(1);
		}
                 if (recv(sockstate, &retcode, sizeof retcode, 0) < 0) {
		perror("can not read message from server\n");
		return -1;
	        }
                retcode=ntohl(retcode);
		if (retcode == 221) {
			printf(" Goodbye!Hope to see you again.\n");
			break;
		}

		if (retcode == 502) {
			
			printf("%d Invalid command.\n", retcode);
		} else {
			
			if ((sockdata = client_connection(sockstate)) < 0) {
				perror("Error opening socket for data connection");
				exit(1);
			}


			if (strcmp(cmd.code, "DATE") == 0 || strcmp(cmd.code, "LIST") == 0 ||
                strcmp(cmd.code, "PING") == 0 || strcmp(cmd.code, "W") == 0 ||
                strcmp(cmd.code, "RETR") == 0||strcmp(cmd.code, "CD") == 0) {
                if (!flag) {
                    printf("Login required");
                    continue;
                }
				
                            size_t numbuffer;			
	                    char lsbuf[500];			
	                    int lsflag = 0;
	                   if (recv(sockstate, &lsflag, sizeof lsflag, 0) < 0) {
		             perror("client: error reading message from server\n");
		              return -1;
	                       }

	                    memset(lsbuf, 0, sizeof(lsbuf));
	                    while ((numbuffer = recv(sockdata, lsbuf, 500, 0)) > 0) {
                            printf("%s", lsbuf);
		           memset(lsbuf, 0, sizeof(lsbuf));
	                   }

	
	             if (recv(sockstate, &lsflag, sizeof lsflag, 0) < 0) {
		          perror("client: error reading message from server\n");
		     return -1;
	                }
                                
			}}

			close(sockdata);
		}
	
	close(sockstate);
        free(port);
        free(host);
        return 0;
}
