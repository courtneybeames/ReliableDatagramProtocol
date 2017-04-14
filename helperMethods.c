#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>

#include "helperMethods.h"

char newHeaderInfo[80];
//headerInfo receiverHeader;


//puts header info into a string to read
char * putHeaderInfoToString(char * type, int seq, int ack, int payload, int window, double timeStamp){

  memset(newHeaderInfo, '\0', sizeof(newHeaderInfo) + 1);
  char tempHeaderInfo[80];
  memset(tempHeaderInfo, '\0', sizeof(tempHeaderInfo) + 1);

  strcpy(newHeaderInfo, "CSC361 ");
  sprintf(tempHeaderInfo, "%s %d %d %d %d %lf ", type, seq, ack, payload, window, timeStamp);

  strcat(newHeaderInfo, tempHeaderInfo);
  strcat(newHeaderInfo, "\n ");
  return newHeaderInfo;
}
//reads from header info string and puts into struct
void readPacketHeader(char * packet, headerInfo * header){
  char * token = strtok(packet, " ");
  char temporaryHeader[80];
  memset(temporaryHeader, '\0', sizeof(temporaryHeader));

  int count = 0;
  while (token != NULL && token[0] != '\n'){
    if(count == 0){
      strcpy((*header).magic,token);
      strcpy(temporaryHeader, token);
      strcat(temporaryHeader, " ");
	  //printf("Timestamp: %s \n", (*header).timeStamp);
    }
    if(count == 1){
      strcpy((*header).type,token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
      //printf("Type: %s \n", (*header).type);
    }
    if(count == 2){
      (*header).seq = atoi(token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
      //printf("Seq Num: %d \n", (*header).seq);
    }
    if(count == 3){
      (*header).ack = atoi(token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
      //printf("Ack Num: %d \n", (*header).ack);
    }
    if(count == 4){
      (*header).payload = atoi(token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
	  //printf("Payload: %d \n", (*header).payload);
    }
    if(count == 5){
      (*header).window = atoi(token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
	  //printf("Window: %d \n", (*header).window);
    }
    if(count == 6){ //change to double for timeval
      //double d;
      //sscanf(token, "%lf", &d);
      //printf("token is %s \n", token);
      //(*header).timeStamp = d;
      strcpy((*header).timeStamp,token);
      strcat(temporaryHeader, token);
      strcat(temporaryHeader, " ");
      //printf("Timestamp: %s \n", (*header).timeStamp);
    }

    count++;
    token = strtok(NULL, " ");
  }

  if (strcmp((*header).type, "DAT") == 0){
    token = strtok(NULL, "");
    //printf("token is %s \n", token);
    memset((*header).data, '\0', sizeof((*header).data)+1);
    if(token != NULL){
      strcpy((*header).data, token);
    }
    //printf("header.data = %s \n", (*header).data);
  }

}


void logMessageReceiver(char * status, char *sender_ip, char *receiver_ip, int sender_port, int receiver_port, headerInfo *header) {
    time_t t;
    time(&t);
    struct tm* timeStruct = localtime(&t);
    char timeArr[17];
    strftime(timeArr, 16, "%H:%M:%S.%02u", timeStruct);

    if((*header).seq == -1){
      printf("%s %s %s:%d %s:%d %s %d %d\n", timeArr, status, sender_ip, sender_port, receiver_ip, receiver_port, (*header).type, (*header).ack, (*header).window);
    }else{
      printf("%s %s %s:%d %s:%d %s %d %d\n", timeArr, status, sender_ip, sender_port, receiver_ip, receiver_port, (*header).type, (*header).seq, (*header).payload);
    }
}

void logMessageSender(char * status, char *sender_ip, char *receiver_ip, int sender_port, int receiver_port, char * type, int seq, int payload) {
    time_t t;
    time(&t);
    struct tm* timeStruct = localtime(&t);
    char timeArr[17];
    strftime(timeArr, 16, "%H:%M:%S.%02u", timeStruct);

    printf("%s %s %s:%d %s:%d %s %d %d\n", timeArr, status, sender_ip, sender_port, receiver_ip, receiver_port, type, seq, payload);

}

