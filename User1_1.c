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
#define MAX_STATUS 3
#define MAX_TEXTO 240
#define MAX_UID 6
#define MAX_PASSWORD 9
#define MAX_IP 20
#define MAX_PORT 5
#define MAX_GROUPS 2 //maximo é 99 grupos
#define MAX_GROUPDISPLAY 3268  //tamanho de [ GID GName MID] = 1+2+1+24+1+4 | 99*33 +1


bool userLogStatus = false; // true = login , false = logout


/******* Validar o Port *******/
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


/******* Validar user_UID *******/
char* validarUser_UID(int i, char *comando, char *user_UID){
  for (int k = 0; comando[i] != ' '; i++,k++) {
      if (isdigit(comando[i]) > 0){
          user_UID[k] = comando[i];
      }
      else {
          printf("Erro no UID, tem que ser numerico\n");
          return  "";
      }
  }
  if (strlen(user_UID) != 5) {
      printf("Erro no UID, tem que ser tamanho 5\n");
      return  "";
  }
  return user_UID;
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


/******* Validar lista de grupos *******/
char* validarListaGrupos(int k, char *message_received, char* groups_list){
  char list_aux[3] = "";
  char list_aux2[4] = "";
  int i = 0;


  while(1){

    //PARTE 1
    for (int w = 0; message_received[k] != ' ' && isdigit(message_received[k]) > 0; i++, w++, k++) {
          list_aux[w] = message_received[k];
    }

    if( strlen(list_aux) != 2){
      return  "";
    }

    strcat(groups_list, list_aux);
    groups_list[i] = ' '; //adiciona espaço
    strcpy(list_aux,"");
    k++, i++;


    //PARTE 2
    for (i; message_received[k] != ' '; i++,k++) {
        if (isdigit(message_received[k]) > 0 || isalpha(message_received[k]) != 0 ||
            message_received[k] == '-' || message_received[k] == '_'){
          groups_list[i] = message_received[k];
        }
        else{
          return "";
        }
    }

    groups_list[i] = ' '; //adiciona espaço
    k++, i++;


    //PARTE 3
    for (int w = 0; message_received[k] != ' ' && isdigit(message_received[k]) > 0; i++, w++, k++) {
          list_aux2[w] = message_received[k];
    }

    if( strlen(list_aux2) != 4){
      return  "";
    }

    strcat(groups_list, list_aux2);
    groups_list[i] = '\n'; //adiciona espaço
    strcpy(list_aux2,"");

    if(message_received[k] == '\n'){
      return groups_list;
    }
    k++, i++;
  }

}


/******* Indica o status de Login/Logout *******/
bool checkUserLogStatus(){
  return userLogStatus;
}





int main (int argc, char *argv[]) {
    char address[MAX_IP];    // $$$$$$$$$$$$$$$$$$ ATENTION $$$$$$$$$$$$$$$$$
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

    errcode=getaddrinfo(address,port,&hints,&res);
    if(errcode!=0) {
        printf("Falha na verificação do IP\n");
        exit(1); //error
    }



    /******* Ciclo que fica à espera dos comandos do utilizador *******/
    while (1) {

        char comando[MAX_COMANDO] = "";  //input do user
        char tarefa[MAX_TAREFA] = "";  //tarefa a executar
        char message_sent[MAX_TEXTO] = "";  //mensagem enviada ao server
        char message_received[MAX_GROUPDISPLAY] = "";  //mensagem recebida pelo server

        if(checkUserLogStatus())
          printf("\n>>"); //loged in
        else
          printf("\n>"); //loged out

        fgets(comando,MAX_COMANDO,stdin);

        // le o tipo de tarefa que e para fazer do input
        int i = 0;
        for (i; isalpha(comando[i]) != 0; i++) {
            tarefa[i] = comando[i];
        }
        int msg = 3; // comeca no 0, logo 3 letras da tarefa + 1


        /* ----------------------------------------*/
        /*        Tarefa: registar utilizador      */
        /*        mensagem: >REG UID pass          */
        /* ----------------------------------------*/


        if (!strcmp(tarefa,"reg")){

            char user_UID[MAX_UID] = "";
            char password[MAX_PASSWORD] = "";
            char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server
            char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

            strcpy(message_sent,"REG"); //adicionar tarefa em maiusculas a mensagem
            message_sent[msg] = ' '; //adicionar espaço a mensagem

            //Verificar o UID
            strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
            if(strcmp(user_UID,"") == 0)
              continue;
            i=i+6, msg=msg+6; // 5 carateres do UID + 1
            strcat(message_sent,user_UID); //adicionar UID a mensagem
            message_sent[msg] = ' '; //adicionar espaço a mensagem

            //Verificar a password
            strcpy(password, validarPassword( i+1, comando, password));
            if(strcmp(password,"") == 0)
              continue;
            i=i+9, msg=msg+9; // 8 carateres da password + 1
            strcat(message_sent,password); //adicionar password a mensagem
            message_sent[msg] = '\n'; //adiciona o '\n' no final da mensagem

            //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

            // comunicacao com o server em UDP
            n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
            if(n==-1) exit(1); //error

            addrlen=sizeof(addr);
            n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
            if(n==-1) exit(1); //error

            // le o tipo de tarefa que o server respondeu
            int k = 0;
            for (; isalpha(message_received[k]) != 0; k++) {
                tarefa_res[k] = message_received[k];
            }

            //Verificar tarefa da resposta do servidor
            if (strcmp(tarefa_res, "RRG") == 0) {

              // le o tipo de status da tarefa que o server respondeu
              k++;
              for (int w = 0;  isalpha(message_received[k]) != 0; k++, w++) {
                  status_res[w] = message_received[k];
              }

              //Verificar status da resposta do servidor
              if (strcmp(status_res, "OK") == 0){
                printf("\nUser successfully registed\n");
              }
              else if(strcmp(status_res, "DUP") == 0) {
                printf("\nUser already registed\n");
              }
              else if(strcmp(status_res, "NOK") == 0){
                printf("\nError on registation\n");
              }
              else{
                printf("\nErro nas Mensagens do Protocolo\n");
              }
            }
            else{
              printf("\nErro no Protocolo\n");
            }

        }


        /* ----------------------------------------*/
        /*        Tarefa: Apagar utilizador        */
        /*        mensagem: >UNR UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"unregister") || !strcmp(tarefa,"unr")) {

          char user_UID[MAX_UID] = "";
          char password[MAX_PASSWORD] = "";
          char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          strcpy(message_sent,"UNR"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          if(strcmp(user_UID,"") == 0)
            continue;
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          if(strcmp(password,"") == 0)
            continue;
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = '\n'; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          // le o tipo de tarefa que o server respondeu
          int k = 0;
          for (; isalpha(message_received[k]) != 0; k++) {
              tarefa_res[k] = message_received[k];
          }

          //Verificar tarefa da resposta do servidor
          if (strcmp(tarefa_res, "RUN") == 0) {

            // le o tipo de status da tarefa que o server respondeu
            k++;
            for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
                status_res[w] = message_received[k];
            }

            //Verificar status da resposta do servidor
            if (strcmp(status_res, "OK") == 0){
              printf("\nUser successfully unregisted\n");
            }
            else if(strcmp(status_res, "NOK") == 0){
              printf("\nError on unregistation\n");
            }
            else{
              printf("\nErro nas Mensagens do Protocolo\n");
            }
          }
          else{
            printf("\nErro no Protocolo\n");
          }

        }


        /* ----------------------------------------*/
        /*        Tarefa: login                    */
        /*        mensagem: >LOG UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"login")){

          char user_UID[MAX_UID] = "";
          char password[MAX_PASSWORD] = "";
          char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          strcpy(message_sent,"LOG"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          if(strcmp(user_UID,"") == 0)
            continue;
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          if(strcmp(password,"") == 0)
            continue;
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = '\n'; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          // le o tipo de tarefa que o server respondeu
          int k = 0;
          for (; isalpha(message_received[k]) != 0; k++) {
              tarefa_res[k] = message_received[k];
          }

          //Verificar tarefa da resposta do servidor
          if (strcmp(tarefa_res, "RLO") == 0) {

            // le o tipo de status da tarefa que o server respondeu
            k++;
            for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
                status_res[w] = message_received[k];
            }

            //Verificar status da resposta do servidor
            if (strcmp(status_res, "OK") == 0){
              printf("\nYou are now Logged in\n");
              userLogStatus = true;
            }
            else if(strcmp(status_res, "NOK") == 0){
              printf("\nError on Login\n");
            }
            else{
              printf("\nErro nas Mensagens do Protocolo\n");
            }
          }
          else{
            printf("\nErro no Protocolo\n");
          }

        }


        /* ----------------------------------------*/
        /*        Tarefa: logout                   */
        /*        mensagem: >OUT UID pass          */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"logout")){

          char user_UID[MAX_UID] = "";
          char password[MAX_PASSWORD] = "";
          char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          strcpy(message_sent,"OUT"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar o UID
          strcpy(user_UID, validarUser_UID( i+1, comando, user_UID));
          if(strcmp(user_UID,"") == 0)
            continue;
          i=i+6, msg=msg+6; // 5 carateres do UID + 1
          strcat(message_sent,user_UID); //adicionar UID a mensagem
          message_sent[msg] = ' '; //adicionar espaço a mensagem

          //Verificar a password
          strcpy(password, validarPassword( i+1, comando, password));
          if(strcmp(password,"") == 0)
            continue;
          i=i+9, msg=msg+9; // 8 carateres da password + 1
          strcat(message_sent,password); //adicionar password a mensagem
          message_sent[msg] = '\n'; //adiciona o '\n' no final da mensagem

          //printf("uid: %s | pass: %s | message_sent: %s\n",user_UID, password, message_sent); //APAGAR FUTURO

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,128,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          // le o tipo de tarefa que o server respondeu
          int k = 0;
          for (; message_received[k] != ' ' || isalpha(message_received[k]) != 0; k++) {
              tarefa_res[k] = message_received[k];
          }

          //Verificar tarefa da resposta do servidor
          if (strcmp(tarefa_res, "ROU") == 0) {

            // le o tipo de status da tarefa que o server respondeu
            k++;
            for (int w = 0; message_received[k] != '\n' || isalpha(message_received[k]) != 0; k++, w++) {
                status_res[w] = message_received[k];
            }

            //Verificar status da resposta do servidor
            if (strcmp(status_res, "OK") == 0){
              printf("\nYou are now Logged out\n");
              userLogStatus = false;
            }
            else if(strcmp(status_res, "NOK") == 0){
              printf("\nError on Logout\n");
            }
            else{
              printf("\nErro nas Mensagens do Protocolo\n");
            }
          }
          else{
            printf("\nErro no Protocolo\n");
          }

        }


        /* ----------------------------------------*/
        /*        Tarefa: groups                   */
        /*        mensagem: >GLS                   */
        /* ----------------------------------------*/

        else if (!strcmp(tarefa,"groups") || !strcmp(tarefa,"gl")){

          char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server
          char num_groups[MAX_GROUPS] = ""; // numero de grupos respondido pelo server
          char groups_list[MAX_GROUPDISPLAY] = ""; // lista de grupos

          strcpy(message_sent,"GLS"); //adicionar tarefa em maiusculas a mensagem
          message_sent[msg] = '\n'; //adicionar espaço a mensagem

          // comunicacao com o server
          n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
          if(n==-1) exit(1); //error

          addrlen=sizeof(addr);
          n=recvfrom(fd,message_received,3267,0,(struct sockaddr*)&addr,&addrlen);
          if(n==-1) exit(1); //error

          // le o tipo de tarefa que o server respondeu
          int k = 0;
          for (; isalpha(message_received[k]) != 0; k++) {
              tarefa_res[k] = message_received[k];
          }

          //Verificar tarefa da resposta do servidor
          if (strcmp(tarefa_res, "RGL") == 0) {

            // le quantos grupos o server respondeu
            k++;
            for (int w = 0; isdigit(message_received[k]) > 0 ; k++, w++) {
                num_groups[w] = message_received[k];
            }

            int number = atoi(num_groups);

            //Verificar numero de grupos da resposta do servidor
            if (0 < number < 100){ // existe grupos

              //Verificar se lista de grupos cumpre Protocolo
              k++;
              strcpy(groups_list,validarListaGrupos(k, message_received, groups_list));
              if(strcmp(groups_list, "") == 0)
                printf("\nErro na Lista de Grupos\n");
              else
                printf("\n%s", groups_list);

            }
            else if (strcmp(num_groups, "")){ // se num_groups = "" -> mumber = 0
              printf("\nErro nas Mensagens do Protocolo\n");
            }
            else if (number == 0){
              printf("\nNo groups available\n");
            }
          }
          else{
            printf("\nErro no Protocolo\n");
          }

        }



        //Apenas depois do login
        else if ( (!strcmp(tarefa,"subscribe") || !strcmp(tarefa,"s") ) && checkUserLogStatus()){ //subscribe GID GName
          
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
