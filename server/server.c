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



int main(int argc, char *argv[])
{
	
	if (argc != 1) {
		printf("please input: ./server  \n");
		exit(0);
	}
        int sockstate, pid;
        int sockfd;
	struct sockaddr_in sock_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("can not create socket");
		return -1;
	}
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(34567);
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
        if (sock_listen < 0 ) {
		perror("Can not create socket");
		exit(1);
	}
	while(1) {
                 int sockfd;
	         struct sockaddr_in client_addr;
	         socklen_t len = sizeof(client_addr);
	         sockfd = accept(sock_listen, (struct sockaddr *) &client_addr, &len);

	          if (sockfd < 0) {
		   perror("can not accept");
		  return -1;
	         }
	         sockstate=sockfd;
                
		if (sockstate	< 0 )
			break;
                pid =fork();
		if (pid == 0) {
			close(sock_listen);
			server_handler(sockstate);
			close(sockstate);
			exit(0);
		}
                if (pid < 0) {
			perror("Can not create fork process");}

		close(sockstate);
	}

	close(sock_listen);

	return 0;
}


int server_get(int sockstate, int datasock, char* filename)
{
    char buf[256];
    char cnt[128];
    memset(buf, 0, sizeof buf);
    sprintf(buf, "wc -c %s > tmp.txt", filename);
    if(system(buf) < 0) {
        exit(0);
    }
    FILE* fd = fopen("tmp.txt", "r");
    fread(buf, 1, 256, fd);
    sscanf(buf, "%s %s", cnt, buf);
    fclose(fd);
    int port = rand() % 1000 + 5000;
    sprintf(buf, "echo get port: %d size: %s > tmp.txt", port, cnt);
    system(buf);
    sprintf(buf, "nc -v -l %d < %s &", port, filename);
    system(buf);
    char data[500];
    size_t num_read;
    fd = fopen("tmp.txt", "r");
	if (!fd) {
		exit(1);
	}
	fseek(fd, SEEK_SET, 0);
	int startrc = htonl(1);
	if (send(sockstate, &startrc, sizeof startrc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}	 
	memset(data, 0, 500);
	while ((num_read = fread(data, 1, 500, fd)) > 0) {
		if (send(datasock, data, num_read, 0) < 0) {
			perror("err");
		}
		memset(data, 0, 500);
	}
	fclose(fd);
        int filerc = htonl(226);
	if (send(sockstate, &filerc, sizeof filerc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}	
	return port;
}



int server_cd(int datasock, int sockstate, char* msg)
{
        char data[500];
	size_t num_read;
	FILE* fd;
        int rs = -1;
        if(chdir(msg) == -1) {
         perror("wrong directory.");
          }
        else{
        char dir[500];
        if(getcwd(dir, 500) == NULL) 
        {
        perror("Error getting the current working directory");
        exit(1);
        }
        char buf[200];
        sprintf(buf, "echo 'current directory is: %s '> tmp.txt", dir);
        rs = system(buf);
        }
        
        fseek(fd, SEEK_SET, 0);
	int filerc = htonl(1);
	if (send(sockstate, &filerc, sizeof filerc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}	 

	memset(data, 0, 500);
	while ((num_read = fread(data, 1, 500, fd)) > 0) {
		if (send(datasock, data, num_read, 0) < 0) {
			perror("err");
		}
		memset(data, 0, 500);
	}
	fclose(fd);
	int listrc = htonl(226);
	if (send(sockstate, &listrc, sizeof listrc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}		
        return 0;}



int server_service(int datasock, int sockstate, char* cmd)
{
	char data[500];
	size_t num_read;
	FILE* fd;
    int rs = -1;
    if(strcmp(cmd, "LIST") == 0) {
        rs = system("ls -l | tail -n+2 > tmp.txt");
    }
    else if(strcmp(cmd, "DATE") == 0) {
        rs = system("date > tmp.txt");
    }
    else if(strcmp(cmd, "W") == 0) {
        rs = system("echo 'user test anonymous' > tmp.txt");
    }
    
     else {
        char buf[256];
        sprintf(buf, "ping %s -c 1 > tmp.txt", cmd);
        rs = system(buf);
    }
	if (rs < 0) {
		exit(1);
	}
	fd = fopen("tmp.txt", "r");
	if (!fd) {
		exit(1);
	}

	fseek(fd, SEEK_SET, 0);
	int filerc = htonl(1);
	if (send(sockstate, &filerc, sizeof filerc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}	 

	memset(data, 0, 500);
	while ((num_read = fread(data, 1, 500, fd)) > 0) {
		if (send(datasock, data, num_read, 0) < 0) {
			perror("err");
		}
		memset(data, 0, 500);
	}
	fclose(fd);
	int listrc = htonl(226);
	if (send(sockstate, &listrc, sizeof listrc, 0) < 0 ) {
		perror("error sending...\n");
		return -1;
	}		
        return 0;
}









int server_connection(int sockstate)
{
	char buf[1024];
	int wait, datasock;

	if (recv(sockstate, &wait, sizeof wait, 0) < 0 ) {
		perror("Error while waiting");
		return -1;
	}
	struct sockaddr_in client_addr;
	socklen_t len = sizeof client_addr;
	getpeername(sockstate, (struct sockaddr*)&client_addr, &len);
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));
        int sockfd;
	struct sockaddr_in dest_addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        	perror("error creating socket");
        	return -1;
    }
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(30020);
	dest_addr.sin_addr.s_addr = inet_addr(&buf);

	if(connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0 ) {
        	perror("error connecting to server");
		return -1;
    	}
        datasock=sockfd;
	if (datasock < 0)
		return -1;
	return datasock;
}






int server_authentication(char*user, char*pass)
{
	FILE* fd;
	fd = fopen(".auth", "r");
        int flag = 0;
        char username[30];
	char password[30];
	char *usrnpwd;
	char buf[30];
	char *cmdline = NULL;
	size_t numbuffer;
	size_t len = 0;
	if (fd == NULL) {
		perror("file not found");
		exit(1);
	}
	while ((numbuffer = getline(&cmdline, &len, fd)) != -1) {
		memset(buf, 0, 30);
		strcpy(buf, cmdline);
                usrnpwd = strtok (buf," ");
		strcpy(username, usrnpwd);
                if (usrnpwd != NULL) {
			usrnpwd = strtok (NULL, " ");
			strcpy(password, usrnpwd);
		}
        int a, b=(int)strlen(password);
	for (a = 0; a < b; a++) {
		if (isspace(password[a])) password[a] = 0;
		if (password[a] == '\n') password[a] = 0;
	}
	if ((strcmp(user,username) == 0) && (strcmp(pass,password) == 0)) {
			flag = 1;
			break;
		}
	}
	free(cmdline);
	fclose(fd);
	return flag;
}





int server_login(int sockstate)
{
	char buf[100];
	char user[100];
	char pass[100];
	memset(user, 0, 100);
	memset(pass, 0, 100);
	memset(buf, 0, 100);
	size_t numbuffer;
	memset(buf, 0, sizeof(buf));
	numbuffer = recv(sockstate, buf, sizeof(buf), 0);
	if ( numbuffer == -1) {
		perror("can not receive message\n");
		exit(1);
	}
	int i = 5;
	int n = 0;
	while (buf[i] != 0)
		user[n++] = buf[i++];
        int loginrc=htonl(331);
                if (send(sockstate, &loginrc, sizeof loginrc, 0) < 0 ) {
		perror("can not send message\n");
		return -1;}
	memset(buf, 0, 100);
        size_t numbuffer2;
	memset(buf, 0, sizeof(buf));
	numbuffer2 = recv(sockstate, buf, sizeof(buf), 0);
	if ( numbuffer2 == -1) {
		perror("can not receive message\n");
		exit(1);
	}

	i = 5;
	n = 0;
	while (buf[i] != 0) {
		pass[n++] = buf[i++];
	}
        int flag=server_authentication(user, pass);
	return (flag);
}






int server_command(int sockstate, char*cmd, char*msg)
{
	int retcode ;
	char buffer[500];
        memset(buffer, 0, 500);
	memset(cmd, 0, 5);
	memset(msg,0, 500);
        size_t numbuffer;
	memset(buffer, 0, sizeof(buffer));
	numbuffer = recv(sockstate, buffer, sizeof(buffer), 0);
	if (numbuffer == -1) {
		perror("can not reveive message\n");
		return -1;
	}

	strncpy(cmd, buffer, 4);
	char *tmp = buffer + 5;
	strcpy(msg, tmp);

	if (strcmp(cmd, "QUIT") == 0) {
		retcode = 221;
	} else if((strcmp(cmd, "USER") == 0) || (strcmp(cmd, "PASS") == 0) ||
			(strcmp(cmd, "LIST") == 0) || (strcmp(cmd, "RETR") == 0) ||
            (strcmp(cmd, "DATE") == 0) || (strcmp(cmd, "PING") == 0) ||(strcmp(cmd, "CD") == 0)||
            (strcmp(cmd, "W") == 0)) {
		retcode = 200;
	} else { 
		retcode = 502;
	}
        int rcvcommand=htonl(retcode);
                if (send(sockstate, &rcvcommand, sizeof rcvcommand, 0) < 0 ) {
		perror("can not send message\n");
		return -1;}
	return retcode;
}






void server_handler(int sockstate)
{
	int datasock;
	char cmd[5];
	char msg[500];
        int conn=htonl(220);
        if (send(sockstate, &conn, sizeof conn, 0) < 0 ) {
		perror("can not send message\n");
		return -1;
	}

	if (server_login(sockstate) == 1) {
                int log=htonl(230);
                if (send(sockstate, &log, sizeof log, 0) < 0 ) {
		perror("can not send message\n");
		return -1;}
	}
		
	 else {
		int logfail=htonl(430);
                if (send(sockstate, &logfail, sizeof logfail, 0) < 0 ) 
               {
		perror("can not send message\n");
		return -1;
                }
		exit(0);
	}

	while (1) {
		int rc = server_command(sockstate, cmd, msg);

		if ((rc < 0) || (rc == 221)) {
			break;
		}

		if (rc == 200 ) {
			if ((datasock= server_connection(sockstate)) < 0) {
				close(sockstate);
				exit(1);
			}
                        
			if (strcmp(cmd, "LIST") == 0 || strcmp(cmd, "DATE") == 0 || strcmp(cmd, "W") == 0){ 
				server_service(datasock, sockstate, cmd);
			}
            else if (strcmp(cmd, "PING") == 0) {
                server_service(datasock, sockstate, msg);
            }
            else if (strcmp(cmd, "CD") == 0){
                server_cd(datasock,sockstate,msg);
            }
            else if (strcmp(cmd, "RETR") == 0) { 
				server_get(sockstate, datasock, msg);
			}
			close(datasock);
		}
	}
}
