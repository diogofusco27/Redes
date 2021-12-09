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
#define MAX_STATUS 5
#define MAX_TEXTO 240
#define MAX_USERID 5
#define MAX_IP 30
#define MAX_PORT 5
#define MAX_GROUPID 2 //maximo é 99 grupos
#define MAX_MESSAGEID 3
#define MAX_GROUPDISPLAY 3368  //tamanho de [ GID GName MID] = 1+2+1+24+1+4 | 99*33 +1


bool userLogStatus = false; // true = login , false = logout
char userLogID[MAX_USERID + 1] = "";
char activeGroup[MAX_GROUPID + 1] = "";


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

/******* Cria a mensagem que vai ser enviada ao server *******/
char* createMessage(char *comando, char *message){

  char tarefa[MAX_TAREFA] = "";  //tarefa a executar

  // le o tipo de tarefa que e para fazer do input
  int i = 0;
  for (i; isalpha(comando[i]) != 0; i++) {
      tarefa[i] = comando[i];
  }

  //adicionar tarefa em maiusculas a mensagem
  if (!strcmp(tarefa,"reg"))
    strcpy(message,"REG");
  else if (!strcmp(tarefa,"unregister") || !strcmp(tarefa,"unr"))
    strcpy(message,"UNR");
  else if (!strcmp(tarefa,"login"))
    strcpy(message,"LOG");
  else if (!strcmp(tarefa,"logout"))
    strcpy(message,"OUT");
  else if (!strcmp(tarefa,"groups") || !strcmp(tarefa,"gl"))
    strcpy(message,"GLS");
  else if (!strcmp(tarefa,"subscribe") || !strcmp(tarefa,"s"))
    strcpy(message,"GSR");
  else if (!strcmp(tarefa,"unsubscribe") || !strcmp(tarefa,"u"))
    strcpy(message,"GUR");
  else if (!strcmp(tarefa,"my_groups") || !strcmp(tarefa,"mgl"))
    strcpy(message,"GLM");
  else if (!strcmp(tarefa,"select") || !strcmp(tarefa,"sag"))
    strcpy(message,"SAG");
  else if (!strcmp(tarefa,"ulist") || !strcmp(tarefa,"ul"))
    strcpy(message,"ULS");
  else if (!strcmp(tarefa,"post"))
    strcpy(message,"PST");
  else if (!strcmp(tarefa,"retrieve") || !strcmp(tarefa,"r"))
    strcpy(message,"RTV");
  else // inclui se a tarefa for 'exit'
    return comando;

  int k = 3;
  message[k] = ' '; //adicionar espaço a mensagem
  i++, k++;

  // copia o resto do comando para a mensagem
  for (; comando[i] !='\n'; i++, k++) {
      message[k] = comando[i];
  }

  message[k] = '\n'; //adiciona o '\n' no final da mensagem
  return message;

}


