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
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>

#define MAX_PORT 5
#define MAX_TAREFA 3
#define MAX_TEXTO 240
#define MAX_USER_ID 5
#define MAX_PASSWORD 8
#define MAX_GROUP_ID 2
#define MAX_GROUPS 99 // grupo 01 ate 99
#define MAX_GROUP_NAME 24
#define MAX_MESSAGE_ID 4
#define MAX_MESSAGES 9999
#define MAX_DIRECTORY 47 // directory + filename + .txt = 19+24+4 = 47

// TAREFA + ' ' + GROUPID = 3+1+2 = 6
// ' ' + GROUPID + ' ' + GROUPNAME + ' ' + MESSAGEID = 1+2+1+24+1+4 = 33
// NUMEROdeGRUPOS = 99                6+33*99 = 3273 + '\n' = 3274
#define MAX_MESSAGE_UDP_SENT 3274

// 'TAREFA status' = 3+1+3 = 7   [ GName[ UID]*] = 1+24+(1+5)*100000 = 600025
//  7 + 600025 +'\n' = 600033
#define MAX_MESSAGE_TCP_SENT 600033

// 'TAREFA UID GID Tsize text [Fname Fsize data]' =
//  3+1+5+1+2+1+3+1+240+1+[24+1+10+1+data] = 294 + data + '\n' = 295 + data
#define MAX_MESSAGE_TCP_RECEIVED 295

// TAREFA + ' ' + USERID + ' ' + GROUPID + ' ' + GROUPNAME + '\n'= 3+1+5+1+2+1+24 = 38
#define MAX_MESSAGE_UDP_RECEIVED 38


bool verboseStatus = false; // true = vebose on , false = verbose off


/******* Retorna o maior valor *******/
int max(int x, int y){
    if (x > y)
        return x;
    else
        return y;
}


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
  for (int k = 0; isdigit(comando[i]) > 0; i++,k++) {
      user_ID[k] = comando[i];
  }
  if (strlen(user_ID) != 5) {
      printf("Erro no UID, tem que ser tamanho 5 e numerico\n");
      return  "";
  }
  return user_ID;
}


/******* Validar Password *******/
char* validarPassword(int i, char *comando, char *password){
  for (int k = 0; isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0 ;i++, k++) {
      password[k] = comando[i];
  }
  if (strlen(password) != 8) {
      printf("Erro na password, tem que ser tamanho 8 e alphanumerica\n");
      return  "";
  }
  return password;
}


