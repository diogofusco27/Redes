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
#define MAX_USERID 5
#define MAX_PASSWORD 8
#define MAX_GROUPID 2
#define MAX_GROUPS 99 // grupo 01 ate 99
#define MAX_GROUPNAME 24
#define MAX_MESSAGEID 3
#define MAX_DIRECTORY 47 // directory + filename + .txt = 19+24+4 = 47

// TAREFA + ' ' + GROUPID = 3+1+2 = 6
// ' ' + GROUPID + ' ' + GROUPNAME + ' ' + MESSAGEID = 1+2+1+24+1+4 = 33
// NUMEROdeGRUPOS = 99                6+33*99 = 3273 + '\n' = 3274
#define MAX_MESSAGEUDPSENT 3274

// TAREFA + ' ' + USERID + ' ' + GROUPID + ' ' + GROUPNAME + '\n'= 3+1+5+1+2+1+24 = 38
#define MAX_MESSAGEUDPRECEIVED 38

char* itoa(int, char* , int);

bool verboseStatus = false; // true = vebose on , false = verbose off


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
    printf("Erro no group Name, maximo tamanho 24e tem que ser alphanumerico ou '-' ou '_'\n");
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
	int fd, errcode, connection_info;
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

	connection_info = getaddrinfo(NULL,port,&hints,&res);
	if(connection_info!=0) exit(1); //error

	n=bind(fd,res->ai_addr, res->ai_addrlen);
	if(n==-1) exit(1); //error;


