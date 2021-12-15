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
#include <signal.h>

#define MAX_COMANDO 500
#define MAX_TAREFA 3
#define MAX_TAREFA_COMANDO 11
#define MAX_STATUS 5
#define MAX_TEXTO 240
#define MAX_USER_ID 5
#define MAX_USERS 100000
#define MAX_PASSWORD 8
#define MAX_IP 30
#define MAX_PORT 5
#define MAX_GROUP_ID 2 // 99 grupos
#define MAX_MESSAGE_ID 4
#define MAX_MESSAGES 9999
#define MAX_GROUP_NAME 24
#define MAX_FILE_NAME 24

// 'GID - GNAME MID\n' = 2+3+24+1+4+1 = 35
// NUMEROdeGRUPOS = 99               35*99 = 3465
#define MAX_DISPLAY 3465

// 'TAREFA UID GID GNAME\n'= 3+1+5+1+2+1+24 = 37 + '\n' = 38
#define MAX_MESSAGE_UDP_SENT 38

// 'TAREFA UID GID Tsize text [Fname Fsize data]' =
//  3+1+5+1+2+1+3+1+240+1+[24+1+10+1+data] = 294 + data + '\n' = 295 + data
#define MAX_MESSAGE_TCP_SENT 295

// 'TAREFA GID' = 3+1+2 = 6
// ' GID GNAME MID' = 1+2+1+24+1+4 = 33
// NUMEROdeGRUPOS = 99                6+33*99 = 3273 + '\n' = 3274
#define MAX_MESSAGE_UDP_RECEIVED 3274

// 'TAREFA status' = 3+1+5 = 9   [ GName[ UID]*] = 1+24+(1+5)*100000 = 600025
//  9 + 600025 +'\n' = 600035
#define MAX_MESSAGE_TCP_RECEIVED 600035

// 'TAREFA status N' = 3+1+3+1+4 = 12          12 + (294 * 20) = 5892
// [ MID UID Tsize text [/ Fname Fsize data]] = 1+4+1+5+1+3+1+240+[1+1+24+1+10+1+data] = 294



bool conectionUDP = true; // true = conexao UDP , false = conexao TCP
bool userLogStatus = false; // true = logged in , false = not logged in
bool shutDownTime = false; // true = someone introduced 'exit' command

char userLogID[MAX_USER_ID + 1] = "";
char userLogPass[MAX_PASSWORD + 1] = "";
char activeGroup[MAX_GROUP_ID + 1] = "";


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


/******* Indica o status de Login/Logout *******/
bool checkUserLogStatus(){
  return userLogStatus;
}


