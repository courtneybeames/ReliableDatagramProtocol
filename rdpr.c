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
struct timeval finTimer;
char newHeaderInfo[80];
double timeStamp;
int haveSeenArray[10000];
double finTimeStamp;
double startTime;

//gets time difference between two values 
double get_time(struct timeval curTime, double startTime){
    double startTransmission = curTime.tv_sec+(curTime.tv_usec/1000000.0);
    double transStartTime= startTransmission-startTime;
    return transStartTime;
}

//checks if previously seen the packet
int have_previously_seen(int seqNum){
  int i=0;
  while(haveSeenArray[i] !=0){
    if(haveSeenArray[i] == seqNum){
      return 1;
    }
    i++;
  }
  haveSeenArray[i] = seqNum;
  return 0;
}

//opens socket, uses select to get input from stdIn and client
int main(int argc, char *argv[]){
  struct timeval timeout;
  struct timeval nonBlock;
  nonBlock.tv_sec = 0;
  nonBlock.tv_usec = 0.00003;
  int bytesAdded=0;
 

  memset(haveSeenArray, '\0', sizeof haveSeenArray);
  headerInfo receiverHeader;

  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in sa;

  ssize_t recsize;
  socklen_t fromlen;

  char * receiver_ip = argv[1];
  int receiver_port = atoi(argv[2]);
  char *receiver_file_name = argv[3];

  if(argc != 4){
    printf("Error: need 4 arguments. \n");
    exit(1);
  }

  int select_result;
  fd_set read_fds;

  int sentFinAck=0;

//setting up socket
  memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(receiver_port);
  inet_aton(receiver_ip, &sa.sin_addr);
  fromlen = sizeof(sa);


  int opt=1; //1=TRUE -- so we can reuse sock address
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) == -1){
    printf("error with setsockopt\n");
    exit(1);
  }

  if (-1 == bind(sock, (struct sockaddr *)&sa, sizeof sa)) {
    perror("error bind failed");
    close(sock);
    exit(EXIT_FAILURE);
  }

  char * type = "ACK";
  int seqNum = -1;
  int ackNum = 0;
  int payload = -1;
  int window = 10;
  double timeStamp = 0.0;
  
  char fileArray[10000];
  memset(fileArray, '\0', sizeof fileArray);
  int flag =1;

  char * sender_ip = "192.168.1.100";
  int sender_port = 8080;
  char * receiveStatus = "r";
  char * sendStatus = "s";

  FILE * fp = fopen(receiver_file_name, "w");

  if(fp == NULL){
    printf("Error on file open. \n");
  }

  int firstRound=1;
  int initialSeqNum =0;
 
 //for logging
  int totalBytesReceived=0;
  int uniqueBytesReceived=0;
  int totalDatPacksReceived=0;
  int uniqueDatPacksReceived=0;
  int synPacks=0;
  int finPacks=0;
  int rstPacks=0;
  int ackPacks=0;
 
  for (;;) {
    /* FD_ZERO() clears out the called socks, so that it doesn't contain any file descriptors. */
    FD_ZERO( &read_fds );
    //FD_SET() adds the file descriptor "read_fds" to the fd_set, so that select() will return the character if a key is pressed
    FD_SET(STDIN_FILENO, &read_fds );

    FD_SET(sock, &read_fds);

    char buffer[1024];
    memset(buffer, '\0', sizeof buffer + 1);


    select_result = select(sock +1, &read_fds, NULL, NULL, &nonBlock);

    //checking input from client or stdin.
    if(FD_ISSET(sock, &read_fds)){
      memset(buffer, '\0', sizeof buffer + 1);
      recsize = recvfrom(sock, (void*)buffer, sizeof buffer, 0, (struct sockaddr*)&sa, &fromlen);
      //if received something
      if (recsize < 0) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(EXIT_FAILURE);
      }
   
      readPacketHeader(buffer, &receiverHeader);
      
      //checks which log to do
      if(have_previously_seen(receiverHeader.seq)==1){
        receiveStatus = "R";
        sendStatus = "S";
      }else{
        if(strcmp(receiverHeader.type, "DAT") == 0){
          uniqueDatPacksReceived++;
          uniqueBytesReceived += receiverHeader.payload;
        }
        if(strcmp(receiverHeader.type, "SYN") ==0){
          gettimeofday(&timeout, NULL);
          startTime = timeout.tv_sec+(timeout.tv_usec/1000000.0);
        }
        receiveStatus = "r";
        sendStatus = "s";
      }

      //logs to receiver
      logMessageReceiver(receiveStatus, sender_ip, receiver_ip, sender_port, receiver_port, &receiverHeader);
      if(firstRound){

        firstRound = 0;
	      initialSeqNum = receiverHeader.seq;
      }

//if syn packet
      if(strcmp(receiverHeader.type, "SYN") == 0){

        synPacks++;
        type = "ACK";
        ackNum = receiverHeader.seq + 1;
        window = 10;
        putHeaderInfoToString(type, seqNum, ackNum, payload, window, timeStamp);
        sendto(sock, newHeaderInfo, sizeof newHeaderInfo, 0, (struct sockaddr*)&sa, sizeof sa);
        ackPacks++;
        
        logMessageSender(sendStatus, sender_ip, receiver_ip, sender_port, receiver_port, type, ackNum, window);
      }
//if dat packet, add to file
      if(strcmp(receiverHeader.type, "DAT") == 0){
        totalDatPacksReceived++;
        type = "ACK";
        ackNum = receiverHeader.seq + receiverHeader.payload;
        window=window-1;
        
        
    	  fseek(fp, receiverHeader.seq - initialSeqNum -1, SEEK_SET );
        fputs(receiverHeader.data, fp);
        bytesAdded = strlen(receiverHeader.data);
        
        totalBytesReceived += receiverHeader.payload;

        putHeaderInfoToString(type, seqNum, ackNum, payload, window, timeStamp);

        sendto(sock, newHeaderInfo, sizeof newHeaderInfo, 0, (struct sockaddr*)&sa, sizeof sa);
        ackPacks++;

        logMessageSender(sendStatus, sender_ip, receiver_ip, sender_port, receiver_port, type, ackNum, window);
        window=window+1;
      }
//if fin packet, acknowledge and then time to see if received ack
      if(strcmp(receiverHeader.type, "FIN") == 0){
        finPacks++;
        window = 10;
        type = "ACK";
        sentFinAck=1;        
        ackNum = receiverHeader.seq + 1;
        payload = 0;
        putHeaderInfoToString(type, seqNum, ackNum, payload, window, timeStamp);
        
        logMessageSender(sendStatus, sender_ip, receiver_ip, sender_port, receiver_port, type, ackNum, window);
        
        gettimeofday(&finTimer, NULL);
        finTimeStamp = finTimer.tv_sec+(finTimer.tv_usec/1000000.0);        
        sendto(sock, newHeaderInfo, sizeof newHeaderInfo, 0, (struct sockaddr*)&sa, sizeof sa);
        ackPacks++;

      }
      if(strcmp(receiverHeader.type, "RST") == 0){
        rstPacks++;
        exit(0);
      }

    };
    //log if sent final acknowledgement
    if(sentFinAck==1){
        gettimeofday(&timeout, NULL);
        gettimeofday(&finTimer, NULL);
        double timeTaken= get_time(finTimer, finTimeStamp);
        double totalTime= get_time(timeout, startTime);
        if(timeTaken > 0.07){
          printf("total data bytes received: %d \n", totalBytesReceived);
          printf("unique data bytes received: %d \n", uniqueBytesReceived);
          printf("total data packets received: %d \n", totalDatPacksReceived);
          printf("unique data packets received: %d \n", uniqueDatPacksReceived);
          printf("SYN packets received: %d \n", synPacks);
          printf("FIN packets received: %d \n", finPacks);
          printf("RST packets received: %d \n", rstPacks);
          printf("ACK packets sent: %d \n", ackPacks);
          printf("RST packets sent: %d \n", rstPacks);
          printf("total time duration (second): %lf \n", totalTime);
          exit(0);
        }  
    }

    //checks for "q"
    if(FD_ISSET(STDIN_FILENO, &read_fds)){
      fgets(buffer, sizeof(buffer), stdin);
      if(strcmp(buffer,"q\n") == 0){
        fclose(fp);
        close(sock);
        exit(0);
      }else{
        printf("Command not recognized.\n");
      }
    }
  }
}