/******* Criar directorias *******/
  FILE *fp;

  // Verifica se o ficheiro existe, se existir as diretorias tambem existem
  fp = fopen("IWasHere.txt", "r"); // ATENTION nao sei se e preciso verificar?
  if (fp == NULL) {
    errcode = mkdir("USERS",0777);
    if(errcode!=0) exit(1); //error
    errcode = mkdir("GROUPS",0777);
    if(errcode!=0) exit(1); //error
    fp = fopen("GROUPS/lastGroupID.txt", "w+"); // Ficheiro que contem id do ultimo grupo a ser criado
    fputs("00",fp); // Inicializa o ficheiro com o id '00'
    fclose(fp);
    fp = fopen("IWasHere.txt", "w+");
  }
  fclose(fp);




  /******* Ciclo que fica à espera das mensagens dos users *******/
	while (1){

    bool userShutedDown = false;

		char tarefa[MAX_TAREFA + 1] = "";  // tarefa a executar
		char message_received[MAX_MESSAGEUDPRECEIVED + 1] = "";  // mensagem recebida do user
		char message_sent[MAX_MESSAGEUDPSENT + 1] = "";  // mensagem enviada ao user
    char dirName[MAX_DIRECTORY + 1] = ""; // nome de uma diretoria
    char fileName[MAX_DIRECTORY + 1] = ""; // nome de um ficheiro + diretoria
    char addressUser[NI_MAXHOST]; // address da mensagem recebida
    char portUser[NI_MAXSERV]; // port da mensagem recebida

		// comunicacao com o user em UDP
		addrlen=sizeof(addr);
		n=recvfrom(fd,message_received,MAX_MESSAGEUDPRECEIVED,0,(struct sockaddr*)&addr,&addrlen);
		if(n==-1) exit(1); //error

    // Adquirir IP e Port da mensagem recebida
    connection_info = getnameinfo((struct sockaddr *) &addr,
                                  addrlen, addressUser, NI_MAXHOST,
                                  portUser, NI_MAXSERV, NI_NUMERICSERV);

		// le o tipo de tarefa que e para fazer da mensagem
		int t = 0;
		for (t; isalpha(message_received[t]) != 0; t++) {
				tarefa[t] = message_received[t];
		}


    /* ----------------------------------------- */
		/*        Tarefa: exit (logout)              */
		/*        recebe: >EXT UID                   */
		/*        envia: >                           */
		/* ----------------------------------------- */

    if (!strcmp(tarefa,"EXT")){

      char user_ID[MAX_USERID + 1] = "";
      char forged_message[MAX_MESSAGEUDPRECEIVED + 1] = "";
      char passwordFile[MAX_PASSWORD + 1] = "";

      //Buscar o UID
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
      if(strcmp(user_ID,"") == 0){ // easter egg
				printf("Only by the power of god could this be written");
        continue;
			}

      //Cria o nome do ficheiro
      strcpy(fileName, "USERS/");      // USERS/
      strcat(fileName, user_ID);       // USERS/uid
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

      strcpy(tarefa, "OUT"); // tarefa passa a ser logout

      strcpy(forged_message, tarefa);        // 'OUT'
      strcat(forged_message, " ");           // 'OUT '
      strcat(forged_message, user_ID);       // 'OUT uid'
      strcat(forged_message, " ");           // 'OUT uid '
      strcat(forged_message, passwordFile);  // 'OUT uid pass'
      strcat(forged_message, "\n");          // 'OUT uid pass\n'

      strcpy(message_received, forged_message); //substitui a message_received
      userShutedDown = true;

    }


		/* ----------------------------------------- */
		/*        Tarefa: registar utilizador        */
		/*        recebe: >REG UID pass              */
		/*        envia: >RRG status                 */
		/* ----------------------------------------- */

		if (!strcmp(tarefa,"REG")){

			char user_ID[MAX_USERID + 1] = "";
			char password[MAX_PASSWORD + 1] = "";

			//Verificar o UID
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
			if(strcmp(user_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser registation failed, invalid UID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RRG NOK\n");
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

			//Verificar a password
			strcpy(password, validarPassword( t+1, message_received, password));
			if(strcmp(password,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser registation failed, invalid password\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RRG NOK\n");
				goto sendMessageToUser;
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
        goto sendMessageToUser;
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

      char user_ID[MAX_USERID + 1] = "";
			char password[MAX_PASSWORD + 1] = "";
      char passwordFile[MAX_PASSWORD + 1] = "";
      char fileNameEND[MAX_PASSWORD + 1] = ""; // diretoria vazia = USERS/uid/.

			//Verificar o UID
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
			if(strcmp(user_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser unregistation failed, invalid UID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RUN NOK\n");
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

			//Verificar a password
			strcpy(password, validarPassword( t+1, message_received, password));
			if(strcmp(password,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser unregistation failed, invalid password\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RUN NOK\n");
				goto sendMessageToUser;
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
        goto sendMessageToUser;
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
        goto sendMessageToUser;
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

      char user_ID[MAX_USERID + 1] = "";
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
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

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
          goto sendMessageToUser;
      }

      //Verificar a password
			strcpy(password, validarPassword( t+1, message_received, password));
			if(strcmp(password,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser login failed, invalid password\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RLO NOK\n");
				goto sendMessageToUser;
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
          goto sendMessageToUser;
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

      char user_ID[MAX_USERID + 1] = "";
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
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

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
        goto sendMessageToUser;
      }

      //verifica se o user esta no mesmo port que usou para fazer login
      if(verificaPortLoginFile(loginFile, portUser) == false){
        if (checkVerboseStatus() == true) {
          printf("\nUser logout failed, invalid port connection\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"ROU NOK\n");
				goto sendMessageToUser;
			}

      //Verificar a password
			strcpy(password, validarPassword( t+1, message_received, password));
			if(strcmp(password,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser logout failed, invalid password\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"ROU NOK\n");
				goto sendMessageToUser;
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
        goto sendMessageToUser;
      }


      //Apaga o ficheiro login
      errcode = remove(loginFile); // Faz logout
      if(errcode!=0) exit(1); //error

      if(checkVerboseStatus()){
				printf("\nUser %s logged out\n", user_ID);
        printf("Port: %s | Address: %s\n", portUser, addressUser);
			}
			strcpy(message_sent,"ROU OK\n");

      if(userShutedDown == true){
        continue;
      }

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
      char groupID[MAX_GROUPID + 1] = "";
      char aux[1 + 1] = "";
      char groupName[MAX_DIRECTORY + 1] = "";
      char lastMessageID[MAX_MESSAGEID + 1] = "";

      int i = 1;
      for( i; i <= MAX_GROUPS; i++){
        strcpy(dirName, "GROUPS/");         // GROUPS/

        if(i < 10 ){
          sprintf(aux,"%d",i);
          strcpy(groupID, "0");
          strcat(groupID, aux);
        }
        else if(i >= 10 && i <= 99)
         sprintf(groupID,"%d",i);
        else // easter egg
          printf("Hello there !!");

        strcat(dirName, groupID);          // GROUPS/gid

        //Verifica se o grupo existe, senão exisitr ja percorreu todos
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

        //Cria o nome do ficheiro
        strcpy(fileName, dirName);             // GROUPS/gid
        strcat(fileName, "/MSG/");             // GROUPS/gid/MSG/
        strcat(fileName, "lastMessageID.txt"); // GROUPS/gid/MSG/lastMessageID.txt

        //Ler o id da ultima mensagem do grupo no ficheiro
        fp = fopen(fileName, "r+");
        int h, f = 0;
        while ((h = fgetc(fp)) != EOF){
          if(h == '\n') // se apanha um '\n'
            break;
          lastMessageID[f] = h;
          f++;
        }
        fclose(fp);

        //Cria o nome do ficheiro
        strcat(groupsInfo, " ");            // ' '
        strcat(groupsInfo, groupID);        // ' gid'
        strcat(groupsInfo, " ");            // ' gid '
        strcat(groupsInfo, groupName);      // ' gid  gname'
        strcat(groupsInfo, " ");            // ' gid  gname '
        strcat(groupsInfo, lastMessageID);  // ' gid  gname  mid'

      }

      //Vai buscar o groupID do ultimo grupo, que é igual ao numero de grupos existentes
      fp = fopen("GROUPS/lastGroupID.txt", "r");
      int p, r = 0;
      while ((p = fgetc(fp)) != EOF){
        if(p == '\n') // quando apanha o '\n'
          break;
        groupID[r] = p;
        r++;
      }
      fclose(fp);

      strcpy(taskInfo, "RGL ");             // 'RGL '
      strcat(taskInfo, groupID);            // 'RGL gid'

      if(checkVerboseStatus()){
				printf("\nGroups Displayed\n");
        printf("Port: %s | Address: %s\n", portUser, addressUser);
			}

      strcpy(message_sent, taskInfo);       // 'RGL gid'
      strcat(message_sent, groupsInfo);     // 'RGL gid[ gid  gname mid]*'
      strcat(message_sent, "\n");           // 'RGL gid[ gid  gname mid]*\n'

		}


    /* ----------------------------------------- */
    /*        Tarefa: subscribe                  */
    /*        recebe: >GSR UID GID GName         */
    /*        envia: >RGS status                 */
    /* ----------------------------------------- */

		else if (!strcmp(tarefa,"GSR")){

      char user_ID[MAX_USERID + 1] = "";
      char group_ID[MAX_GROUPID + 1] = "";
      char aux[1 + 1] = "";
      char group_Name[MAX_GROUPNAME + 1] = "";
      char lastGroupID[MAX_GROUPID + 1] = "";
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
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

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
        goto sendMessageToUser;
      }

      //Verificar o GID
			strcpy(group_ID, validarGroup_ID( t+1, message_received, group_ID));
			if(strcmp(group_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser subscription failed, invalid GID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RGS E_GRP\n");
				goto sendMessageToUser;
			}
			t = t + MAX_GROUPID + 1;

      //Verificar o GName
			strcpy(group_Name, validarGroup_Name( t+1, message_received, group_Name));
			if(strcmp(group_Name,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser subscription failed, invalid GName\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RGS E_GNAME\n");
				goto sendMessageToUser;
			}

      //Verifica se é suposto criar um grupo novo
      if (strcmp(group_ID, "00") == 0 ){

        strcpy(dirName, "GROUPS/");       // GROUPS/

        //Cria o nome do ficheiro
        strcpy(fileName, dirName);             // GROUPS/
        strcat(fileName, "lastGroupID.txt");   // GROUPS/lastGroupID.txt

        //Ler o id do ultimo grupo no ficheiro
        fp = fopen(fileName, "r+");
        int c, w = 0;
        while ((c = fgetc(fp)) != EOF){
          if(c == '\n') // se apanha um '\n'
            break;
          lastGroupID[w] = c;
          w++;
        }
        fclose(fp);

        int b = atoi(lastGroupID);

        //Verifica se ja existe o numero maximo de grupos
        if(b == 99){
          if (checkVerboseStatus() == true) {
            printf("\nUser subscription failed, reached limit number of groups\n");
            printf("Port: %s | Address: %s\n", portUser, addressUser);
          }
  				strcpy(message_sent,"RGS E_FULL\n");
  				goto sendMessageToUser;
  			}

        b++;
        if(b < 10 ){
          sprintf(aux,"%d",b);
          strcpy(group_ID, "0");
          strcat(group_ID, aux);
        }
        else if(b >= 10 && b <= 99)
         sprintf(group_ID,"%d",b);
        else // easter egg
          printf("General Kenobi !!");

        //Cria o nome da diretoria do grupo
        strcat(dirName, group_ID);        // GROUPS/gid

        //Cria a directoria do grupo
        errcode = mkdir(dirName,0777);
        if(errcode!=0) exit(1); //error

        //Escreve o id do grupo na lista de grupos
  			fp = fopen(fileName, "w+");
        fputs(group_ID, fp);
        fclose(fp);

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
        strcpy(fileName, "GROUPS/");           // GROUPS/
        strcat(fileName, group_ID);            // GROUPS/gid
        strcat(fileName, "/MSG/");             // GROUPS/gid/MSG
        strcat(fileName, "lastMessageID.txt"); // GROUPS/gid/MSG/uid.txt

        //Cria o ficheiro
        fp = fopen(fileName, "w+");  // Ficheiro que contem o id da ultima mensagem que foi posted
        fputs("0000",fp); // Inicializa o ficheiro com o id '00'
        fclose(fp);

        //Cria o nome do ficheiro
        strcpy(fileName, "GROUPS/");     // GROUPS/
        strcat(fileName, group_ID);      // GROUPS/gid
        strcat(fileName, "/");           // GROUPS/gid/
        strcat(fileName, user_ID);       // GROUPS/gid/uid
        strcat(fileName, ".txt");        // GROUPS/gid/uid.txt

        //Cria o ficheiro
        fp = fopen(fileName, "w+");  // Subscrever no grupo
        fclose(fp);

        if(checkVerboseStatus()){
  				printf("\nGroup %s created and subscribed by user %s\n", group_ID, user_ID);
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }

        strcpy(message_sent, "RGS NEW ");
        strcat(message_sent, group_ID);
        strcat(message_sent, "\n");
        goto sendMessageToUser;
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
        goto sendMessageToUser;
      }

      //Cria o nome do ficheiro
      strcpy(fileName, dirName);       // GROUPS/gid
      strcat(fileName, "/");           // GROUPS/gid/
      strcat(fileName, user_ID);       // GROUPS/gid/uid
      strcat(fileName, ".txt");        // GROUPS/gid/uid.txt

      //Cria o ficheiro
      fp = fopen(fileName, "w+");  // Subscribe no grupo
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

      char user_ID[MAX_USERID + 1] = "";
      char group_ID[MAX_GROUPID + 1] = "";
      char fileName[MAX_DIRECTORY + 1] = "";

      //Verificar o UID
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
			if(strcmp(user_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser unsubscription failed, invalid UID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RGU E_USR\n");
				goto sendMessageToUser;
			}
			t = t + MAX_USERID + 1;

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
        goto sendMessageToUser;
      }

      //Verificar o GID
			strcpy(group_ID, validarGroup_ID( t+1, message_received, group_ID));
			if(strcmp(group_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nUser unsubscription failed, invalid GID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RGU E_GRP\n");
				goto sendMessageToUser;
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
        goto sendMessageToUser;
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
        goto sendMessageToUser;
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
      char groupID[MAX_GROUPID + 1] = "";
      char aux[1 + 1] = "";
      char groupName[MAX_DIRECTORY + 1] = "";
      char user_ID[MAX_USERID + 1] = "";
      char lastMessageID[MAX_MESSAGEID + 1] = "";

      //Verificar o UID
      printf("%s\n",message_received );
			strcpy(user_ID, validarUser_ID( t+1, message_received, user_ID));
			if(strcmp(user_ID,"") == 0){
        if (checkVerboseStatus() == true) {
          printf("\nMy groups displayed failed, invalid UID\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
				strcpy(message_sent,"RGM E_USR\n");
				goto sendMessageToUser;
			}

      //Cria o nome do ficheiro
      strcpy(fileName, "USERS/");      // USERS/
      strcat(fileName, user_ID);       // USERS/uid
      strcat(fileName, "/");           // USERS/uid/
      strcat(fileName, user_ID);       // USERS/uid/uid
      strcat(fileName, "_login.txt");  // USERS/uid/uid_login.txt

      //Verificar se o user esta logged in
      if( access( fileName, R_OK ) != 0 ) {
        if (checkVerboseStatus() == true) {
          printf("\nMy groups displayed failed, UID not logged in\n");
          printf("Port: %s | Address: %s\n", portUser, addressUser);
        }
        strcpy(message_sent,"RGM E_USR\n");
        goto sendMessageToUser;
      }

      int i = 1;
      for( i; i <= MAX_GROUPS; i++){
        strcpy(dirName, "GROUPS/");         // GROUPS/

        if(i < 10 ){
          sprintf(aux,"%d",i);
          strcpy(groupID, "0");
          strcat(groupID, aux);
        }
        else if(i >= 10 && i <= 99)
         sprintf(groupID,"%d",i);
        else // easter egg
          printf("Sneaky Sneaky");

        strcat(dirName, groupID);          // GROUPS/gid

        //Verifica se o grupo existe
        if( access( dirName, R_OK ) != 0 ) {
            break;
        }

        strcpy(fileName, dirName);         // GROUPS/gid
        strcat(fileName, "/");             // GROUPS/gid/
        strcat(fileName, user_ID);          // GROUPS/gid/uid
        strcat(fileName, ".txt");          // GROUPS/gid/uid.txt

        //Verifica se o user esta subscribed no grupo
        if( access( fileName, R_OK ) == 0 ){

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

          //Cria o nome do ficheiro
          strcpy(fileName, dirName);             // GROUPS/gid
          strcat(fileName, "/MSG/");             // GROUPS/gid/MSG/
          strcat(fileName, "lastMessageID.txt"); // GROUPS/gid/MSG/lastMessageID.txt

          //Ler o id da ultima mensagem do grupo no ficheiro
          fp = fopen(fileName, "r+");
          int p, r = 0;
          while ((p = fgetc(fp)) != EOF){
            if(p == '\n') // se apanha um '\n'
              break;
            lastMessageID[r] = p;
            r++;
          }
          fclose(fp);

          strcat(groupsInfo, " ");            // ' '
          strcat(groupsInfo, groupID);        // ' gid'
          strcat(groupsInfo, " ");            // ' gid '
          strcat(groupsInfo, groupName);      // ' gid  gname'
          strcat(groupsInfo, " ");            // ' gid  gname '
          strcat(groupsInfo, lastMessageID);  // ' gid  gname  mid'
        }

      }

      //Vai buscar o groupID do ultimo grupo, que é igual ao numero de grupos existentes
      fp = fopen("GROUPS/lastGroupID.txt", "r");
      int c, w = 0;
      while ((c = fgetc(fp)) != EOF){
        if(c == '\n') // quando apanha o '\n'
          break;
        groupID[w] = c;
        w++;
      }
      fclose(fp);

      strcpy(taskInfo, "RGM ");             // 'RGL '
      strcat(taskInfo, groupID);            // 'RGL gid'

      if(checkVerboseStatus()){
				printf("\nGroups subscribed by user %s displayed\n", user_ID);
        printf("Port: %s | Address: %s\n", portUser, addressUser);
			}

      strcpy(message_sent, taskInfo);       // 'RGL gid'
      strcat(message_sent, groupsInfo);     // 'RGL gid[ gid  gname mid]*'
      strcat(message_sent, "\n");           // 'RGL gid[ gid  gname mid]*\n'

		}

		else if (!strcmp(tarefa,"ULS")){

		}

		else if (!strcmp(tarefa,"PST")){

		}

		else if (!strcmp(tarefa,"RTV")){

		}

	  else
	    strcpy(message_sent,"ERR");

    sendMessageToUser:

    if (strcmp(message_sent, "") == 0)
      strcpy(message_sent, "ERR");

		n=sendto(fd,message_sent,strlen(message_sent),0,(struct sockaddr*)&addr,addrlen);
		if(n==-1 )exit(1); //error

	}

	freeaddrinfo(res);
	close(fd);
	return 0;
}
