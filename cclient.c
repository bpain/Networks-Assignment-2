/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

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

#include "networks.h"
#include "safeUtil.h"
#include "PDUops.h"
#include "pollLib.h"

#define MAXBUF 1400
#define MAXHANDLE 100
#define DEBUG_FLAG 1

void sendToServer(int socketNum);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void handleClient();
int initClient(); 
int send_L_packet(); 
int send_C_packet( uint8_t numhandles, uint8_t* handles, uint8_t* lengths, int message_len, uint8_t* message); 
int send_B_packet( uint8_t* message, int message_len); 
int send_M_packet( uint8_t* handle1, uint8_t* message, uint8_t handle_len, int message_len, uint8_t flags); 
int send_init(); 
void parse_buffer(uint8_t* buffer, int total_length);
void handle4(uint8_t* buffer, int total_length); 
void handle5(uint8_t* buffer, int total_length); 
void handle7(uint8_t* buffer); 
void handle11(uint8_t* buffer);
void handle12(uint8_t* buffer); 



uint8_t handle[100]; 
uint8_t handle_length; 
int socketNum; 


int get_message(char* message){ //reused code --> just parses message w/ fgets --> pain in booty when trying to test longer thingamajigs 
	int message_len;
	fgets(message, MAXBUF, stdin); 
	message_len = strlen(message); 
	return message_len; 
}

int get_print_handle(uint8_t* handle, uint8_t* buffer){ //print and return handle field --> just reused code a lot 
	uint8_t len = buffer[0]; 
	memcpy(handle, buffer + 1, len); 
	handle[len] = 0; 
	printf("%s: ", handle);
	return len;
}

int main(int argc, char * argv[]){ // setup/teardown + poll loop code 
	socketNum = 0;         
	checkArgs(argc, argv);
	handle_length = strlen(argv[1]); 
	memcpy(handle, argv[1], handle_length); 
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	setupPollSet();
	addToPollSet(STDIN_FILENO);
    addToPollSet(socketNum);
	if(!initClient()){
		handleClient();
	}
	close(socketNum);
	return 0;
}

int check_message(char* message, int message_len){ //duh 
	if(handle[0] != '\n'){
		fgets(message, MAXBUF, stdin); 
		message_len = strlen(message); 
	} 
	else{
		printf("empty handle"); 
		return -1; 
	}
	return 0; 
}

int M_type(){ //get handle + check it + break up into multiple packets if necessary 
	char target_handle[MAXHANDLE]; 
	char message[MAXBUF]; 
	scanf("%s", target_handle );
	uint8_t handle_length = strlen(target_handle);  
	int message_len;
	if(target_handle[0] != '\n'){
		message_len = get_message(message);  
	} 
	else{
		printf("empty handle"); 
		return -1; 
	}
	if(handle_length > 100){
		printf("handle >100 chars"); 
		return -1; 
	}

	message[message_len-1] = 0; //make newline null terminator 
	//break up message if needed 
	int ptr = 0; 
	for(int i = 0; i < message_len/199; i++){
		send_M_packet((uint8_t*)(target_handle), (uint8_t*)(message+1 + ptr), handle_length, 199, 5);
		ptr += 199; 
	} 
	if(message_len%199 != 0 || message_len < 199){
		send_M_packet((uint8_t*)(target_handle), (uint8_t*)(message+1), handle_length, message_len%199, 5);
	}
	return 0;
}

//
int B_type(){
	char message[MAXBUF]; 
	int message_len =0;
	fgets(message, MAXBUF, stdin); 
	message_len = strlen(message);	
	message[message_len-1] = 0; 
	int ptr = 0; 
	for(int i = 0; i < message_len/199; i++){
		send_B_packet((uint8_t*)(message+1 + ptr), 199);
		ptr += 199; 
	} 
	if(message_len%199 != 0 || message_len < 199){
		send_B_packet((uint8_t*)(message+1), message_len%199);
	}
	return 0; 
	// return send_B_packet((uint8_t*)(message+ 1), message_len-2);
}


