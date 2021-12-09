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

#define MAX_PORT 5
#define MAX_TAREFA 20
#define MAX_TEXTO 240
#define MAX_USERID 5
#define MAX_PASSWORD 8
#define MAX_GROUPID 2
#define MAX_GROUPS 99 // grupo 01 ate 99
#define MAX_GROUPNAME 24
#define MAX_MESSAGEID 3
#define MAX_GROUPDISPLAY 3368  //tamanho de [ GID GName MID] = 1+2+1+24+1+4 | 99*33 +1


bool verboseStatus = false; // true = vebose on , false = verbose off


/*
struct userInfo{
    char userID[MAX_USERID]; // user ID
    char password[MAX_PASSWORD]; // user password
    char groupSub[99]; // ID of subcribed group
		char groupAct[1]; // ID of active group
    bool logStatus; // Logged in/out
} allUsers[100000]; // userID -> 00000 ate 99999

struct groupInfo{
		char groupID[MAX_GROUPID]; // group ID
		char name[MAX_GROUPNAME]  // group Name
		struct msgInfo; // mensagens neste grupo
} allGroups[99] //// groupID -> 01 ate 99

struct msgInfo{
		char msgID[MAX_MESSAGEID]; // message ID
		char text[MAX_TEXTO]  // texto da mensagem
} allMessages[9999] //// msgID -> 0001 ate 9999
*/

/******* Validar Port *******/
bool validarPort (char* argv) {
  if (strlen(argv) != 5)
      return false;

  for(int i = 0; i < strlen(argv); i++) {
      if (!isdigit(argv[i])) // Verificar que apenas contém números
          return false;
  }

  int port = atoi(argv);
  if (port > 0 && port < 64000) // Não pode ser mais de 64000
      return true;

  return false;
}


/******* Validar user_ID *******/
char* validarUser_ID(int i, char *comando, char *user_ID){
  for (int k = 0; comando[i] != ' '; i++,k++) {
      if (isdigit(comando[i]) > 0){
          user_ID[k] = comando[i];
      }
      else {
          printf("Erro no UID, tem que ser numerico\n");
          return  "";
      }
  }
  if (strlen(user_ID) != 5) {
      printf("Erro no UID, tem que ser tamanho 5\n");
      return  "";
  }
  return user_ID;
}


/******* Validar Password *******/
char* validarPassword(int i, char *comando, char *password){
  for (int k = 0; comando[i] != ' ' && comando[i] !='\n' && comando[i] !='\0';i++, k++) {
      if (isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0){ //isalnum() estava a dar erro
          password[k] = comando[i];
      }
      else {
          printf("Erro na password, tem que ser alphanumerico\n");
          return  "";
      }
  }
  if (strlen(password) != 8) {
      printf("Erro na password, tem que ser tamanho 8\n");
      return  "";
  }
  return password;
}


/******* Validar ID de grupos *******/
char* validarGroup_ID(int i, char* comando, char* group_ID){
  for (int w = 0; comando[i] != ' ' && isdigit(comando[i]) > 0; i++, w++) {
        group_ID[w] = comando[i];
  }

  if( strlen(group_ID) != 2){
    printf("Erro no group ID, tem que ser tamanho 2 e numerico\n");
    return  "";
  }

  return group_ID;
}


/******* Validar Nomes de grupos *******/
char* validarGroup_Name(int i, char* comando, char* group_Name){
  for (int w = 0; comando[i] != ' ' && comando[i] != '\n'; i++, w++) {
    if (isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0 ||
                comando[i] == '-' || comando[i] == '_'){
      group_Name[w] = comando[i];
    }
    else{
      printf("Erro no group Name, caracter invalido\n");
      return "";
    }
  }

  if( strlen(group_Name) > 24 || strlen(group_Name) == 0){
    printf("Erro no group Name, maximo tamanho 24\n");
    return  "";
  }

  return group_Name;
}


/******* Indica o status de verbose *******/
bool checkVerboseStatus(){
  return verboseStatus;
}