/******* Validar lista de grupos *******/
char* validarListaGrupos(int k, char *message_received, char* groups_list){
  char list_aux[MAX_GROUPID + 1] = "";
  char list_aux3[MAX_MESSAGEID + 1] = "";
  int i = 0;

  while(1){ // formato -> GID GName MID  | GID=Parte 1 , GName=Parte 2, MID=Parte 3

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
    for (i; message_received[k] != ' '; i++, k++) {
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
          list_aux3[w] = message_received[k];
    }

    if( strlen(list_aux3) != 4){
      return  "";
    }

    strcat(groups_list, list_aux3);
    groups_list[i] = '\n'; //adiciona espaço
    strcpy(list_aux3,"");

    //verifica se esta no fim da lista
    if(message_received[k] == '\n'){
      return groups_list;
    }
    k++, i++;
  }

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


/******* Indica o status de Login/Logout *******/
bool checkUserLogStatus(){
  return userLogStatus;
}






int main (int argc, char *argv[]) {

    char address[MAX_IP] = "127.0.0.1";
    char port[MAX_PORT] = "58021";

    /******* Validar input inicial do utilizador *******/
    if (argc == 3) {
        if (strcmp(argv[1],"-n") == 0) { // IP
            strcpy(address,argv[2]);
        }
        else if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2])) { // Port
            strcpy(port,argv[2]);
        }
        else {
            printf("Input errado\n");
            exit(1);
        }
    }
    else if (argc == 5){
        if (strcmp(argv[1],"-n") == 0 && strcmp(argv[3],"-p") == 0 && validarPort(argv[4])) {
            strcpy(address,argv[2]);
            strcpy(port,argv[4]);
        }
        else if (strcmp(argv[1],"-p") == 0 && validarPort(argv[2]) && strcmp(argv[3],"-n") == 0) {
            strcpy(port,argv[2]);
            strcpy(address,argv[4]);
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
        char message_sent[MAX_TEXTO] = "";  //mensagem enviada ao server
        char message_received[MAX_GROUPDISPLAY] = "";  //mensagem recebida do server
        char tarefa[MAX_TAREFA] = "";  //tarefa a executar
        char tarefa_res[MAX_TAREFA] = ""; // tarefa respondida pelo server

        if(checkUserLogStatus())
          printf("\n>>"); //loged in
        else
          printf("\n>"); //loged out

        fgets(comando,MAX_COMANDO,stdin);

        // cria a mensagem para enviar ao server
        strcpy(message_sent, createMessage(comando, message_sent));

        // verifica se é para encerrar o User
        if (!strcmp(message_sent,"exit\n")) {
          freeaddrinfo(res);
          close(fd);
          exit(1);
        }

        // le o tipo de tarefa da mensagem
        int t = 0;
        for (t; isalpha(message_sent[t]) != 0; t++) {
            tarefa[t] = message_sent[t];
        }

        // Verifica se tarefa é select
        if (!strcmp(tarefa,"SAG")){
          if (checkUserLogStatus()){
            strcpy(activeGroup, validarGroup_ID( t+1, message_sent, activeGroup));
            if(strcmp(activeGroup,"") == 0)
              continue;
            printf("Group %s selected\n", activeGroup );
            continue;
          }
          else{
            printf("Select failed\n");
            continue;
          }
        }

        // comunicacao com o server em UDP
        n=sendto(fd,message_sent,strlen(message_sent),0,res->ai_addr,res->ai_addrlen);
        if(n==-1) exit(1); //error

        addrlen=sizeof(addr);
        n=recvfrom(fd,message_received,MAX_GROUPDISPLAY,0,(struct sockaddr*)&addr,&addrlen);
        if(n==-1) exit(1); //error

        // le o tipo de tarefa que o server respondeu
        int k = 0;
        for (; isalpha(message_received[k]) != 0; k++) {
            tarefa_res[k] = message_received[k];
        }


        /* ----------------------------------------- */
        /*        Tarefa: registar utilizador        */
        /*        comando: >REG UID pass             */
        /*        resposta: >RRG status              */
        /* ----------------------------------------- */


        if (strcmp(tarefa_res, "RRG") == 0) {

            char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

            // Tipo de status da tarefa que o server respondeu
            k++;
            for (int w = 0;  isalpha(message_received[k]) != 0; k++, w++) {
                status_res[w] = message_received[k];
            }

            // Verificar status da resposta do servidor
            if (strcmp(status_res, "OK") == 0){
              printf("User successfully registed\n");
            }
            else if(strcmp(status_res, "DUP") == 0) {
              printf("User already registed\n");
            }
            else if(strcmp(status_res, "NOK") == 0){
              printf("Registation failed\n");
            }
            else{
              printf("Erro nas Mensagens do Protocolo\n");
            }

        }


        /* ----------------------------------------- */
        /*        Tarefa: Apagar utilizador          */
        /*        comando: >UNR UID pass             */
        /*        resposta: >RUN status              */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "RUN") == 0) {

          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
          k++;
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("User successfully unregisted\n");
          }
          else if(strcmp(status_res, "NOK") == 0){
            printf("Unregistation failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: login                      */
        /*        comando: >LOG UID pass             */
        /*        resposta: >RLO status              */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "RLO") == 0) {

          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
          k++;
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("You are now Logged in\n");
            userLogStatus = true;
            t++;
            for (int i = 0; isdigit(message_sent[t]) > 0; i++, t++){
              userLogID[i] = message_sent[t];
            }
          }
          else if(strcmp(status_res, "NOK") == 0){
            printf("Login failed\n");
            strcpy(userLogID,"");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
            strcpy(userLogID,"");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: logout                     */
        /*        comando: >OUT UID pass             */
        /*        resposta: >ROU status              */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "ROU") == 0) {

          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // Le o tipo de status da tarefa que o server respondeu
          k++;
          for (int w = 0; message_received[k] != '\n' || isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("You are now Logged out\n");
            userLogStatus = false;
            strcpy(userLogID,"");
          }
          else if(strcmp(status_res, "NOK") == 0){
            printf("Logout failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: groups                     */
        /*        comando: >GLS                      */
        /*        resposta: >RGL N[ GID GName MID]*  */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "RGL") == 0) {

          char num_groups[MAX_GROUPID + 1] = ""; // numero de grupos respondido pelo server
          char groups_list[MAX_GROUPDISPLAY] = ""; // lista de grupos

          // le quantos grupos o server respondeu
          k++;
          for (int w = 0; isdigit(message_received[k]) > 0 ; k++, w++) {
              num_groups[w] = message_received[k];
          }

          //Verificar numero de grupos da resposta do servidor
          if (0 < atoi(num_groups) < 100){ // existe grupos

            //Verificar se lista de grupos cumpre Protocolo
            k++;
            strcpy(groups_list,validarListaGrupos(k, message_received, groups_list));
            if(strcmp(groups_list, "") == 0)
              printf("Erro na Lista de Grupos\n");
            else
              printf("Grupos:\n%s", groups_list);
          }
          else if (strcmp(num_groups,"0")){
            printf("No groups available\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }



        /******* Apenas depois do login ********/


        /* ----------------------------------------- */
        /*        Tarefa: subscribe                  */
        /*        comando: >GSR UID GID GName        */
        /*        resposta: >RGS status              */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "RGS") == 0) {

          char group_ID_res[MAX_GROUPID + 1] = "";
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
          k++;
          for (int w = 0; isalpha(message_received[k]) != 0 || message_received[k] == '_'; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("Group subscribed\n");
          }
          else if (strcmp(status_res, "NEW") == 0){

            //Verificar a group ID recebido
            strcpy(group_ID_res, validarGroup_ID( k+1, message_received, group_ID_res));
            if(strcmp(group_ID_res,"") == 0){
              printf("Erro no group ID devolvido pelo Server\n");
            }
            else{
              printf("Group %s created and subscribed\n", group_ID_res);
            }
          }
          else if (strcmp(status_res, "E_USR") == 0){
            printf("Invalid user ID\n");
          }
          else if (strcmp(status_res, "E_GRP") == 0){
            printf("Invalid group ID\n");
          }
          else if (strcmp(status_res, "E_GNAME") == 0){
            printf("Invalid group Name\n");
          }
          else if (strcmp(status_res, "E_FULL") == 0){
            printf("No more space to create groups\n");
          }
          else if (strcmp(status_res, "NOK") == 0){
            printf("Group subscribed failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: unsubscribe                */
        /*        comando: >GUR UID GID              */
        /*        resposta: >RGU status              */
        /* ----------------------------------------- */


        else if (strcmp(tarefa_res, "RGU") == 0) {

          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
          k++;
          for (int w = 0; isalpha(message_received[k]) != 0 || message_received[k] == '_'; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("Group unsubscribed\n");
          }
          else if (strcmp(status_res, "E_USR") == 0){
            printf("Invalid user ID\n");
          }
          else if (strcmp(status_res, "E_GRP") == 0){
            printf("Invalid group ID\n");
          }
          else if (strcmp(status_res, "NOK") == 0){
            printf("Group unsubscribed failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: my groups                  */
        /*        comando: >GLM UID                  */
        /*        resposta: >RGM N[ GID GName MID]*  */
        /* ----------------------------------------- */

        else if (strcmp(tarefa_res, "RGM") == 0) {

          char num_groups[MAX_GROUPID + 1] = ""; // numero de grupos respondido pelo server
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server
          char groups_list[MAX_GROUPDISPLAY] = ""; // lista de grupos


          // le quantos grupos o server respondeu ou status
          k++;
          for (int w = 0; isdigit(message_received[k]) > 0 ||  isalpha(message_received[k]) != 0; k++, w++) {
              if(isdigit(message_received[k]) > 0)
                num_groups[w] = message_received[k];
              if(isalpha(message_received[k]) != 0)
                status_res[w] = message_received[k];
          }

          //Verificar numero de grupos da resposta do servidor
          if (0 < atoi(num_groups) < 100){ // existe grupos

            //Verificar se lista de grupos cumpre Protocolo
            k++;
            strcpy(groups_list,validarListaGrupos(k, message_received, groups_list));
            if(strcmp(groups_list, "") == 0)
              printf("Erro na Lista de Grupos\n");
            else
              printf("Grupos:\n%s", groups_list);

          }
          else if (strcmp(num_groups, "0")){
            printf("No groups subscribed\n");
          }
          else if (strcmp(status_res, "E_USR") == 0){
            printf("Invalid user ID\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }



        //TCP server
        else if (strcmp(tarefa_res, "ULS") == 0){ //ulist

        }


        else if (strcmp(tarefa_res, "PST") == 0){ //post “text” [Fname]

        }


        else if (strcmp(tarefa_res, "RTV") == 0){ //retrieve MID

        }


        else if (!strcmp(tarefa_res,"ERR")) {
          printf("\nErro no Protocolo\n");
        }
    }
    return 0;
}
