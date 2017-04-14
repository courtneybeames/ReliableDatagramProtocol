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

// headerInfo header;
char newHeaderInfo[80];

typedef struct headerInfo{
    char magic[6];
    char type[3];
    int seq;
    int ack;
    int payload;
    int window;
    char timeStamp[20];
    char data[1024];
}headerInfo;


char * putHeaderInfoToString(char * type, int seq, int ack, int payload, int window, double timeStamp);

void readPacketHeader(char * packet, headerInfo* header);

void logMessageSender(char *status, char *sender_ip, char *receiver_ip, int sender_port, int receiver_port, char * type, int seq, int payload);
void logMessageReceiver(char * status, char *sender_ip, char *receiver_ip, int sender_port, int receiver_port, headerInfo *header);