int main (int argc, char *argv[]) {

	char port[MAX_PORT] = "58021";

	/******* Validar input inicial do server *******/
	if (argc == 2) {
			if (strcmp(argv[1],"-v") == 0) { // verbose mode
					verboseStatus = true;
			}
			else {
					printf("Input errado\n");
					exit(1);
			}
	}
	else if (argc == 3) {
			if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2])) { // Port
					strcpy(port,argv[2]);
			}
			else {
					printf("Input errado\n");
					exit(1);
			}
	}
	else if (argc == 4){
			if (strcmp(argv[1],"-v") == 0 && strcmp(argv[2],"-p") == 0 && validarPort(argv[3])) {
					verboseStatus = true;
					strcpy(port,argv[3]);
			}
			else if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2]) && strcmp(argv[3],"-v") == 0) {
					strcpy(port,argv[2]);
					verboseStatus = true;
			}
			else {
					printf("Input errado\n");
					exit(1);
			}
	}
	else if (argc != 1) {
			printf("Input errado\n");
			exit(1);
	}

	/******* Inicializar conexao UDP *******/
	int fd,errcode;
	ssize_t n;
	socklen_t addrlen;
	struct addrinfo hints,*res;
	struct sockaddr_in addr;
	char buffer[128];

	fd=socket(AF_INET,SOCK_DGRAM,0); //UDP socket
	if(fd==-1) exit(1); //error

	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_INET; // IPv4
	hints.ai_socktype=SOCK_DGRAM; // UDP socket
	hints.ai_flags=AI_PASSIVE;

	errcode=getaddrinfo(NULL,port,&hints,&res);
	if(errcode!=0) exit(1); //error

	n=bind(fd,res->ai_addr, res->ai_addrlen);
	if(n==-1) exit(1); //error;

	while (1){

		char tarefa[MAX_TAREFA] = "";  //tarefa a executar
		char message_received[MAX_TEXTO] = "";  //mensagem recebida do user
		char message_sent[MAX_GROUPDISPLAY] = "";  //mensagem enviada ao user

		// comunicacao com o user em UDP
		addrlen=sizeof(addr);
		n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
		if(n==-1) exit(1); //error


		// le o tipo de tarefa que e para fazer da mensagem
		int t = 0;
		for (t; isalpha(message_received[t]) != 0; t++) {
				tarefa[t] = message_received[t];
		}


		/* ----------------------------------------- */
		/*        Tarefa: registar utilizador        */
		/*        recebide: >REG UID pass            */
		/*        envia: >RRG status                 */
		/* ----------------------------------------- */

		if (!strcmp(tarefa,"REG")){

			char user_ID[MAX_USERID + 1] = "";
			char password[MAX_PASSWORD + 1] = "";

			//Verificar o UID
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
			if(strcmp(user_ID,"") == 0){
				strcpy(message_sent,"RRG NOK\n");
				continue;
			}
			t=t+strlen(user_ID)+1;

			//Verificar a password
			strcpy(password, validarPassword( t+1, message_received, password));
			if(strcmp(password,"") == 0){
				strcpy(message_sent,"RRG NOK\n");
				continue;
			}

			//FAZER COISAS


			if(checkVerboseStatus()){
				printf("User %s registed\n", user_ID);
			}
			strcpy(message_sent,"RRG OK\n");
		}

	  else if (!strcmp(tarefa,"UNR")){

		}

		else if (!strcmp(tarefa,"LOG")){

		}

		else if (!strcmp(tarefa,"OUT")){

		}

		else if (!strcmp(tarefa,"GLS")){

		}

		else if (!strcmp(tarefa,"GSR")){

		}

		else if (!strcmp(tarefa,"GUR")){

		}

		else if (!strcmp(tarefa,"GLM")){

		}

		else if (!strcmp(tarefa,"ULS")){

		}

		else if (!strcmp(tarefa,"PST")){

		}

		else if (!strcmp(tarefa,"RTV")){

		}

	  else
	    strcpy(message_sent,"ERR");

		n=sendto(fd,message_sent,strlen(message_sent),0,(struct sockaddr*)&addr,addrlen);
		if(n==-1 )exit(1); //error

	}
	freeaddrinfo(res);
	close(fd);
	return 0;
}
