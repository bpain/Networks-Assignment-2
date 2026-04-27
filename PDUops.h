#include <stdint.h>
#ifndef PDUOPS
#define PDUOPS

int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize);

#endif 