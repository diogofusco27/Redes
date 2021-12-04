#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_COMANDO 128
#define MAX_CODIGO 4
#define MAX_TEXTO 124
#define MAX_UID 5
#define MAX_PASSWORD 8
#define MAX_IP 20
#define MAX_PORTO 5

bool validarPorto (char* argv) {
	int len = strlen(argv);
	if (len != 5)
		return false;
	//Verificar que apenas contém números
	for(int i = 0; i < strlen(argv);i++) {
		if (!isdigit(argv[i]))
			return false;
	}
	//Não pode ser mais de 70000 nem menos de 58000
	int port = atoi(argv);
	if (port > 0 && port < 64000)
		return true;
	return false;
}

int main (int argc, char *argv[]) {
	char address[MAX_IP];
	char port[MAX_PORTO] = "58021";

	//Validar input inicial do utilizador
    if (argc == 3) {
		if (strcmp(argv[1],"-n") == 0) { //IP address (server)
			strcpy(address,argv[2]);
		} else if (strcmp(argv[1],"-p") == 0 && validarPorto(argv[2])) { //Port
			strcpy(port,argv[2]);
		} else {
			//error
			printf("Input errado\n");
			exit(1);
		}
	}
	else if (argc == 5){ //port
		if (strcmp(argv[1],"-n") == 0 && strcmp(argv[3],"-p") == 0 && validarPorto(argv[4])) {
			strcpy(address,argv[2]);
			strcpy(port,argv[4]);
		} else if (strcmp(argv[1],"-p") == 0 && validarPorto(argv[2]) && strcmp(argv[3],"-n") == 0) {
			strcpy(port,argv[2]);
			strcpy(address,argv[4]);
		} else {
			//error
			printf("Input errado\n");
			exit(1);
		}
		
	}
	else if (argc != 1) {
		//erro
		printf("Input errado\n");
		exit(1);
	}
	
	//Abrir UDP
	int fd,errcode;
	ssize_t n;
	socklen_t addrlen;
	struct addrinfo hints,*res;
	struct sockaddr_in addr;
	char buffer[128];

	fd=socket(AF_INET,SOCK_DGRAM,0);    //UDP socket
	if(fd==-1) exit(1); //error

	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET;            //IPv4
	hints.ai_socktype=SOCK_DGRAM;       //UDP socket
	
	errcode=getaddrinfo(address,"58001",&hints,&res);
	if(errcode!=0) {
		printf("Falha na verificação do IP\n");
		exit(1); //error
	} 

	//saber que função chamar
	char comando[MAX_COMANDO];
	char codigo[MAX_CODIGO];
	char message[128];

	//ciclo à espera de comandos do user
	while (1) {
		printf("à espera de input\n");
		fgets(comando,MAX_COMANDO,stdin);
		sscanf(comando,"%s",codigo);
		if (!strcmp(codigo,"reg")){ //reg UID pass
			
		} else if (!strcmp(codigo,"unregister") || !strcmp(codigo,"unr")) { //unregister UID pass

		} else if (!strcmp(codigo,"login")){ //login UID pass

		} else if (!strcmp(codigo,"logout")){ //logout

		} else if (!strcmp(codigo,"groups") || !strcmp(codigo,"gl")){ //groups

		} 
		//Apenas depois do login
		else if (!strcmp(codigo,"subscribe") || !strcmp(codigo,"s")){ //subscribe GID GName

		} else if (!strcmp(codigo,"unsubscribe") || !strcmp(codigo,"u")){ //unsubscribe GID

		} else if (!strcmp(codigo,"my_groups") || !strcmp(codigo,"mgl")){ //my_groups

		} else if (!strcmp(codigo,"select") || !strcmp(codigo,"sag")){ //select GID

		} 
		//TCP server
		else if (!strcmp(codigo,"ulist") || !strcmp(codigo,"ul")){ //ulist

		}
		//message commands
		else if (!strcmp(codigo,"post")){ //post “text” [Fname]

		} else if (!strcmp(codigo,"retrieve") || !strcmp(codigo,"r")){ //retrieve MID

		}
		else if (!strcmp(codigo,"exit")) { //exit
			break;
		} else {
			printf("Comando não suportado! Tente novamente!\n");
			continue;
		}
		printf("send to\n");
		n=sendto(fd,"Hello\n",6,0,res->ai_addr,res->ai_addrlen);
		if(n==-1) exit(1); //error

		addrlen=sizeof(addr);
		n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);

		if(n==-1) exit(1); //error

		write(1,"echo: ",6); 
		write(1,buffer,n);
	}

	freeaddrinfo(res);
	close(fd);
}