int C_type(){
	char handles[MAXHANDLE * 9]; 
	char message[MAXBUF]; 
	uint8_t num_handles; 
	scanf("%hhd", &num_handles); 
	if(num_handles < 2 || num_handles > 9){
		printf("number of handles out of range"); 
		return -1; 
	} 
	char current_value[MAXHANDLE] = {0}; 
	uint8_t lengths[9] = {0}; 
	uint8_t length; 
	int counter = 0; 
	for(int i = 0; i < num_handles; i++){
		scanf("%s", current_value); 
		length = strlen(current_value); 
		if(length <= 100 && (strncmp(current_value, "\n", MAXHANDLE) != 0)) {
			lengths[i] = length; 
			memcpy(handles + counter, current_value, length);
			counter += length; 
			memset(current_value, 0, MAXHANDLE); 
		}
		else{
			printf("handle %d too long", i); 
			return -1; 
		}
	}
	int message_len; 
	if(handle[0] != '\n'){
		fgets(message, MAXBUF, stdin); 
		message_len = strlen(message); 
	} 	
	message[message_len-1] = 0;  
	int ptr = 0; 
	for(int i = 0; i < message_len/199; i++){
		send_C_packet(num_handles, (uint8_t*)(handles), lengths, 199, (uint8_t*)(message+1 + ptr));
		ptr += 199; 
	} 
	if(message_len%199 != 0 || message_len < 199){
		send_C_packet(num_handles, (uint8_t*)(handles), lengths, message_len%199, (uint8_t*)(message+1));
	}
	//return send_C_packet(num_handles, (uint8_t*)handles, lengths, message_len, (uint8_t*)(message+1)); 
	//}
	return 0; 
}

int initClient(){
	if(handle_length > 100){
		printf("handle > 100 chars");
		return -1; 
	}
	send_init(); 
	while(1){
		int socket = pollCall(POLL_WAIT_FOREVER); 
		if(socket == socketNum){
			uint8_t buffer; 
			recvPDU(socket, &buffer, 1); 
			if(buffer == 2){
				return 0; 
			}
			else{
				printf("handle already in use\n"); 
				return -1; 
			}
		}
	}
}

void handleClient(){
	uint8_t buffer[MAXBUF];
	while(1){
		printf("$:");
		fflush(stdout);
		int socket = pollCall(POLL_WAIT_FOREVER);
		if(socket == STDIN_FILENO){
			scanf("%s", buffer);
			if(strncmp((char*)buffer, "%M", 3) == 0 || strncmp((char*)buffer, "%m", 3) == 0){
				M_type(); 
			}
			else if(strncmp((char*)buffer, "%C", 3) == 0 || strncmp((char*)buffer, "%c", 3)== 0){
				C_type(); 
			}
			else if(strncmp((char*)buffer, "%B", 3) == 0 || strncmp((char*)buffer, "%b", 3)== 0){
				B_type();  
			}
			else if(strncmp((char*)buffer, "%L", 3) == 0 || strncmp((char*)buffer, "%l", 3)== 0){
				send_L_packet(); 
			}
			else{
				printf("incorrect parameter\n"); 
			}
		}
		else{
			int bytes = recvPDU(socketNum, buffer, MAXBUF); 
			if(bytes > 0){
				parse_buffer(buffer, bytes); 
				// printf("Message received on socket %d, length: %ld Data: %s\n", socketNum, strlen((char *)buffer), buffer);
			}
			else if(bytes == 0){
				printf("Server has terminated\n");
				exit(0);
			}
			else{
				perror("recv error");
			}
		}
	}
}

void handle4(uint8_t* buffer, int total_length){
	uint8_t handle[MAXHANDLE + 1]; 
	uint8_t len = get_print_handle(handle, buffer); 
	char message[MAXBUF]; 
	memcpy(message, buffer + 1 + len, total_length - len - 2); 
	printf("%s\n", message);  
}

void handle6(uint8_t* buffer, int total_length){
	uint8_t handle[MAXHANDLE + 1]; 
	uint8_t len = get_print_handle(handle, buffer); 
	uint8_t num_handles = buffer[1 + len]; 
	int ptr = 2+ len; 
	for(int i = 0; i < num_handles; i++){
		ptr += buffer[ptr] + 1; 
	}
	printf("%s\n", buffer + ptr); 
}