/******* Validar ID de grupos *******/
char* validarGroup_ID(int i, char* comando, char* group_ID){
  for (int w = 0; isdigit(comando[i]) > 0; i++, w++) {
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
  for (int w = 0; isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0 ||
                  comando[i] == '-' || comando[i] == '_'; i++, w++) {
    group_Name[w] = comando[i];
  }

  if( strlen(group_Name) > 24 || strlen(group_Name) == 0){
    printf("Erro no group Name, maximo tamanho 24 e tem que ser alphanumerico ou '-' ou '_'\n");
    return  "";
  }

  return group_Name;
}


bool verificaPortLoginFile(char* loginFile, char* portUser){
  char portAux[MAX_PORT + 1] = "";
  FILE *fp;

  //Vai buscar o port que foi usado para fazer login
  fp = fopen(loginFile, "r");
  int p, r = 0;
  while ((p = fgetc(fp)) != EOF){
    if(p == '\n') // quando apanha o '\n'
      break;
    portAux[r] = p;
    r++;
  }
  fclose(fp);

  if(strcmp(portUser, portAux) != 0)
    return false;
  return true;
}


/******* Indica o status de verbose *******/
bool checkVerboseStatus(){
  return verboseStatus;
}







int main (int argc, char *argv[]) {

	char port[MAX_PORT] = "58021"; //port default

  FILE *fp; // to create files

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


	/******* Inicializar conexao UDP e TCP e Select *******/
	int fdUDP, fdTCP, newfdTCP, errcode;
  fd_set rset;
  enum state {idle,busy};
  int maxfd, counter;
	ssize_t nu, nt;
	socklen_t addrlen;
	struct addrinfo hintsUDP, *resUDP, hintsTCP, *resTCP;
	struct sockaddr_in addr;
  pid_t pid;

	fdUDP = socket(AF_INET,SOCK_DGRAM,0); //UDP socket
	if(fdUDP == -1){
      printf("Falha na criação do socket UDP\n");
      exit(1); //error
  }

  fdTCP = socket(AF_INET,SOCK_STREAM,0); //TCP socket
	if(fdTCP == -1){
      printf("Falha na criação do socket TCP\n");
      exit(1); //error
  }

	memset(&hintsUDP,0,sizeof hintsUDP);
	hintsUDP.ai_family=AF_INET; // IPv4
	hintsUDP.ai_socktype=SOCK_DGRAM; // UDP socket
	hintsUDP.ai_flags=AI_PASSIVE;

  memset(&hintsTCP,0,sizeof hintsTCP);
	hintsTCP.ai_family=AF_INET; // IPv4
	hintsTCP.ai_socktype=SOCK_STREAM; //TCP socket
	hintsTCP.ai_flags=AI_PASSIVE;

	errcode = getaddrinfo(NULL,port,&hintsUDP,&resUDP);
	if(errcode != 0) exit(1); //error

  errcode = getaddrinfo(NULL,port,&hintsTCP,&resTCP);
	if(errcode != 0) exit(1); //error

	nu = bind(fdUDP,resUDP->ai_addr, resUDP->ai_addrlen);
	if(nu == -1){
      printf("Falha no bind do socket UDP\n");
      exit(1); //error
  }

  nt = bind(fdTCP,resTCP->ai_addr, resTCP->ai_addrlen);
	if(nt == -1){
      printf("Falha no bind do socket TCP\n");
      exit(1); //error
  }

  if(listen(fdTCP,5) == -1){
      printf("Falha no listen do socket TCP\n");
      exit(1); //error
  }

  FD_ZERO(&rset); // clear the descriptor set
  maxfd = max(fdUDP, fdTCP) + 1;

  /******* Ciclo que fica à espera das mensagens dos users *******/
	while (1){

    char tarefa[MAX_TAREFA + 1] = "";  // tarefa a executar
    char message_received[MAX_MESSAGE_TCP_RECEIVED + 1] = "";  // mensagem recebida do user
    char message_sent[MAX_MESSAGE_TCP_SENT + 1] = "";  // mensagem enviada ao user
    char dirName[MAX_DIRECTORY + 1] = ""; // nome de uma diretoria
    char fileName[MAX_DIRECTORY + 1] = ""; // nome de uma diretoria + ficheiro
    char addressUser[NI_MAXHOST] = ""; // address da mensagem recebida
    char portUser[NI_MAXSERV] = ""; // port da mensagem recebida

    // set fdTCP and fdUDP in readset
    FD_SET(fdUDP, &rset);
    FD_SET(fdTCP, &rset);

    // select the ready descriptor
    counter = select(maxfd, &rset, NULL, NULL, NULL);


                                  /*       */
                                  /*       */
                                  /*  UDP  */
                                  /*       */
                                  /*       */

    // if UDP socket is readable then receive the message
    if (FD_ISSET(fdUDP, &rset)) {

        // comunicacao com o user em UDP
     		addrlen = sizeof(addr);
     		nu = recvfrom(fdUDP,message_received,sizeof(message_received),0,(struct sockaddr*)&addr,&addrlen);
     		if(nu == -1){
             printf("Falha na receção da mensagem usando UDP\n");
             exit(1); //error
        }

        // Adquirir IP e Port da mensagem recebida
        if(getnameinfo((struct sockaddr *) &addr, addrlen,
                        addressUser, sizeof(addressUser),
                        portUser, sizeof(portUser), 0) != 0){
          printf("Failed to get Address and Port from user UDP message\n");
        }

    		// le o tipo de tarefa que e para fazer da mensagem
    		int t = 0;
    		for (t; isalpha(message_received[t]) != 0; t++) {
    				tarefa[t] = message_received[t];
    		}



    		/* ----------------------------------------- */
    		/*        Tarefa: registar utilizador        */
    		/*        recebe: >REG UID pass              */
    		/*        envia: >RRG status                 */
    		/* ----------------------------------------- */

    		if (!strcmp(tarefa,"REG")){

    			char user_ID[MAX_USER_ID + 1] = "";
    			char password[MAX_PASSWORD + 1] = "";

    			//Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser registation failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RRG NOK\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

    			//Verificar a password
    			strcpy(password, validarPassword( t+1, message_received, password));
    			if(strcmp(password,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser registation failed, invalid password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RRG NOK\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome da directoria
          strcpy(dirName, "USERS/");       // USERS/
          strcat(dirName, user_ID);        // USERS/uid

          //Verifica se o user ja esta registado
          if( access( dirName, R_OK ) == 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser registation failed, UID was already registed\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RRG DUP\n");
            goto sendMessageToUserUDP;
          }

          //Cria a directoria
          errcode = mkdir(dirName,0777);
          if(errcode!=0) exit(1); //error

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_pass.txt");   // USERS/uid/uid_pass.txt

          //Cria o ficheiro e escreve a password
    			fp = fopen(fileName, "w+");
          fputs(password, fp);
          fclose(fp);

    			if(checkVerboseStatus()){
    				printf("\nUser %s registed\n", user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}
    			strcpy(message_sent,"RRG OK\n");
    		}


        /* ----------------------------------------- */
        /*        Tarefa: Apagar utilizador          */
        /*        recebe: >UNR UID pass              */
        /*        envia: >RUN status                 */
        /* ----------------------------------------- */

    	  else if (!strcmp(tarefa,"UNR")){

          char user_ID[MAX_USER_ID + 1] = "";
    			char password[MAX_PASSWORD + 1] = "";
          char passwordFile[MAX_PASSWORD + 1] = "";
          char fileNameEND[MAX_DIRECTORY + 1] = ""; // diretoria vazia = USERS/uid/.

    			//Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser unregistation failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RUN NOK\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

    			//Verificar a password
    			strcpy(password, validarPassword( t+1, message_received, password));
    			if(strcmp(password,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser unregistation failed, invalid password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RUN NOK\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome da directoria
          strcpy(dirName, "USERS/");       // USERS/
          strcat(dirName, user_ID);        // USERS/uid

          //Verifica se o user ja esta registado
          if( access( dirName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser unregistation failed, UID was not registed\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RUN NOK\n");
            goto sendMessageToUserUDP;
          }

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_pass.txt");   // USERS/uid/uid_pass.txt

          //Buscar a password do userID
          fp = fopen(fileName, "r");
          int c, w = 0;
          while ((c = fgetc(fp)) != EOF){
            if(c == '\n') // quando apanha o '\n'
              break;
            passwordFile[w] = c;
            w++;
          }
          fclose(fp);

          //Comparar as passwords
          if (strcmp(password,passwordFile) != 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser unregistation failed, incorrect password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"ROU NOK\n");
            goto sendMessageToUserUDP;
          }

          //Apaga ficheiros dentro da diretoria
          DIR *theFolder = opendir(dirName);
          struct dirent *next_file;
          char filepath[50];

          strcpy(fileNameEND, dirName);     // USERS/uid
          strcat(fileNameEND, "/.");        // USERS/uid/.

          while ( (next_file = readdir(theFolder)) != NULL ){
              // cria o caminho para cada ficheiro
              sprintf(filepath, "%s/%s", dirName, next_file->d_name);
              if(strcmp(filepath, fileNameEND) == 0)
                break;
              errcode = remove(filepath);
              if(errcode!=0) exit(1); //error
          }
          closedir(theFolder);

          //Apaga a directoria
          errcode = rmdir(dirName);
          if(errcode!=0) exit(1); //error

    			if(checkVerboseStatus()){
    				printf("\nUser %s unregisted\n", user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}
    			strcpy(message_sent,"RUN OK\n");

    		}


        /* ----------------------------------------- */
        /*        Tarefa: login                      */
        /*        recebe: >LOG UID pass              */
        /*        envia: >RLO status                 */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"LOG")){

          char user_ID[MAX_USER_ID + 1] = "";
          char password[MAX_PASSWORD + 1] = "";
          char passwordFile[MAX_PASSWORD + 1] = "";
          char loginFile[MAX_PASSWORD + 1] = "";

          //Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser login failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RLO NOK\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

          //Cria o nome da directoria
          strcpy(dirName, "USERS/");       // USERS/
          strcat(dirName, user_ID);        // USERS/uid

          //Verifica se o user ja esta registado
          if( access( dirName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser login failed, UID was not registed\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
              strcpy(message_sent,"RLO NOK\n");
              goto sendMessageToUserUDP;
          }

          //Verificar a password
    			strcpy(password, validarPassword( t+1, message_received, password));
    			if(strcmp(password,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser login failed, invalid password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RLO NOK\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcpy(loginFile, fileName);     // USERS/uid/uid
          strcat(fileName, "_pass.txt");   // USERS/uid/uid_pass.txt
          strcat(loginFile, "_login.txt"); // USERS/uid/uid_login.txt

          //Buscar a password do userID
          fp = fopen(fileName, "r");
          int c, w = 0;
          while ((c = fgetc(fp)) != EOF){
            if(c == '\n') // quando apanha o '\n'
              break;
            passwordFile[w] = c;
            w++;
          }
          fclose(fp);

          //Comparar as passwords
          if (strcmp(password,passwordFile) != 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser login failed, incorrect password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
              strcpy(message_sent,"RLO NOK\n");
              goto sendMessageToUserUDP;
          }

          //Cria o ficheiro de login
          fp = fopen(loginFile, "w+");
          fputs(portUser,fp);
          fclose(fp);

          if(checkVerboseStatus()){
    				printf("\nUser %s logged in\n", user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}
    			strcpy(message_sent,"RLO OK\n");

    		}


        /* ----------------------------------------- */
        /*        Tarefa: logout                     */
        /*        recebe: >OUT UID pass              */
        /*        envia: >ROU status                 */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"OUT")){

          char user_ID[MAX_USER_ID + 1] = "";
          char password[MAX_PASSWORD + 1] = "";
          char passwordFile[MAX_PASSWORD + 1] = "";
          char loginFile[MAX_DIRECTORY + 1] = "";

          //Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser logout failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"ROU NOK\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

          //Cria o nome da directoria
          strcpy(dirName, "USERS/");        // USERS/
          strcat(dirName, user_ID);         // USERS/uid

          //Cria o nome do ficheiro
          strcpy(loginFile, dirName);       // USERS/uid
          strcat(loginFile, "/");           // USERS/uid/
          strcat(loginFile, user_ID);       // USERS/uid/uid
          strcat(loginFile, "_login.txt");  // USERS/uid/uid_login.txt

          //Verifica se o user esta logged in
          if( access( loginFile, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser logout failed, UID not logged in\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"ROU NOK\n");
            goto sendMessageToUserUDP;
          }

          //verifica se o user esta no mesmo port que usou para fazer login
          if(verificaPortLoginFile(loginFile, portUser) == false){
            if (checkVerboseStatus() == true) {
              printf("\nUser logout failed, invalid port connection\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"ROU NOK\n");
    				goto sendMessageToUserUDP;
    			}

          //Verificar a password
    			strcpy(password, validarPassword( t+1, message_received, password));
    			if(strcmp(password,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser logout failed, invalid password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"ROU NOK\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_pass.txt");   // USERS/uid/uid_pass.txt

          //Buscar a password do userID
          fp = fopen(fileName, "r");
          int c, w = 0;
          while ((c = fgetc(fp)) != EOF){
            if(c == '\n') // quando apanha o '\n'
              break;
            passwordFile[w] = c;
            w++;
          }
          fclose(fp);

          //Comparar as passwords
          if (strcmp(password,passwordFile) != 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser logout failed, incorrect password\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"ROU NOK\n");
            goto sendMessageToUserUDP;
          }

          //Apaga o ficheiro login
          errcode = remove(loginFile); // Faz logout
          if(errcode!=0) exit(1); //error

          if(checkVerboseStatus()){
    				printf("\nUser %s logged out\n", user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}
    			strcpy(message_sent,"ROU OK\n");

    		}


        /* ----------------------------------------- */
        /*        Tarefa: groups                     */
        /*        recebe: >GLS                       */
        /*        envia: >RGL N[ GID GName MID]*     */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"GLS")){

          // TAREFA + ' ' + GROUPID + ' ' = 3+1+2+1 = 7
          char taskInfo[7 + 1] = "";
          // ' ' + GROUPID + ' ' + GROUPNAME + ' ' + MESSAGEID = 1+2+1+24+1+4 = 33 + '\n' = 34
          char groupsInfo[34 + 1] = "";
          char groupID[MAX_GROUP_ID + 1] = "";
          char aux[3 + 1] = "";
          char groupName[MAX_DIRECTORY + 1] = "";
          char messageID[MAX_MESSAGE_ID + 1] = "";
          char lastMessageID[MAX_MESSAGE_ID + 1] = "";

          int i = 1;
          for( i; i <= MAX_GROUPS; i++){

            if(i < 10 ){
              sprintf(aux,"%d",i);
              strcpy(groupID, "0");
              strcat(groupID, aux);
            }
            else if(i >= 10 && i <= 99)
             sprintf(groupID,"%d",i);

            strcpy(dirName, "GROUPS/");        // GROUPS/
            strcat(dirName, groupID);          // GROUPS/gid

            //Verifica se o grupo existe, se não exisitr ja percorreu todos
            if( access( dirName, R_OK ) != 0 ) {
                break;
            }

            //Cria o nome do ficheiro
            strcpy(fileName, dirName);         // GROUPS/gid
            strcat(fileName, "/");             // GROUPS/gid/
            strcat(fileName, groupID);         // GROUPS/gid/gid
            strcat(fileName, "_name.txt");     // GROUPS/gid/gid_name.txt

            //Ler o nome do grupo ao ficheiro
            fp = fopen(fileName, "r");
            int c, w = 0;
            while ((c = fgetc(fp)) != EOF){
              if(c == '\n') // quando apanha o '\n'
                break;
              groupName[w] = c;
              w++;
            }
            fclose(fp);

            strcpy(messageID, "0000");
            int m = 1;
            for( m; m <= MAX_MESSAGES; m++){

                strcpy(lastMessageID, messageID);

                if(m < 10 ){
                  sprintf(aux,"%d",m);
                  strcpy(messageID, "000");
                  strcat(messageID, aux);
                }
                else if(m >= 10 && m <= 99){
                  sprintf(aux,"%d",m);
                  strcpy(messageID, "00");
                  strcat(messageID, aux);
                }
                else if(m >= 100 && m <= 999){
                  sprintf(aux,"%d",m);
                  strcpy(messageID, "0");
                  strcat(messageID, aux);
                }
                else if(m >= 1000 && m <= 9999){
                  sprintf(messageID,"%d",m);
                }

                strcpy(dirName, "GROUPS/");        // GROUPS/
                strcat(dirName, groupID);          // GROUPS/gid
                strcat(dirName, "/MSG/");          // GROUPS/gid/MSG/
                strcat(dirName, messageID);        // GROUPS/gid/MSG/mid

                //Verifica se a mensagem existe, se não exisitr ja percorreu todas
                if( access( dirName, R_OK ) != 0 ) {
                    break;
                }

            }

            //Cria a resposta
            strcat(groupsInfo, " ");            // ' '
            strcat(groupsInfo, groupID);        // ' gid'
            strcat(groupsInfo, " ");            // ' gid '
            strcat(groupsInfo, groupName);      // ' gid  gname'
            strcat(groupsInfo, " ");            // ' gid  gname '
            strcat(groupsInfo, lastMessageID);  // ' gid  gname  mid'

          }

          sprintf(groupID,"%d",i-1);
          strcpy(taskInfo, "RGL ");             // 'RGL '
          strcat(taskInfo, groupID);            // 'RGL numero_de_grupos'

          if(checkVerboseStatus()){
    				printf("\nGroups Displayed\n");
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}

          strcpy(message_sent, taskInfo);       // 'RGL numero_de_grupos'
          strcat(message_sent, groupsInfo);     // 'RGL numero_de_grupos[ gid  gname mid]*'
          strcat(message_sent, "\n");           // 'RGL numero_de_grupos[ gid  gname mid]*\n'

    		}


        /* ----------------------------------------- */
        /*        Tarefa: subscribe                  */
        /*        recebe: >GSR UID GID GName         */
        /*        envia: >RGS status                 */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"GSR")){

          char user_ID[MAX_USER_ID + 1] = "";
          char group_ID[MAX_GROUP_ID + 1] = "";
          char aux[1 + 1] = "";
          char group_Name[MAX_GROUP_NAME + 1] = "";
          char loginFile[MAX_PASSWORD + 1] = "";
          char fileName[MAX_DIRECTORY + 1] = "";

          //Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser subscription failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGS E_USR\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

          //Cria o nome do ficheiro
          strcpy(fileName, "USERS/");      // USERS/
          strcat(fileName, user_ID);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_login.txt");  // USERS/uid/uid_login.txt

          //Verificar se o user esta logged in
          if( access( fileName, F_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser subscription failed, UID not logged in\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGS NOK\n");
            goto sendMessageToUserUDP;
          }

          //Verificar o GID
    			strcpy(group_ID, validarGroup_ID( t+1, message_received, group_ID));
    			if(strcmp(group_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser subscription failed, invalid GID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGS E_GRP\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_GROUP_ID + 1;

          //Verificar o GName
    			strcpy(group_Name, validarGroup_Name( t+1, message_received, group_Name));
    			if(strcmp(group_Name,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser subscription failed, invalid GName\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGS E_GNAME\n");
    				goto sendMessageToUserUDP;
    			}

          //Verifica se é suposto criar um grupo novo
          if (strcmp(group_ID, "00") == 0 ){

            int i = 1;
            //Encontrar um groupID disponivel
            for( i; i <= MAX_GROUPS; i++){

              if(i < 10 ){
                sprintf(aux,"%d",i);
                strcpy(group_ID, "0");
                strcat(group_ID, aux);
              }
              else if(i >= 10 && i <= 99)
               sprintf(group_ID,"%d",i);

              strcpy(dirName, "GROUPS/");        // GROUPS/
              strcat(dirName, group_ID);         // GROUPS/gid

              //Verifica se o grupo existe, se não existir o grupo fica com esse id
              if( access( dirName, R_OK ) != 0 ) {
                  break;
              }

              //Verifica se ja existe o numero maximo de grupos
              if(i == 99){
                if (checkVerboseStatus() == true) {
                  printf("\nUser subscription failed, reached limit number of groups\n");
                  printf("Port: %s | Address: %s\n", portUser, addressUser);
                }
        				strcpy(message_sent,"RGS E_FULL\n");
        				goto sendMessageToUserUDP;
        			}
            }

            //Cria a directoria do grupo
            errcode = mkdir(dirName,0777);
            if(errcode!=0) exit(1); //error

            //Cria o nome do ficheiro
            strcpy(fileName, dirName);       // GROUPS/gid
            strcat(fileName, "/");           // GROUPS/gid/
            strcat(fileName, group_ID);      // GROUPS/gid/gid
            strcat(fileName, "_name.txt");   // GROUPS/gid/gid_name.txt

            //Cria o ficheiro e escreve o nome do grupo
      			fp = fopen(fileName, "w+");
            fputs(group_Name, fp);
            fclose(fp);

            //Cria o nome da diretoria das mensagens do grupo
            strcat(dirName, "/");            // GROUPS/gid/
            strcat(dirName, "MSG");          // GROUPS/gid/MSG

            //Cria a diretoria das mensagens do grupo
            errcode = mkdir(dirName,0777);
            if(errcode!=0) exit(1); //error

            //Cria o nome do ficheiro
            strcpy(fileName, "GROUPS/");     // GROUPS/
            strcat(fileName, group_ID);      // GROUPS/gid
            strcat(fileName, "/");           // GROUPS/gid/
            strcat(fileName, user_ID);       // GROUPS/gid/uid
            strcat(fileName, ".txt");        // GROUPS/gid/uid.txt

            //Cria o ficheiro
            fp = fopen(fileName, "w+");  // Subscrever no grupo
            fputs(user_ID, fp);
            fclose(fp);

            if(checkVerboseStatus()){
      				printf("\nGroup %s created and subscribed by user %s\n", group_ID, user_ID);
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }

            strcpy(message_sent, "RGS NEW ");
            strcat(message_sent, group_ID);
            strcat(message_sent, "\n");
            goto sendMessageToUserUDP;
          }


          //Cria o nome da diretoria do grupo
          strcpy(dirName, "GROUPS/");       // GROUPS/
          strcat(dirName, group_ID);        // GROUPS/gid

          //Verifica se o grupo existe
          if( access( dirName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser subscription failed, GID does not exist\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGS NOK\n");
            goto sendMessageToUserUDP;
          }

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // GROUPS/gid
          strcat(fileName, "/");           // GROUPS/gid/
          strcat(fileName, user_ID);       // GROUPS/gid/uid
          strcat(fileName, ".txt");        // GROUPS/gid/uid.txt

          //Cria o ficheiro
          fp = fopen(fileName, "w+");  // Subscribe no grupo
          fputs(user_ID, fp);
          fclose(fp);

          if(checkVerboseStatus()){
            printf("\nGroup %s subscribed by user %s\n", group_ID, user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
          }

          strcpy(message_sent, "RGS OK\n");
    		}


        /* ----------------------------------------- */
        /*        Tarefa: unsubscribe                */
        /*        recebe: >GUR UID GID               */
        /*        envia: >RGU status                 */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"GUR")){

          char user_ID[MAX_USER_ID + 1] = "";
          char group_ID[MAX_GROUP_ID + 1] = "";

          //Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser unsubscription failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGU E_USR\n");
    				goto sendMessageToUserUDP;
    			}
    			t = t + MAX_USER_ID + 1;

          //Cria o nome do ficheiro
          strcpy(fileName, "USERS/");      // USERS/
          strcat(fileName, user_ID);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_login.txt");  // USERS/uid/uid_login.txt

          //Verificar se o user esta logged in
          if( access( fileName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser unsubscription failed, UID not logged in\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGU NOK\n");
            goto sendMessageToUserUDP;
          }

          //Verificar o GID
    			strcpy(group_ID, validarGroup_ID( t+1, message_received, group_ID));
    			if(strcmp(group_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nUser unsubscription failed, invalid GID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGU E_GRP\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome da diretoria do grupo
          strcpy(dirName, "GROUPS/");       // GROUPS/
          strcat(dirName, group_ID);        // GROUPS/gid

          //Verifica se o grupo existe
          if( access( dirName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser unsubscription failed, GID does not exist\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGU NOK\n");
            goto sendMessageToUserUDP;
          }

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);       // GROUPS/gid
          strcat(fileName, "/");           // GROUPS/gid/
          strcat(fileName, user_ID);       // GROUPS/gid/uid
          strcat(fileName, ".txt");        // GROUPS/gid/uid.txt

          //Verifica se o user esta subscrito no grupo
          if( access( fileName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nUser unsubscription failed, UID not subscribed to GID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGU NOK\n");
            goto sendMessageToUserUDP;
          }

          errcode = remove(fileName); // Unsubscribed no grupo
          if(errcode!=0) exit(1); //error

          if(checkVerboseStatus()){
            printf("\nGroup %s unsubscribed by user %s\n", group_ID, user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
          }

          strcpy(message_sent, "RGU OK\n");
    		}


        /* ----------------------------------------- */
        /*        Tarefa: my groups                  */
        /*        recebe: >GLM UID                   */
        /*        envia: >RGM N[ GID GName MID]*     */
        /* ----------------------------------------- */

    		else if (!strcmp(tarefa,"GLM")){

          // TAREFA + ' ' + GROUPID + ' ' = 3+1+2+1 = 7
          char taskInfo[7 + 1] = "";
          // ' ' + GROUPID + ' ' + GROUPNAME + ' ' + MESSAGEID = 1+2+1+24+1+4 = 33 + '\n' = 34
          char groupsInfo[34 + 1] = "";
          char groupID[MAX_GROUP_ID + 1] = "";
          char aux[3 + 1] = "";
          char groupName[MAX_DIRECTORY + 1] = "";
          char user_ID[MAX_USER_ID + 1] = "";
          char lastMessageID[MAX_MESSAGE_ID + 1] = "";
          char messageID[MAX_MESSAGE_ID + 1] = "";

          //Verificar o UID
    			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
    			if(strcmp(user_ID,"") == 0){
            if (checkVerboseStatus() == true) {
              printf("\nMy groups displayed failed, invalid UID\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGM E_USR\n");
    				goto sendMessageToUserUDP;
    			}

          //Cria o nome do ficheiro
          strcpy(fileName, "USERS/");      // USERS/
          strcat(fileName, user_ID);       // USERS/uid
          strcat(fileName, "/");           // USERS/uid/
          strcat(fileName, user_ID);       // USERS/uid/uid
          strcat(fileName, "_login.txt");  // USERS/uid/uid_login.txt

          //Verificar se o user esta logged in e esta no port em que fez login
          if( access( fileName, R_OK ) != 0 ) {
            if (checkVerboseStatus() == true) {
              printf("\nMy groups displayed failed, UID not logged in\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
            strcpy(message_sent,"RGM E_USR\n");
            goto sendMessageToUserUDP;
          }

          //verifica se o user esta no mesmo port que usou para fazer login
          if(verificaPortLoginFile(fileName, portUser) == false){
            if (checkVerboseStatus() == true) {
              printf("\nMy groups displayed failed, invalid port connection\n");
              printf("Port: %s | Address: %s\n", portUser, addressUser);
            }
    				strcpy(message_sent,"RGM E_USR\n");
    				goto sendMessageToUserUDP;
    			}

          int i = 1, counter = 0;
          for( i; i <= MAX_GROUPS; i++){
            strcpy(dirName, "GROUPS/");         // GROUPS/

            if(i < 10 ){
              sprintf(aux,"%d",i);
              strcpy(groupID, "0");
              strcat(groupID, aux);
            }
            else if(i >= 10 && i <= 99)
             sprintf(groupID,"%d",i);

            strcat(dirName, groupID);          // GROUPS/gid

            //Verifica se o grupo existe
            if( access( dirName, R_OK ) != 0 ) {
                break;
            }

            strcpy(fileName, dirName);         // GROUPS/gid
            strcat(fileName, "/");             // GROUPS/gid/
            strcat(fileName, user_ID);         // GROUPS/gid/uid
            strcat(fileName, ".txt");          // GROUPS/gid/uid.txt

            //Verifica se o user esta subscribed no grupo
            if( access( fileName, R_OK ) == 0 ){

              strcpy(fileName, dirName);       // GROUPS/gid
              strcat(fileName, "/");           // GROUPS/gid/
              strcat(fileName, groupID);       // GROUPS/gid/gid
              strcat(fileName, "_name.txt");   // GROUPS/gid/gid_name.txt

              //Ler o nome do grupo ao ficheiro
              fp = fopen(fileName, "r");
              int c, w = 0;
              while ((c = fgetc(fp)) != EOF){
                if(c == '\n') // quando apanha o '\n'
                  break;
                groupName[w] = c;
                w++;
              }
              fclose(fp);

              strcpy(messageID, "0000");
              int m = 1;
              for( m; m <= MAX_MESSAGES; m++){

                  strcpy(lastMessageID, messageID);

                  if(m < 10 ){
                    sprintf(aux,"%d",m);
                    strcpy(messageID, "000");
                    strcat(messageID, aux);
                  }
                  else if(m >= 10 && m <= 99){
                    sprintf(aux,"%d",m);
                    strcpy(messageID, "00");
                    strcat(messageID, aux);
                  }
                  else if(m >= 100 && m <= 999){
                    sprintf(aux,"%d",m);
                    strcpy(messageID, "0");
                    strcat(messageID, aux);
                  }
                  else if(m >= 1000 && m <= 9999){
                    sprintf(messageID,"%d",m);
                  }

                  strcpy(dirName, "GROUPS/");        // GROUPS/
                  strcat(dirName, groupID);          // GROUPS/gid
                  strcat(dirName, "/MSG/");          // GROUPS/gid/MSG/
                  strcat(dirName, messageID);        // GROUPS/gid/MSG/mid

                  //Verifica se a mensagem existe, se não exisitr ja percorreu todas
                  if( access( dirName, R_OK ) != 0 ) {
                      break;
                  }

              }

              strcat(groupsInfo, " ");            // ' '
              strcat(groupsInfo, groupID);        // ' gid'
              strcat(groupsInfo, " ");            // ' gid '
              strcat(groupsInfo, groupName);      // ' gid  gname'
              strcat(groupsInfo, " ");            // ' gid  gname '
              strcat(groupsInfo, lastMessageID);  // ' gid  gname  mid'

              counter++;
            }

          }

          sprintf(groupID,"%d",counter);
          strcpy(taskInfo, "RGM ");             // 'RGL '
          strcat(taskInfo, groupID);            // 'RGL numero_de_grupos'

          if(checkVerboseStatus()){
    				printf("\nGroups subscribed by user %s displayed\n", user_ID);
            printf("Port: %s | Address: %s\n", portUser, addressUser);
    			}

          strcpy(message_sent, taskInfo);       // 'RGL numero_de_grupos'
          strcat(message_sent, groupsInfo);     // 'RGL numero_de_grupos[ gid  gname mid]*'
          strcat(message_sent, "\n");           // 'RGL numero_de_grupos[ gid  gname mid]*\n'

    		}


        /* ----------------------------------------- */
        /*        Tarefa:                            */
        /*        recebe: >Comando invalido          */
        /*        envia: >ERR                        */
        /* ----------------------------------------- */

    	  else
    	    strcpy(message_sent,"ERR");


        sendMessageToUserUDP:

    		nu = sendto(fdUDP,message_sent,strlen(message_sent),0,(struct sockaddr*)&addr,addrlen);
    		if(nu == -1 ){
            printf("Falha no envia da mensagem usando UDP\n");
            exit(1); //error
        }

  }
                                    /*       */
                                    /*       */
                                    /*  TCP  */
                                    /*       */
                                    /*       */

  // ATENTION Only one messaging command (post, retreive) can be issued at a given time.

  // if TCP socket is readable then accept the connection
  if (FD_ISSET(fdTCP, &rset)) {

        // aceita a coneção TCP e cria um fork para fazer o trabalho
        addrlen = sizeof(addr);
        if((newfdTCP = accept(fdTCP ,(struct sockaddr *) &addr,&addrlen)) == -1){ // espera por coneção TCP
            printf("Falha no accept da coneção TCP\n");
            exit(1); //error
        }

        // Adquirir IP e Port da mensagem recebida
        if(getnameinfo((struct sockaddr *) &addr, addrlen,
                        addressUser, sizeof(addressUser),
                        portUser, sizeof(portUser), 0) != 0){
            printf("Failed to get Address and Port from user TCP message\n");
        }

        if((pid = fork()) == -1) {
            printf("Erro no fork\n");
            exit(1); //error
        }
        else if(pid == 0){ // child process starts here

            close(fdTCP);

            // ATENTION
            /*
            char message_received_Aux[MAX_MESSAGE_TCP_RECEIVED + 1] = "";  //mensagem recebida do user

            while (1){
              nt = read(newfdTCP,message_received_Aux,sizeof(message_received_Aux));
              if(nt == -1){
                printf("Falha na receção da mensagem usando TCP\n");
                exit(1); //error
              }
              if(nt == 0){ // ja não ha nada para ler da mensagem do user
                break;
              }
              strcat(message_received,message_received_Aux);
            }
            */

            nt = read(newfdTCP,message_received,sizeof(message_received));
            if(nt == -1){
              printf("Falha na receção da mensagem usando TCP\n");
              exit(1); //error
            }

            // le o tipo de tarefa que e para fazer da mensagem
            int t = 0;
            for (t; isalpha(message_received[t]) != 0; t++) {
              tarefa[t] = message_received[t];
            }


            /* --------------------------------------------- */
            /*        Tarefa: ulist                          */
            /*        recebe: >ULS GID                       */
            /*        envia: >RUL status [GName [UID ]*]     */
            /* --------------------------------------------- */

            if (!strcmp(tarefa,"ULS")){

                char group_ID[MAX_GROUP_ID + 1] = "";
                char group_Name[MAX_GROUP_NAME + 1] = "";
                char user_ID[MAX_USER_ID + 1] = "";
                char fileNameEND[MAX_DIRECTORY + 1] = ""; // diretoria vazia = GROUPS/uid/.

                //Verificar o GID
                strcpy(group_ID, validarGroup_ID( t+1, message_received, group_ID));
                if(strcmp(group_ID,"") == 0){
                  if (checkVerboseStatus() == true) {
                    printf("\nUser list display failed, invalid GID\n");
                    printf("Port: %s | Address: %s\n", portUser, addressUser);
                  }
                  strcpy(message_sent,"RUL NOK\n");
                  goto sendMessageToUserTCP;
                }

                //Cria o nome da diretoria do grupo
                strcpy(dirName, "GROUPS/");       // GROUPS/
                strcat(dirName, group_ID);        // GROUPS/gid

                //Verifica se o grupo existe
                if( access( dirName, R_OK ) != 0 ) {
                  if (checkVerboseStatus() == true) {
                    printf("\nUser list display failed, GID does not exist\n");
                    printf("Port: %s | Address: %s\n", portUser, addressUser);
                  }
                  strcpy(message_sent,"RUL NOK\n");
                  goto sendMessageToUserTCP;
                }

                ////Cria o nome do ficheiro
                strcpy(fileName, dirName);        // GROUPS/gid
                strcat(fileName, "/");            // GROUPS/gid/
                strcat(fileName, group_ID);       // GROUPS/gid/gid
                strcat(fileName, "_name.txt");    // GROUPS/gid/gid_name.txt

                //Ler o nome do grupo no ficheiro
                fp = fopen(fileName, "r");
                int c, w = 0;
                while ((c = fgetc(fp)) != EOF){
                  if(c == '\n') // quando apanha o '\n'
                    break;
                  group_Name[w] = c;
                  w++;
                }
                fclose(fp);

                strcpy(message_sent, "RUL OK ");
                strcat(message_sent, group_Name);

                //Cria o nome do ficheiro
                strcpy(fileNameEND, dirName);     // GROUPS/gid
                strcat(fileNameEND, "/.");        // GROUPS/gid/.

                //Le ficheiros dentro da diretoria
                DIR *theFolder = opendir(dirName);
                struct dirent *next_file;
                char filepath[MAX_DIRECTORY + 1];
                int userSubscribedCounter = 0;

                while ( (next_file = readdir(theFolder)) != NULL ){
                  // cria o caminho para cada ficheiro
                  sprintf(filepath, "%s/%s", dirName, next_file->d_name);

                  if(strcmp(filepath, fileNameEND) == 0)
                    break;

                  if(strcmp(filepath, fileName) != 0){ // não abre o GROUPS/gid/gid_name.txt
                    //Ler o nome do user no ficheiro
                    fp = fopen(filepath, "r");
                    int c, w = 0;
                    while ((c = fgetc(fp)) != EOF){
                      if(c == '\n') // quando apanha o '\n'
                        break;
                      user_ID[w] = c;
                      w++;
                    }
                    fclose(fp);

                    strcat(message_sent, " ");
                    strcat(message_sent, user_ID);
                    userSubscribedCounter++;
                  }
                }
                closedir(theFolder);

                strcat(message_sent, "\n"); // coloca '\n' no fim da mensagem

                if(checkVerboseStatus()){
                  printf("\nUser list of group %s displayed\n", group_Name);
                  printf("Port: %s | Address: %s\n", portUser, addressUser);
                }

            }


            /* ----------------------------------------------------------- */
            /*        Tarefa: post                                         */
            /*        recebe: >PST UID GID Tsize text [Fname Fsize data]   */
            /*        envia: >RPT status                                   */
            /* ----------------------------------------------------------- */

            else if (!strcmp(tarefa,"PST")){

            }


            /* ---------------------------------------------------------------------------- */
            /*        Tarefa: retrieve                                                      */
            /*        recebe: >RTV UID GID MID                                              */
            /*        envia: >RRT status [N[ MID UID Tsize text [/ Fname Fsize data]]*]     */
            /* ---------------------------------------------------------------------------- */

            else if (!strcmp(tarefa,"RTV")){

            }


            /* ----------------------------------------- */
            /*        Tarefa:                            */
            /*        recebe: >Comando invalido          */
            /*        envia: >ERR                        */
            /* ----------------------------------------- */

            else
                strcpy(message_sent,"ERR");


            sendMessageToUserTCP:

            nt = write(newfdTCP,message_sent,strlen(message_sent));
            if(nt == -1){
                printf("Falha no envia da mensagem usando TCP\n");
                exit(1); //error
            }

            exit(0); // child process ends here
            }
            // parent continua a partir daqui
            close(newfdTCP);

          }

    }

  freeaddrinfo(resTCP);
  freeaddrinfo(resUDP);
  close(fdUDP);
  close(fdTCP);
  return 0;
}
