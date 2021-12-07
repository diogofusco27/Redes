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

#define MAX_COMANDO 500
#define MAX_TAREFA 20
#define MAX_TEXTO 124
#define MAX_UID 6
#define MAX_PASSWORD 9
#define MAX_IP 20
#define MAX_PORT 5

/******* Validar o Port *******/
bool validarPort (char* argv) {
    int len = strlen(argv);
    if (len != 5)
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

/******* Validar user_UID *******/
char* validarUser_UID(int i, char *comando, char *user_UID){
  for (int k = 0; comando[i] != ' '; i++,k++) {
      if (isdigit(comando[i]) > 0){
          user_UID[k] = comando[i];
      }
      else {
          printf("Erro no UID, tem que ser numerico\n");
          exit(1);
      }
  }
  if (strlen(user_UID) != 5) {
      printf("Erro no UID, tem que ser 5 digitos\n");
      exit(1);
  }
  else{
    return user_UID;
  }
  return user_UID; //nunca vem aqui
}

/******* Validar Password *******/
char* validarPassword(int i, char *comando, char *password){
  for (int k = 0; comando[i] != ' ' && comando[i] !='\n' && comando[i] !='\0';i++, k++) {
      if (isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0){ //isalnum() estava a dar erro
          password[k] = comando[i];
      }
      else {
          printf("Erro na password, tem que ser alphanumerico\n");
          exit(1);
      }
  }
  if (strlen(password) != 8) {
      printf("Erro na password, tem que ser 8 digitos\n");
      exit(1);
  }
  else{
    return password;
  }
  return password; //nunca vem aqui
}





int main (int argc, char *argv[]) {
    char address[MAX_IP];
    char port[MAX_PORT] = "58021";

    /******* Validar input inicial do utilizador *******/
    if (argc == 3) {
        if (strcmp(argv[1],"-n") == 0) { // IP
            strcpy(address,argv[2]);
        } else if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2])) { // Port
            strcpy(port,argv[2]);
        } else {
            printf("Input errado\n");
            exit(1);
        }
    }
    else if (argc == 5){
        if (strcmp(argv[1],"-n") == 0 && strcmp(argv[3],"-p") == 0 && validarPort(argv[4])) {
            strcpy(address,argv[2]);
            strcpy(port,argv[4]);
        } else if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2]) && strcmp(argv[3],"-n") == 0) {
            strcpy(port,argv[2]);
            strcpy(address,argv[4]);
        } else {
            printf("Input errado\n");
            exit(1);
        }
    }
    else if (argc != 1) {
        printf("Input errado\n");
        exit(1);
    }
		//printf("ip: %s || port: %s\n", address, port);  //APAGAR FUTURO


    /******* Inicializar conexao UDP *******/
    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;

    fd=socket(AF_INET,SOCK_DGRAM,0);    //UDP socket
    if(fd==-1) exit(1); //error

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;            //IPv4
    hints.ai_socktype=SOCK_DGRAM;       //UDP socket

    //errcode=getaddrinfo("tejo.tecnico.ulisboa.pt","58011",&hints,&res); //APAGAR FUTURO
    errcode=getaddrinfo(address,port,&hints,&res);
    if(errcode!=0) {
        printf("Falha na verificação do IP\n");
        exit(1); //error
    }

		//FILE *fp;


    /******* Ciclo que fica à espera dos comandos do utilizador *******/
    while (1) {
        char comando[MAX_COMANDO] = "";  //input do user
        char tarefa[MAX_TAREFA] = "";  //tarefa a executar
        char message_sent[MAX_TEXTO] = "";  //mensagem enviada ao server
        char message_received[MAX_TEXTO] = "";  //mensagem recebida pelo server
        char user_UID[MAX_UID] = "";
        char password[MAX_PASSWORD] = "";

				/* opening file for reading */
			  /*fp = fopen("file.txt" , "r");
			  if(fp == NULL) {
			     perror("Error opening file");
			     return(-1);
			  }*/

        printf("\n>");
        fgets(comando,MAX_COMANDO,stdin);
        //fgets(comando,MAX_COMANDO,fp);
        //fclose(fp);

        // le o tipo de tarefa que e para fazer
        int i = 0;
        for (i; comando[i] != ' ';i++) {
            tarefa[i] = comando[i];
        }
        int msg = 3; // comeca no 0, logo 3 letras da tarefa + 1


        /* ----------------------------------------*/
        /*        Tarefa: registar utilizador      */
        /*        mensagem: >REG UID pass          */
        /* ----------------------------------------*/

        if (!strcmp(tarefa,"reg")){

            strcpy(message_sent,"REG"); //adicionar tarefa em maiusculas a mensagem
            message_sent[msg] = ' '; //adicionar espaço a mensagem

            //Verificar o UID
            strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
            i=i+6, msg=msg+6; // 5 carateres do UID + 1
            strcat(message_sent,user_UID); //adicionar UID a mensagem
            message_sent[msg] = ' '; //adicionar espaço a mensagem

            //Verificar a password
            strcpy(password, validarPassword( i+1, comando, password));
            i=i+9, msg=msg+9; // 8 carateres da password + 1
            strcat(message_sent,password); //adicionar password a mensagem
            message_sent[msg] = comando[i]; //adiciona o '\n' no final da mensagem

            //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

            // comunicacao com o server
            n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
            if(n==-1) exit(1); //error

            addrlen=sizeof(addr);
            n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1) exit(1); //error

            //Verificar resposta do servidor
            if (strcmp(message_received, "RRG OK\n") == 0) {
              printf("\nUser successfully registed\n");
            }
            else if(strcmp(message_received, "RRG DUP\n") == 0) {
              printf("\nUser already registed\n");
            }
            else if(strcmp(message_received, "RRG NOK\n") == 0){
              printf("\nError on server side\n");
            }
            else{
              printf("\nnao devia vir aqui: %s\n", message_sent);
            }

        }


        /* ----------------------------------------*/
        /*        Tarefa: Apagar utilizador        */
        /*        mensagem: >UNR UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"unregister") || !strcmp(tarefa,"unr")) {

          strcpy(message_sent,"UNR"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = comando[i]; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          //Verificar resposta do servidor
          if (strcmp(message_received, "RUN OK\n") == 0) {
            printf("\nUser successfully deleted\n");
          }
          else if(strcmp(message_received, "RUN NOK\n") == 0) {
            printf("\nIncorrect user ID or password\n");
          }
          else{
            printf("\nnao devia vir aqui: %s\n", message_sent);
          }

        }


        /* ----------------------------------------*/
        /*        Tarefa: login                    */
        /*        mensagem: >LOG UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"login")){

          strcpy(message_sent,"LOG"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = comando[i]; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          //Verificar resposta do servidor
          if (strcmp(message_received, "RLO OK\n") == 0) {
            printf("\nYou are now Logged in\n");
          }
          else if(strcmp(message_received, "RLO NOK\n") == 0){
            printf("\nIncorrect user ID or password or Error on server\n");
          }
          else{
            printf("\nnao devia vir aqui: %s\n", message_sent);
          }

        }


        /* ----------------------------------------*/
        /*        Tarefa: logout                   */
        /*        mensagem: >OUT UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"logout")){

          strcpy(message_sent,"OUT"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = comando[i]; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          //Verificar resposta do servidor
          if (strcmp(message_received, "ROU OK\n") == 0) {
            printf("\nYou are now Logged out\n");
          }
          else if(strcmp(message_received, "ROU NOK\n") == 0){
            printf("\nIncorrect user ID or password or Error on server\n");
          }
          else{
            printf("\nnao devia vir aqui: %s\n", message_sent);
          }

        }



        else if (!strcmp(tarefa,"groups") || !strcmp(tarefa,"gl")){ //groups

        }
        //Apenas depois do login
        else if (!strcmp(tarefa,"subscribe") || !strcmp(tarefa,"s")){ //subscribe GID GName

        }
        else if (!strcmp(tarefa,"unsubscribe") || !strcmp(tarefa,"u")){ //unsubscribe GID

        }
        else if (!strcmp(tarefa,"my_groups") || !strcmp(tarefa,"mgl")){ //my_groups

        }
        else if (!strcmp(tarefa,"select") || !strcmp(tarefa,"sag")){ //select GID

        }
        //TCP server
        else if (!strcmp(tarefa,"ulist") || !strcmp(tarefa,"ul")){ //ulist

        }
        //message commands
        else if (!strcmp(tarefa,"post")){ //post “text” [Fname]

        }
        else if (!strcmp(tarefa,"retrieve") || !strcmp(tarefa,"r")){ //retrieve MID

        }
        else if (!strcmp(tarefa,"exit")) { //exit
            break;
        }
        else {
          printf("Comando não suportado! Tente novamente!\n");
          continue;
        }


    }

    freeaddrinfo(res);
    close(fd);
}
