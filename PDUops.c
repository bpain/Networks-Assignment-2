#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>


#include "PDUops.h"




int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData){
    uint8_t buffer[1400];   
    *(uint16_t *)buffer = htons(lengthOfData+2);
    memcpy(buffer + 2, dataBuffer, lengthOfData);
    int error = send(clientSocket, buffer, lengthOfData + 2, 0); 
    if( error < 0){
        perror("send error"); 
        return -1; 
    }
    return error;
}

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize){
    uint8_t buffer[1400];
    uint16_t lengthOfData;
    int bytesRead = recv(clientSocket, &lengthOfData, 2, MSG_WAITALL );
    if(bytesRead == 0){
        perror("client socket disconected");
        return 0;
    }
    else if(bytesRead < 0){
        perror("recv error");
        return -1;
    }
    if((ntohs(lengthOfData)-2) > bufferSize){
        perror("buffer too small");
        return -1;
    }
    bytesRead = recv(clientSocket, buffer, (ntohs(lengthOfData)-2), MSG_WAITALL);
    if (bytesRead == 0){
        perror("client socket disconected");
        return 0;
    }
    else if(bytesRead < 0){
        perror("recv error");
        return -1;
    }
    memcpy(dataBuffer, buffer, bytesRead);
    return bytesRead;

}