void parse_buffer(uint8_t* buffer, int total_length){
	uint8_t flag = buffer[0]; 
	switch(flag){
		case 4:
			handle4(buffer + 1, total_length);
            break;

		case 5:
			handle5(buffer+1, total_length); 
			break; 
		case 6:
			handle6(buffer + 1, total_length); 
		break; 

		case 7: 
			handle7(buffer + 1); 
		break; 

		case 11: 
			handle11(buffer + 1); 
		break; 

		case 12: 
			handle12(buffer + 1); 
		break;

		case 13: 
			printf("\n"); //just avoid double $: cause I don't think there's anything to actually do here  
		break; 
	}
}

void handle5(uint8_t* buffer, int total_length){
	uint8_t handle[MAXHANDLE + 1]; 
	uint8_t len = get_print_handle(handle, buffer); 
	char message[MAXBUF]; 
	memcpy(message, buffer + 1 + len + handle_length + 2, total_length - len  - handle_length- 4); 
	message[total_length-len-handle_length-4] = 0;
	printf("%s\n", message); 
}

void handle7(uint8_t* buffer){
	uint8_t len = buffer[0]; 
	char handle[MAXHANDLE]; 
	memcpy(handle, buffer + 1, len); 
	handle[len] = 0; //add null terminator 
	printf("Client with handle %s does not exist\n", handle);
}

void handle11(uint8_t* buffer){
	uint32_t network_order; 
	memcpy(&network_order, buffer, sizeof(uint32_t)); 
	uint32_t num = ntohl(network_order); 
	printf("Number of clients: %d\n", num); 
}

void handle12(uint8_t* buffer){
	uint8_t len = buffer[0]; 
	char handle[MAXHANDLE]; 
	memcpy(handle, buffer + 1, len); 
	handle[len] = 0; 
	printf("\t%s\n", handle); 
}

int send_M_packet( uint8_t* handle1, uint8_t* message, uint8_t handle_len, int message_len, uint8_t flags){
	uint8_t buffer[MAXBUF]; 
	int ptr = 0; 
	buffer[ptr++] = flags; 
	buffer[ptr++] = handle_length; 
	memcpy(buffer + ptr, handle, handle_length); 
	ptr += handle_length; 
	buffer[ptr++] = 1; 
	buffer[ptr++] = handle_len;
	memcpy(buffer + ptr, handle1, handle_len); 
	ptr += handle_len; 
	memcpy(buffer + ptr, message, message_len); 
	int size = handle_length + handle_len + message_len + 4; 
	return sendPDU(socketNum, buffer, size); 
} 

int send_B_packet( uint8_t* message, int message_len){
	uint8_t buffer[MAXBUF]; 
	int ptr = 0; 
	buffer[ptr++] = 4; 
	buffer[ptr++] = handle_length; 
	memcpy(buffer + ptr, handle, handle_length); 
	ptr += handle_length; 
	memcpy(buffer + ptr, message, message_len); 
	// buffer[ptr + message_len] =  0; 
	int size = handle_length + message_len + 2; 
	return sendPDU(socketNum, buffer, size); 
}  

int send_C_packet( uint8_t numhandles, uint8_t* handles, uint8_t* lengths, int message_len, uint8_t* message){
	if(numhandles < 2 || numhandles > 9){
		return -1; 
	}
	uint8_t buffer[MAXBUF]; 
	int ptr = 0; 
	buffer[ptr++] = 6; 
	buffer[ptr++] = handle_length; 
	memcpy(buffer + ptr, handle, handle_length); 
	ptr += handle_length; 
	int ptr2 = 0; 
	buffer[ptr++] = numhandles; 
	for(int i = 0; i < numhandles; i++){
		buffer[ptr++] = lengths[i]; 
		memcpy(buffer + ptr, handles + ptr2, lengths[i]); 
		ptr += lengths[i]; 
		ptr2 +=lengths[i]; 
	}
	memcpy(buffer + ptr, message, message_len); 
	int size = ptr + message_len; 
	return sendPDU(socketNum, buffer, size); 
}  

int send_L_packet(){
	uint8_t buffer = 10; 
	return sendPDU(socketNum, &buffer, 1); 
}

int send_init(){
	uint8_t buffer[MAXHANDLE]; 
	int ptr = 0; 
	buffer[ptr++] = 1; 
	buffer[ptr++] = handle_length; 
	memcpy(buffer + ptr, handle, handle_length);  
	int size = handle_length + 2; 
	return sendPDU(socketNum, buffer, size); 
}


void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle-name host-name port-number \n", argv[0]);
		exit(1);
	}
}
