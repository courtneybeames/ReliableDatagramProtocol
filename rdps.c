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


#include <fcntl.h>

#include "helperMethods.h"

char newHeaderInfo[80];
struct timeval timeout, startTran, beginCode;
headerInfo headerList[10000];
char * fileSender;
int synPacketsSent=0;
int ackPacketsReceived=0;
int datPacketsSent=0;
int finPacketsSent=0;
int uniqueDatPacketsSent=0;
int rstPacketsSent=0;
int rstPacketsReceived=0;
int totalDataBytesSent=0;
int packetsSent =0;
int haveSeenArray[10000];

//check if previously seen acknowledgment
int have_previously_seen(int ackNum){
  int i=0;
  while(haveSeenArray[i] !=0){
    if(haveSeenArray[i] == ackNum){
      return 1;
    }
    i++;
  }
  haveSeenArray[i] = ackNum;
  return 0;
}

//gets time between two values
double get_time(struct timeval curTime, double startTime){
    double startTransmission = curTime.tv_sec+(curTime.tv_usec/1000000.0);
    double transStartTime= startTransmission-startTime;
    return transStartTime;
}


//resends timed out packets (puts them into new header with new timeout val)
void resendPacket(headerInfo packet, int sock, struct sockaddr_in receiver, double newTimeStamp){
  
  putHeaderInfoToString(packet.type, packet.seq, packet.ack, packet.payload, packet.window, newTimeStamp);
  char *fileSend = (char*) malloc(80 + packet.payload);
  strcpy(fileSend, newHeaderInfo);


  if(strcmp(packet.type, "DAT") ==0){
    strcat(fileSend, packet.data);
    datPacketsSent++;
    totalDataBytesSent += packet.payload;

  }
  if(strcmp(packet.type, "SYN") == 0){
    synPacketsSent++;
  }
  if(strcmp(packet.type, "FIN") == 0){
    finPacketsSent++;

  }
  if(strcmp(packet.type, "RST") == 0){
    rstPacketsSent++;
  }
  char PlzWork[80+packet.payload];
  memset(PlzWork, '\0', sizeof PlzWork +1);
  strcpy(PlzWork, fileSend);
  sendto(sock, PlzWork, sizeof PlzWork, 0, (struct sockaddr*)&receiver, sizeof receiver);
 
}

//Checks if any timed out packets
int checkIfTimedOut(int count){
  gettimeofday(&timeout, NULL);
  double newTimeStamp= timeout.tv_sec+(timeout.tv_usec/1000000.0);
  double timeElapsed;
  int i;
      
  for(i=0; i<count; i++){
    if(headerList[i].seq != -2){
      double d;
      sscanf(headerList[i].timeStamp, "%lf", &d);
      timeElapsed = get_time(timeout, d);
      if(timeElapsed > 0.03){
        return i;
      }
    }
  }
  return -1;
}

//checks if every packet has been ack-ed
int allAcknowledged(int count){
  int i;
  for(i=0; i<count; i++){
    if(headerList[i].seq != -2){
      return -1;
    }
  }
  return 1;
}

//opens socket, uses select to get input from stdIn and client
int main(int argc, char *argv[]){

  gettimeofday(&beginCode, NULL);
  double startCode= beginCode.tv_sec+(beginCode.tv_usec/1000000.0);

  headerInfo senderHeader;
  headerInfo packetInfo;
  memset(haveSeenArray, 0, sizeof haveSeenArray);

  int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in sender;
  struct sockaddr_in receiver;

  ssize_t recsize;
  socklen_t fromlen;

  char * sender_ip = argv[1];
  int sender_port = atoi(argv[2]);
  char * receiver_ip = argv[3];
  int receiver_port = atoi(argv[4]);
  char *sender_file_name = argv[5];

  if(argc != 6){
    printf("Error: need 6 arguments. \n");
    exit(1);
  }


//sender
  memset(&sender, 0, sizeof sender);
  sender.sin_family = AF_INET;
  sender.sin_port = htons(sender_port);
  inet_aton(sender_ip, &sender.sin_addr);

//receiver
  memset(&receiver, 0, sizeof receiver);
  receiver.sin_family = AF_INET;
  receiver.sin_port = htons(receiver_port);
  inet_aton(receiver_ip, &receiver.sin_addr);
  fromlen = sizeof(receiver);

  int opt=1; //1=TRUE -- so we can reuse sock address
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) == -1){
    printf("error with setsockopt\n");
    exit(1);
  }


  if (-1 == bind(sock, (struct sockaddr *)&sender, sizeof sender)) {
    perror("error bind failed");
    close(sock);
    exit(EXIT_FAILURE);
  }
  
  fcntl(sock, F_SETFL, O_NONBLOCK);

  FILE * fp = fopen(sender_file_name, "r");
  if(fp == NULL){
    printf("Error on file open \n");
    exit(0);
  }
  fseek(fp, 0L, SEEK_END);
  int fileSize = ftell(fp);
  char fileInfo[fileSize +1];
  memset(fileInfo, '\0', sizeof(fileInfo)+1);
  rewind(fp);


  if (fread(fileInfo, sizeof(char), fileSize, fp) == 0){
      printf("Error on fileInfo \n");
 }

  fclose(fp);

  srand(time(NULL));


  char * type = "SYN";
  int initialSeqNum = rand() % 100000;
  int nextSeqNum = initialSeqNum;
  int sendBase = initialSeqNum;
  int ackNum = -1;
  int payload = 0;
  int window = -1;
  double timeStamp;
  int count = 0;

  int packetSize = 1024;
  int packetsSent=0;
  int event = 0;
  int bytesOfFileSent=0;

  char infoFromReceiver[80];
  char finArray [sizeof infoFromReceiver];

  memset(infoFromReceiver, '\0', sizeof(infoFromReceiver));
  int timerRunning = 0;