/******* Cria a mensagem que vai ser enviada ao server *******/
char* createMessage(char *comando, char *message){

  bool taskRequiresLogin = false;
  char tarefa[MAX_TAREFA_COMANDO + 1] = "";  //tarefa a executar
  char text[MAX_TEXTO + 1] = ""; // texto do post
  char textSize[3 + 1] = ""; // quantos caracteres tem o texto
  char fileName[MAX_FILE_NAME] = ""; // nome do ficheiro
  char messageID[MAX_MESSAGE_ID] = "";

  // le o tipo de tarefa que e para fazer do comando
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
    return "GLS\n";
  else if (!strcmp(tarefa,"exit"))
    return "exit";
  else if (!strcmp(tarefa,"subscribe") || !strcmp(tarefa,"s")){
    strcpy(message,"GSR");
    taskRequiresLogin = true;
  }
  else if (!strcmp(tarefa,"unsubscribe") || !strcmp(tarefa,"u")){
    strcpy(message,"GUR");
    taskRequiresLogin = true;
  }
  else if (!strcmp(tarefa,"my_groups") || !strcmp(tarefa,"mgl")){
    strcpy(message,"GLM");
    taskRequiresLogin = true;
  }
  else if (!strcmp(tarefa,"select") || !strcmp(tarefa,"sag")){
    strcpy(message,"SAG");
    taskRequiresLogin = true;
  }
  else if (!strcmp(tarefa,"ulist") || !strcmp(tarefa,"ul")){
    strcpy(message,"ULS");
    taskRequiresLogin = true;
    conectionUDP = false; // Comunica com o server por TCP
  }
  else if (!strcmp(tarefa,"post")){
    strcpy(message,"PST");
    taskRequiresLogin = true;
    conectionUDP = false; // Comunica com o server por TCP
  }
  else if (!strcmp(tarefa,"retrieve") || !strcmp(tarefa,"r")){
    strcpy(message,"RTV");
    taskRequiresLogin = true;
    conectionUDP = false; // Comunica com o server por TCP
  }
  else
    return comando;

  //verifica se o user esta logged in, pois a tarefa necessita de login
  if (taskRequiresLogin == true && checkUserLogStatus() == false){
    strcpy(message,"OFF");
    return message;
  }

  //se a tarefa é um ulist, post ou retrieve, verifica o activeGroup e cria a mensagem
  if (strcmp(message, "ULS") == 0 && strcmp(activeGroup, "") != 0){ // ulist
    strcat(message, " ");           // 'ULS '
    strcat(message, activeGroup);   // 'ULS activeGroup'
    strcat(message, "\n");          // 'ULS activeGroup\n'
    return message;
  }
  else if(strcmp(message, "PST") == 0 && strcmp(activeGroup, "") != 0){ // post
    strcat(message, " ");           // 'PST '
    strcat(message, userLogID);     // 'PST userID'
    strcat(message, " ");           // 'PST userID '
    strcat(message, activeGroup);   // 'ULS userID activeGroup'
    strcat(message, " ");           // 'ULS userID activeGroup '

    // copia o texto
    i = i + 2;  //para saltar as '"' e começar no 1º caracter do texto
    int k = 1;
    text[0] = '"';
    for (k ; comando[i] != '"' ; i++, k++) { // termina quando encontrar a outra '"'
        text[k] = comando[i];
    }
    text[k] = '"';

    // ATENTION tamanho do texto e so confirmado no server?
    if(strlen(text) > MAX_TEXTO){ // quer dizer que o texto introduzido é maior do que 240 caracteres
      strcpy(message, "TXT");
      return message;
    }

    //tamanho do texto
    sprintf(textSize, "%zu", strlen(text));

    strcat(message, textSize);      // 'ULS UID activeGroup textSize'
    strcat(message, " ");           // 'ULS UID activeGroup textSize '
    strcat(message, text);          // 'ULS UID activeGroup textSize text'

    //verifica se existe um ficheiro
    i = i + 2;  //para saltar as '"' e começar no 1º caracter do nome do ficheiro
    if(isdigit(comando[i]) > 0 || isalpha(comando[i]) != 0 ||
               comando[i] == '-' || comando[i] == '_' || comando[i] == '.'){ // existe ficheiro

        // ATENTION
        // FAZ COISAS COM O FICHEIRO

    }
    else{ // nao existe ficheiro
      strcat(message, "\n"); // 'ULS UID activeGroup textSize text\n'
      return message;
    }
  }
  else if (strcmp(message, "RTV") == 0 && strcmp(activeGroup, "") != 0){ // retrieve
    strcat(message, " ");           // 'RTV '
    strcat(message, userLogID);     // 'RTV userID'
    strcat(message, " ");           // 'RTV userID '
    strcat(message, activeGroup);   // 'RTV userID activeGroup'
    strcat(message, " ");           // 'RTV userID activeGroup '

    // copia a messsageID do comando para a mensagem
    i++;
    for (int k = 0; comando[i] !='\n'; i++, k++) {
        messageID[k] = comando[i];
    }

    strcat(message, messageID);     // 'RTV userID activeGroup msgID'
    strcat(message, "\n");          // 'RTV userID activeGroup msgID\n'
    return message;
  }
  else{ //necessita de selecionar um activeGroup
    strcpy(message,"AGR");
    return message;
  }

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


/******* Validar lista de users *******/
char* validarListaUsers(int k, char *message_received, char* users_list){
  char list_aux[MAX_USER_ID + 1] = "";

  while(1){ // formato -> UID
            // UID=Parte 1

    //PARTE 1
    for (int w = 0; isdigit(message_received[k]) > 0; w++, k++) {
          list_aux[w] = message_received[k];
    }

    if( strlen(list_aux) != MAX_USER_ID){
      return  "";
    }
    strcat(users_list, list_aux);

    //verifica se esta no fim da lista
    if(message_received[k] == '\n' || message_received[k+1] == '\n'){
      strcat(users_list, "\n");
      break;
    }

    k++;
    strcat(users_list, " ");
  }
  return users_list;
}


/******* Validar lista de mensagens *******/
char* validarListaMensagens(int k, char *message_received, char* messages_list){
    char list_aux[MAX_MESSAGE_ID + 1] = "";
    char list_aux2[MAX_USER_ID + 1] = "";
    char list_aux3[MAX_TEXTO + 1] = "";
    char list_aux4[MAX_FILE_NAME + 1] = "";

    while(1){ // formato -> MID UID Tsize text [/ Fname Fsize data]]*
              // MID=Parte 1 , UID=Parte 2, Tsize=Parte 3.1, text=Parte 3.2
              // Fname=Parte 4.1, Fsize=Parte 4.2


      //PARTE 1
      for (int w = 0; isdigit(message_received[k]) > 0; w++, k++) {
            list_aux[w] = message_received[k];
      }

      if( strlen(list_aux) != MAX_MESSAGE_ID){
        return  "";
      }

      strcat(messages_list, list_aux);
      strcat(messages_list, " ");     //adiciona espaço
      strcpy(list_aux,"");
      k++;

      //PARTE 2
      for (int w = 0; isdigit(message_received[k]) > 0; w++, k++) {
            list_aux2[w] = message_received[k];
      }

      if( strlen(list_aux) != MAX_USER_ID){
        return  "";
      }
    }
}


/******* Validar lista de grupos *******/
char* validarListaGrupos(int k, char *message_received, char* groups_list){
  char list_aux[MAX_GROUP_ID + 1] = "";
  char list_aux3[MAX_MESSAGE_ID + 1] = "";
  int i = 0;

  while(1){ // formato -> GID GName MID
            // GID=Parte 1 , GName=Parte 2, MID=Parte 3

    //PARTE 1
    for (int w = 0; isdigit(message_received[k]) > 0; i++, w++, k++) {
          list_aux[w] = message_received[k];
    }

    if( strlen(list_aux) != MAX_GROUP_ID){
      return  "";
    }

    strcat(groups_list, list_aux);
    groups_list[i] = ' ';     //adiciona espaço
    groups_list[i + 1] = '-'; //adiciona '-'
    groups_list[i + 2] = ' '; //adiciona espaço
    strcpy(list_aux,"");
    k++;
    i = i + 3;


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
    for (int w = 0; isdigit(message_received[k]) > 0; i++, w++, k++) {
          list_aux3[w] = message_received[k];
    }

    if( strlen(list_aux3) != MAX_MESSAGE_ID){
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


/******* Verifica se a tarefa é executavel sem ser enviada ao server *******/
bool inspecionarMensagem(char* message_sent){

  char tarefa[MAX_TAREFA + 1] = "";  //tarefa a executar

  // le o tipo de tarefa da mensagem
  int t = 0;
  for (t; isalpha(message_sent[t]) != 0; t++) {
      tarefa[t] = message_sent[t];
  }

  // verifica se o user tentou fazer login ja existindo um user logged in
  if (strcmp(tarefa,"LOG") == 0 && checkUserLogStatus() == true) {
    char user_ID[MAX_USER_ID + 1] = "";

    //Buscar o userID introduzido no comando
    t++;
    for (int k = 0; isdigit(message_sent[t]) > 0; t++,k++) {
        user_ID[k] = message_sent[t];
    }

    //comparar o userID do comando com o userID do user que esta logged in
    if(strcmp(userLogID, user_ID) == 0){
      printf("User is already logged in\n");
      return true;
    }
    else{
      printf("Task requires the current logged in user to logout\n");
      printf("Logged in user: %s\n", userLogID);
      return true;
    }
  }

  // verifica se o user tentou executar uma tarefa que necessita de um activeGroup
  // sem ter selecionado um activeGroup
  else if (!strcmp(tarefa,"AGR")) {
    printf("Task requires user to select an active group\n");
    return true;
  }

  // verifica se o user tentou executar uma tarefa que necessita de login sem estar logged in
  else if (!strcmp(tarefa,"OFF")) {
    printf("Task requires user to be logged in\n");
    return true;
  }

  // verifica se o user introduziu um texto maior do que 240 caracteres
  else if (!strcmp(tarefa,"TXT")) {
    printf("Text can only have 240 characters\n");
    return true;
  }

  // Verifica se tarefa é select
  else if (!strcmp(tarefa,"SAG")){
    strcpy(activeGroup, validarGroup_ID( t+1, message_sent, activeGroup));
    if(strcmp(activeGroup,"") == 0)
      return true;
    printf("Group %s selected\n", activeGroup);
    return true;
  }
  else
    return false;

}






int main (int argc, char *argv[]) {

    /******* Adquire o IP address *******/
    char address[MAX_IP + 1] = ""; // IP default
    char port[MAX_PORT + 1] = "58021"; // port default
    //char name[MAX_IP + 1] = "";
    //struct hostent *h;
    //struct in_addr *a;

    if(gethostname(address, MAX_IP + 1) == -1){
      printf("Failed to get Hostname\n");
    }
    else{
      printf("Hostname: %s\n", address);
    }
    //h = gethostbyname(name);
    //a = (struct in_addr*)h->h_addr_list[0];
    //strcpy(address, inet_ntoa(*a));

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


    /******* Inicializar conexao UDP e TCP *******/
    int fdUDP, fdTCP, errcode;
    ssize_t nu,nt;
    socklen_t addrlen;
    struct addrinfo hintsUDP, hintsTCP, *resUDP, *resTCP;
    struct sockaddr_in addr;

    fdUDP = socket(AF_INET,SOCK_DGRAM,0);    //UDP socket
    if(fdUDP == -1){
        printf("Falha na criação do socket UDP\n");
        exit(1); //error
    }

    memset(&hintsUDP,0,sizeof hintsUDP);
    hintsUDP.ai_family=AF_INET;            //IPv4
    hintsUDP.ai_socktype=SOCK_DGRAM;       //UDP socket

    memset(&hintsTCP,0,sizeof hintsTCP);
    hintsTCP.ai_family=AF_INET;            //IPv4
    hintsTCP.ai_socktype=SOCK_STREAM;      //TCP socket

    errcode = getaddrinfo(address,port,&hintsTCP,&resTCP);
    if(errcode != 0){
        printf("Falha na verificação do IP\n");
        exit(1); //error
    }

    errcode = getaddrinfo(address,port,&hintsUDP,&resUDP);
    if(errcode != 0){
        printf("Falha na verificação do IP\n");
        exit(1); //error
    }


    /******* Ciclo que fica à espera dos comandos do utilizador *******/
    while (1) {

        char comando[MAX_COMANDO] = "";  //input do user
        char message_sent[MAX_MESSAGE_TCP_SENT + 1] = "";  //mensagem enviada ao server
        char message_received[MAX_MESSAGE_TCP_RECEIVED + 1] = "";  //mensagem recebida do server
        char message_received_Aux[MAX_MESSAGE_TCP_RECEIVED + 1] = "";  //mensagem recebida do server
        char tarefa_res[MAX_TAREFA + 1] = ""; //tarefa respondida pelo server

        if(checkUserLogStatus())
          printf("\n>>"); //logged in
        else
          printf("\n>"); //logged out

        //Apanha o input da linha de comandos
        fgets(comando,MAX_COMANDO,stdin);

        // cria a mensagem para enviar ao server
        strcpy(message_sent, createMessage(comando, message_sent));

        //verififa se tarefa é exit
        if (strcmp(message_sent, "exit") == 0){

          if(checkUserLogStatus() == true){
            shutDownTime = true;
            strcpy(message_sent, "OUT ");
            strcat(message_sent, userLogID);
            strcat(message_sent, " ");
            strcat(message_sent, userLogPass);
            strcat(message_sent, "\n");
          }
          else{
            freeaddrinfo(resUDP);
            freeaddrinfo(resTCP);
            close(fdUDP);
            exit(1);
          }
        }

        // Verifica se é uma tarefa que não necessita enviar mensagem ao server
        if (inspecionarMensagem(message_sent)){
          conectionUDP = true; //volta ao default (UDP)
          continue;
        }

        //Escolhe o tipo de conexao para enviar a mensagem ao server (TCP ou UDP)
        if(conectionUDP == true){ // comunicacao com o server em UDP (default)

          nu=sendto(fdUDP,message_sent,strlen(message_sent),0,resUDP->ai_addr,resUDP->ai_addrlen);
          if(nu == -1){
              printf("Falha no envia da mensagem usando UDP\n");
              exit(1); //error
          }

          addrlen=sizeof(addr);
          nu=recvfrom(fdUDP,message_received,sizeof(message_received),0,(struct sockaddr*)&addr,&addrlen);
          if(nu == -1){
              printf("Falha na receção da mensagem usando UDP\n");
              exit(1); //error
          }

          if (shutDownTime == true){
            freeaddrinfo(resUDP);
            freeaddrinfo(resTCP);
            close(fdUDP);
            exit(1);
          }

        }

        else { // comunicacao com o server em TCP

          fdTCP = socket(AF_INET,SOCK_STREAM,0);   //TCP socket
          if(fdTCP == -1){
              printf("Falha na criação do socket TCP\n");
              exit(1); //error
          }

          nt = connect(fdTCP,resTCP->ai_addr,resTCP->ai_addrlen);
        	if(nt == -1){
              printf("Falha na coneção TCP\n");
              exit(1); //error
          }

        	nt = write(fdTCP,message_sent,strlen(message_sent));
        	if(nt == -1){
              printf("Falha no envia da mensagem usando TCP\n");
              exit(1); //error
          }

          while (1){
            nt = read(fdTCP,message_received_Aux,MAX_MESSAGE_TCP_RECEIVED + 1);
            if(nt == -1){
                printf("Falha na receção da mensagem usando TCP\n");
                exit(1); //error
            }
            if(nt == 0) // ja não ha nada para ler da resposta do server
              break;
            strcat(message_received,message_received_Aux);
          }

          close(fdTCP);
          conectionUDP = true; // volta o UDP a ser a conexao default
        }

        // Variavel 't' = indica a posiçao de leitura na mensagem enviada pelo user
        // Variavel 'k' = indica a posiçao de leitura na mensagem recebida do server
        int t = 4; // 4 = inicio da palavra depois da tarefa
        int k = 0;

        // le o tipo de tarefa que o server respondeu
        for (k; isalpha(message_received[k]) != 0; k++) {
            tarefa_res[k] = message_received[k];
        }
        k++; // Para k ficar a 4 = inicio da palavra depois da tarefa



        /* ----------------------------------------- */
        /*        Tarefa: registar utilizador        */
        /*        comando: >REG UID pass             */
        /*        resposta: >RRG status              */
        /* ----------------------------------------- */


        if (strcmp(tarefa_res, "RRG") == 0) {

            char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

            // Tipo de status da tarefa que o server respondeu
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
          char user_ID[MAX_USER_ID + 1] = "";

          // le o tipo de status da tarefa que o server respondeu
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("User successfully unregisted\n");

            //Verificar se o UID que foi apagado é o mesmo que esta logged in
            if (checkUserLogStatus()){
              for (int i = 0; isdigit(message_sent[t]) > 0; i++, t++){
                user_ID[i] = message_sent[t];
              }
              if (strcmp(userLogID, user_ID) == 0){
                strcpy(userLogID, "");
                strcpy(userLogPass, "");
                strcpy(activeGroup, "");
                userLogStatus = false;
              }
            }
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
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("You are now Logged in\n");
            userLogStatus = true;
            strcpy(activeGroup,"");
            for (int i = 0; isdigit(message_sent[t]) > 0; i++, t++){
              userLogID[i] = message_sent[t];  // guarda o id do user que esta logged in
            }
            t++;
            for (int i = 0; isdigit(message_sent[t]) > 0 || isalpha(message_sent[t]) != 0; i++, t++){
              userLogPass[i] = message_sent[t];  // guarda a pass do user que esta logged in
            }
          }
          else if(strcmp(status_res, "NOK") == 0){
            printf("Login failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
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
          for (int w = 0; message_received[k] != '\n' || isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            printf("You are now Logged out\n");
            userLogStatus = false;
            strcpy(userLogID,"");
            strcpy(userLogPass, "");
            strcpy(activeGroup,"");
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

          char num_groups[MAX_GROUP_ID + 1] = ""; // numero de grupos respondido pelo server
          char groups_list[MAX_DISPLAY + 1] = ""; // lista de grupos

          // le quantos grupos o server respondeu
          for (int w = 0; isdigit(message_received[k]) > 0 ; k++, w++) {
              num_groups[w] = message_received[k];
          }

          //Verificar numero de grupos da resposta do servidor
          if (0 < atoi(num_groups) &&  atoi(num_groups) < 100){ // existe grupos

            //Verificar se lista de grupos cumpre Protocolo
            k++;
            strcpy(groups_list,validarListaGrupos(k, message_received, groups_list));
            if(strcmp(groups_list, "") == 0)
              printf("Erro na Lista de Grupos\n");
            else
              printf("There are %d groups:\n%s", atoi(num_groups), groups_list);
          }
          else if (strcmp(num_groups,"0") == 0){
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

          char group_ID_res[MAX_GROUP_ID + 1] = "";
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
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
            printf("Group subscription failed\n");
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
            printf("Group unsubscription failed\n");
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

          char num_groups[MAX_GROUP_ID + 1] = ""; // numero de grupos respondido pelo server
          char status_res[MAX_STATUS] = ""; // status da tarefa respondida pelo server
          char groups_list[MAX_DISPLAY + 1] = ""; // lista de grupos

          // le quantos grupos o server respondeu ou status
          for (int w = 0; isdigit(message_received[k]) > 0 ||  isalpha(message_received[k]) != 0 ||
                          message_received[k] == '_'; k++, w++) {
              if(isdigit(message_received[k]) > 0)
                num_groups[w] = message_received[k];
              else if(isalpha(message_received[k]) != 0 || message_received[k] == '_')
                status_res[w] = message_received[k];
          }

          //Verificar numero de grupos da resposta do servidor
          if (0 < atoi(num_groups) && atoi(num_groups) < 100){ // existe grupos

            //Verificar se lista de grupos cumpre Protocolo
            k++;
            strcpy(groups_list,validarListaGrupos(k, message_received, groups_list));
            if(strcmp(groups_list, "") == 0)
              printf("Erro na Lista de Grupos\n");
            else
              printf("There are %d groups:\n%s", atoi(num_groups), groups_list);

          }
          else if (strcmp(num_groups, "0") == 0){
            printf("No groups subscribed\n");
          }
          else if (strcmp(status_res, "E_USR") == 0){
            printf("Invalid user ID\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* --------------------------------------------- */
        /*        Tarefa: ulist                          */
        /*        comando: >ULS GID                      */
        /*        resposta: >RUL status [GName [UID ]*]  */
        /* --------------------------------------------- */

        else if (strcmp(tarefa_res, "RUL") == 0){
          char group_Name[MAX_GROUP_NAME + 1] = ""; // nome do grupo respondido pelo server
          char users_list[MAX_DISPLAY + 1] = ""; // lista de users
          char status_res[MAX_STATUS + 1] = ""; // status da tarefa respondida pelo server

          // le o tipo de status da tarefa que o server respondeu
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){

            k++;
            // le o nome do grupo
            for (int w = 0; isdigit(message_received[k]) > 0 || isalpha(message_received[k]) != 0 ||
                            message_received[k] == '-' || message_received[k] == '_'; k++, w++) {
                group_Name[w] = message_received[k];
            }

            if(message_received[k] == '\n'){ //depois do group name esta um '\n'
              printf("No users are subscribed to group %s", group_Name);
            }
            else{
              //Verificar se a lista de users cumpre Protocolo
              k++;
              strcpy(users_list, validarListaUsers(k, message_received, users_list));
              if(strcmp(users_list, "") == 0)
                printf("Erro na Lista de Users\n");
              else
                printf("%s:\n%s", group_Name, users_list);
            }

          }
          else if (strcmp(status_res, "NOK") == 0){
            printf("User list display failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------------------------- */
        /*        Tarefa: post                                         */
        /*        comando: >PST UID GID Tsize text [Fname Fsize data]  */
        /*        resposta: >RPT status                                */
        /* ----------------------------------------------------------- */

        else if (strcmp(tarefa_res, "RPT") == 0){
          char status_res[MAX_STATUS + 1] = ""; // status da tarefa respondida pelo server
          char msg_ID[MAX_GROUP_NAME + 1] = ""; // id respondido pelo server do post enviado pelo user

          // le o id do post ou status
          for (int w = 0; isdigit(message_received[k]) > 0 ||  isalpha(message_received[k]) != 0; k++, w++) {
              if(isdigit(message_received[k]) > 0)
                msg_ID[w] = message_received[k];
              else if(isalpha(message_received[k]) != 0)
                status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (0 < atoi(msg_ID) && atoi(msg_ID) < MAX_MESSAGES + 1){
            printf("Post ID: %s\n", msg_ID);
          }
          else if (strcmp(status_res, "NOK") == 0){
            printf("Post failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }
        }


        /* ---------------------------------------------------------------------------- */
        /*        Tarefa: retrieve                                                      */
        /*        comando: >RTV UID GID MID                                             */
        /*        resposta: >RRT status [N[ MID UID Tsize text [/ Fname Fsize data]]*]  */
        /* ---------------------------------------------------------------------------- */

        else if (strcmp(tarefa_res, "RRT") == 0){
          char status_res[MAX_STATUS + 1] = ""; // status da tarefa respondida pelo server
          char num_msg[2] = ""; // sao entre 1 e 20;
          char message_list[MAX_MESSAGE_TCP_RECEIVED] = ""; // lista de mensagens

          // le o tipo de status da tarefa que o server respondeu
          for (int w = 0; isalpha(message_received[k]) != 0; k++, w++) {
              status_res[w] = message_received[k];
          }

          //Verificar status da resposta do servidor
          if (strcmp(status_res, "OK") == 0){
            // le quantas mensagens o server respondeu
            k++;
            for (int w = 0; isdigit(message_received[k]) > 0 ; k++, w++) {
                num_msg[w] = message_received[k];
            }

            //Verificar se lista de mensagens cumpre Protocolo
            k++;
            strcpy(message_list,validarListaMensagens(k, message_received, message_list));
            if(strcmp(message_list, "") == 0)
              printf("Erro na Lista de Mensagens\n");
            else
              printf("There are %d messages:\n%s", num_msg, message_list);

          }
          else if(strcmp(status_res, "EOF") == 0){
            printf("Currently there are no messages available to be retrieved\n");
          }
          if (strcmp(status_res, "NOK") == 0){
            printf("Retreive messages failed\n");
          }
          else{
            printf("Erro nas Mensagens do Protocolo\n");
          }

        }


        /* ----------------------------------------- */
        /*        Tarefa: Comando invalido           */
        /*        comando: >                         */
        /*        resposta: >ERR                     */
        /* ----------------------------------------- */

        else if (!strcmp(tarefa_res,"ERR")) {
          printf("\nInvalid command\n");
        }

        else if (!strcmp(tarefa_res,"OLA")) {
          printf("\n%s\n", message_received);
        }

        // should never come here if it's working correctly
        else{
          printf("\nSomething went wrong, go check it out\n");
        }

    }

    return 0;
}
