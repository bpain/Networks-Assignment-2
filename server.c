#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>

#include "networks.h"
#include "safeUtil.h"
#include "PDUops.h"
#include "pollLib.h"
#include "handle_table.h"

#define MAXBUF 1024
#define MAXHANDLE 100
#define DEBUG_FLAG 1

int mainServerSocket = 0;   //socket descriptor for the server socket
int clientSocket = 0;   //socket descriptor for the client socket
int portNumber = 0;

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

void handler(int sig) {
    close(mainServerSocket);  // Set flag and return immediately
}

void addNewSocket( int main_socket){
    int clientSocket = tcpAccept(main_socket, DEBUG_FLAG);
    addToPollSet(clientSocket);
    printf("New client added to poll set, socket: %d\n", clientSocket);
}

int send_flag(int socket, uint8_t flag){
    // printf("flag sent"); 
    return sendPDU(socket, &flag, 1); 
}

int handle1(int socket, uint8_t* buffer){
    uint8_t handle[MAXHANDLE]; 
    uint8_t len = buffer[0]; 
    memcpy(handle, buffer + 1, len); 
    if(get_socket(handle, len)){
        return send_flag(socket, 3); 
    }
    add_element(socket, len, handle); 
    return send_flag(socket, 2);
}


int handle5(int socket, uint8_t* buffer, int total_len){
    uint8_t handle[MAXHANDLE]; 
    uint8_t len1 = buffer[1]; 
    uint8_t len2 = buffer[len1 + 3]; 
    memcpy(handle, buffer + len1 + 4, len2);     
    int new_sock = get_socket(handle, len2); 
    if(new_sock){
        return sendPDU(new_sock, buffer, total_len); 
    }   
    uint8_t new_buffer[MAXHANDLE + 2]; 
    new_buffer[0] = 7; 
    new_buffer[1] = len2; 
    memcpy(new_buffer + 2, handle, len2); 
    //handle[len2] = 0; 
    //printf("bad handle %s, len %d", handle, len2); 
    return sendPDU(socket, new_buffer, len2 + 2); 

}


int handle6(int socket, uint8_t* buffer, int total_len) {
    uint8_t lengths[9];
    uint8_t sender_length = buffer[1];
    uint8_t num_handles = buffer[2 + sender_length];
    int header_len = 2 + sender_length + 1;   // type, sender_len, sender, num_handles
    int ptr = 0;
    for (int i = 0; i < num_handles; i++) {
        lengths[i] = buffer[header_len + ptr];
        ptr += lengths[i] + 1;
    }

    int message_len = total_len - (header_len + ptr);
    uint8_t *message_ptr = buffer + header_len + ptr;
    int ptr2 = 0;
    for (int i = 0; i < num_handles; i++) {
        uint8_t current_handle[MAXHANDLE];

        memcpy(current_handle, buffer + header_len + ptr2 +1, lengths[i]);

        int current_socket = get_socket(current_handle, lengths[i]);

        if (current_socket) {
            sendPDU(current_socket, buffer, total_len);
        } else {
            uint8_t new_buffer[MAXHANDLE + 2];
            new_buffer[0] = 7;
            new_buffer[1] = lengths[i];
            memcpy(new_buffer + 2, current_handle, lengths[i]);
            return sendPDU(socket, new_buffer, lengths[i] + 2);
        }

        ptr2 += lengths[i] + 1;
    }
    return 0;
}

int send_handles(int socket){
    uint8_t* lengths = get_lengths(); 
    uint8_t* handles = get_handles(); 
    uint32_t total = getsize(); 
    int ptr = 0; 
    for(int i = 0; i < total; i++){
        uint8_t buffer[MAXHANDLE + 2]; 
        buffer[0] = 12; 
        buffer[1] = lengths[i]; 
        memcpy(buffer + 2, handles + ptr, lengths[i]); 
        ptr += lengths[i]; 
        sendPDU(socket, buffer, ptr + 2); 
    }
    return send_flag(socket, 13); 
}

int handle10(int socket){
    uint8_t buffer[5]; 
    buffer[0] = 11; 
    printf("num handles %d", getsize()); 
    fflush(stdout); 
    uint32_t handles = htonl(getsize());
    memcpy(buffer +1, (uint8_t*)&handles, sizeof(uint32_t)); 
    sendPDU(socket, buffer, 5); 
    return send_handles(socket); 
}

int handle4(int socket, uint8_t* buffer, int total_len){
    uint32_t size = getsize(); 
    int* sockets = get_sockets(); 
    uint8_t len = buffer[1]; 
    uint8_t handle[MAXHANDLE]; 
    memcpy(handle, buffer + 2, len); 
    int bytes  = 0; 
    if(sockets){
        for(int i = 0; i < size; i++){
            if(!(get_socket(handle, len) == sockets[i])){
                uint8_t current_handle[MAXHANDLE]; 
                bytes += sendPDU(sockets[i], buffer, total_len); 
            }
        }
    }
    return bytes; 
}


int parse_message(int socket, uint8_t* buffer, int total_len){
    uint8_t flag = buffer[0]; 
    switch(flag){
        case 1: 
            return handle1(socket, buffer + 1); 
        break; 

        case 4: 
            return handle4(socket, buffer, total_len);
        break; 
        
        case 5: 
            return handle5(socket, buffer, total_len); 
        break; 

        case 6: 
            return handle6(socket, buffer, total_len); 
        break; 

        case 10: 
            return handle10(socket); 
        break; 

        default: 
            perror("unknown flag"); 
        break;
    }
    return 0; 
}

void handleClient(int client_socket){
    uint8_t buffer[MAXBUF];
    int recvResult = recvPDU(client_socket, buffer, MAXBUF);
    if(recvResult == 0 || ((recvResult < 0) && (errno == ECONNRESET))){
        // printf("Client on socket %d disconnected\n", client_socket);
        uint8_t* handle = get_handle(client_socket); 
        if(handle){
            remove_element(handle); 
        }
        removeFromPollSet(client_socket);
        close(client_socket);
    }
    else{
        parse_message(client_socket, buffer, recvResult); 
        // for(int i = 0; i <recvResult; i++){
        //     printf("%c", (char)buffer[i]);
        // }
        // fflush(stdout); 
        
        //printf("Message received on socket %d, length: %ld Data: %s\n", client_socket, recvResult, buffer);
    }
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}


int main(int argc, char *argv[])
{
    signal(SIGINT, handler); 
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);
    setupPollSet();
    addToPollSet(mainServerSocket);
    while(1){
        int socket = pollCall(POLL_WAIT_FOREVER);
        if(socket == mainServerSocket){
            addNewSocket(mainServerSocket);
        }
        else{
            handleClient(socket);
        }
    }
	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);
	
	return 0;
}