int flag =1;
//handshake
  while(event == 0){ //SYN
    int receiveSize = recvfrom(sock, (void*)infoFromReceiver, sizeof infoFromReceiver, 0, (struct sockaddr*)&receiver, &fromlen); 
    if(receiveSize < 0 && flag ==1){
      type = "SYN";
      gettimeofday(&startTran, NULL);
      timeStamp= startTran.tv_sec+(startTran.tv_usec/1000000.0);
      timerRunning = 1;
      putHeaderInfoToString(type, nextSeqNum, ackNum, payload, window, timeStamp);
      char dataSend[sizeof newHeaderInfo];
      memset(dataSend, '\0', sizeof dataSend +1);
      strcpy(dataSend, newHeaderInfo);
      readPacketHeader(dataSend, &packetInfo);
      headerList[count]= packetInfo;
      packetsSent++;
      count++; 
      synPacketsSent++;
      
      sendto(sock, newHeaderInfo, sizeof newHeaderInfo, 0, (struct sockaddr*)&receiver, sizeof receiver);
      logMessageSender("s", sender_ip, receiver_ip, sender_port, receiver_port, type, nextSeqNum, payload);
      nextSeqNum +=1;
      flag =0;
      
    }if(receiveSize <0 && flag ==0){
      int packetNum = checkIfTimedOut(count);
      if (packetNum > -1){
          gettimeofday(&startTran, NULL);
          double newTimeStamp= startTran.tv_sec+(startTran.tv_usec/1000000.0);
          resendPacket(headerList[packetNum], sock, receiver, newTimeStamp);
          sprintf(headerList[packetNum].timeStamp, "%lf", newTimeStamp);
          synPacketsSent++;
          packetsSent++;
          logMessageSender("S",sender_ip, receiver_ip, sender_port, receiver_port, headerList[packetNum].type, headerList[packetNum].seq, headerList[packetNum].payload);
      }
      sleep(0.9);
    }
    if(receiveSize >0){
      event =1;
      readPacketHeader(infoFromReceiver, &senderHeader);\
      packetsSent--;
      logMessageReceiver("r",sender_ip, receiver_ip, sender_port, receiver_port, &senderHeader);
      printf("RECEIVED ACK FOR SYN \n");
      ackPacketsReceived++;
      headerList[0].seq = -2;
      
    }
  }
  int waiting =1;
  int finalAck =0;
  int dataCount = 0;
  int windowCount = 10;
  int finalPayload = 944;


  //START FOREVER LOOP
  for(;;){
  
    while(waiting){

      if(event ==1 && dataCount < 10){ //DATA TO SEND (not more than 10) 

        type = "DAT";
		 
        int ackNum = -1;

        int numBytesLeft = (strlen(fileInfo) - bytesOfFileSent);
        
        if(numBytesLeft > 1024-80){
          payload = 1024 - 80;
        }else{
          payload = numBytesLeft;
        }

        int window = -1;
        putHeaderInfoToString(type, nextSeqNum, ackNum, payload, window, timeStamp);

        char fileSend[80 + payload];
        strcpy(fileSend, newHeaderInfo);
      
        char * pointer = &fileInfo[bytesOfFileSent];
        strncat(fileSend, pointer, payload);

        gettimeofday(&startTran, NULL);
		    timeStamp= startTran.tv_sec+(startTran.tv_usec/1000000.0);
	    
        char *dataSend2 = (char *) malloc(sizeof fileSend +1);
        strcpy(dataSend2, fileSend);
        readPacketHeader(dataSend2, &packetInfo);
        headerList[count]= packetInfo;
        count++;
        free(dataSend2); 
        int i;

      
        sendto(sock, fileSend, sizeof fileSend, 0, (struct sockaddr*)&receiver, sizeof receiver);
        datPacketsSent++;
        uniqueDatPacketsSent++;

        //increasing dataCount
        dataCount++;
        windowCount--;
        bytesOfFileSent += payload;
        totalDataBytesSent += payload;
      
        nextSeqNum += payload;
        
        packetsSent++;

        logMessageSender("s",sender_ip, receiver_ip, sender_port, receiver_port, type, nextSeqNum, payload);
        
        if(bytesOfFileSent == strlen(fileInfo)){
          finalPayload = payload;
          event=0;
        }

      }
      //if receives anything
      memset(infoFromReceiver, '\0', sizeof(infoFromReceiver));
      int receiveSize = recvfrom(sock, (void*)infoFromReceiver, sizeof infoFromReceiver, 0, (struct sockaddr*)&receiver, &fromlen);
      if(receiveSize > 0){
        packetsSent--;
        dataCount --;
        readPacketHeader(infoFromReceiver, &senderHeader);
        if(have_previously_seen(senderHeader.ack) == 1){
          logMessageReceiver("R", sender_ip, receiver_ip, sender_port, receiver_port, &senderHeader);
        }else{
          logMessageReceiver("r", sender_ip, receiver_ip, sender_port, receiver_port, &senderHeader);
        }
        if(strcmp(senderHeader.type, "RST") == 0){
          exit(0);
        }
        ackPacketsReceived++;
        int i=0;

        
        while(i<count){
          if(headerList[i].seq == senderHeader.ack - 944){
            headerList[i].seq = -2;
            break;
          }
          else if(headerList[i].seq == senderHeader.ack - finalPayload){
            headerList[i].seq = -2;
            break;
          }
          else{
            if(finalAck ==1 && headerList[i].seq == senderHeader.ack -1){
              headerList[i].seq = -2;
              break;
            }
          }
          i++;
        }
        
        windowCount = 10 - dataCount;

        waiting=1;
        if(bytesOfFileSent == strlen(fileInfo) && allAcknowledged(count) ==1){
          waiting=0;
          event=2;
        }
        
        //logs when received final ack
        if(finalAck==1 && allAcknowledged(count)==1){
          gettimeofday(&beginCode, NULL);
          printf("total data bytes sent: %d \n", totalDataBytesSent);
          printf("unique data bytes sent: %d \n", bytesOfFileSent);
          printf("total data packets sent: %d \n", datPacketsSent);
          printf("unique data packets sent: %d \n", uniqueDatPacketsSent);
          printf("SYN packets sent: %d \n", synPacketsSent);
          printf("FIN packets sent: %d \n", finPacketsSent);
          printf("RST packets sent: %d \n", rstPacketsSent);
          printf("ACK packets received: %d \n", ackPacketsReceived);
          printf("RST packets received: %d \n", rstPacketsReceived);
          printf("total time duration (second): %lf \n", get_time(beginCode, startCode)); 
          exit(0);
        }
      }
      //checks if anything is timed out
      if(receiveSize < 0){
        int packetNum = checkIfTimedOut(count);

        if (packetNum > -1){
          int i=packetNum;
          int resentPackets=0;
          while(i<count && resentPackets < 5 && headerList[i].seq != -2){
		        gettimeofday(&startTran, NULL);
            double newTimeStamp= startTran.tv_sec+(startTran.tv_usec/1000000.0);

            resentPackets++;
            windowCount--;
            resendPacket(headerList[i], sock, receiver, newTimeStamp);
             
            sprintf(headerList[i].timeStamp, "%lf", newTimeStamp);
            
            packetsSent++;
            logMessageSender("S",sender_ip, receiver_ip, sender_port, receiver_port, headerList[i].type, headerList[i].seq, headerList[i].payload);
            i++;

          waiting =1;
        }
        
      }
    
    }
   }
    
   
    memset(finArray, '\0', sizeof(finArray) +1);
    strcpy(finArray, infoFromReceiver);
    

    
    if(event == 2){ //FIN TO SEND
      type = "FIN";
      readPacketHeader(finArray, &senderHeader);
      ackNum = -1;
      payload = 0;
      window = -1;
	  
      gettimeofday(&startTran, NULL);
      timeStamp= startTran.tv_sec+(startTran.tv_usec/1000000.0);
	  
	  
      putHeaderInfoToString(type, nextSeqNum, ackNum, payload, window, timeStamp);
      
      char dataSend3[sizeof newHeaderInfo];
      memset(dataSend3, '\0', sizeof dataSend3 +1);
      strcpy(dataSend3, newHeaderInfo);
      readPacketHeader(dataSend3, &packetInfo);
      headerList[count]= packetInfo;

      count++;
      
      
      sendto(sock, newHeaderInfo, sizeof newHeaderInfo, 0, (struct sockaddr*)&receiver, sizeof receiver);
      packetsSent++;
      finPacketsSent++;

	    finalAck =1;
      logMessageSender("s", sender_ip, receiver_ip, sender_port, receiver_port, type, nextSeqNum, payload);
      waiting=1;
      
    }
  }

}
