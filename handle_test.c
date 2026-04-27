#include "handle_table.h"
#include <string.h> 
#include <stdint.h>
#include <err.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>


void handler(int thing){
    clear_table(); 
    return; 
}

int main(){
    int len = 10000; 
    signal(SIGSEGV, handler); 
    for(int i = 0; i < len ; i++){
        char handle[10]; 
        sprintf(handle, "test%d", i);
        int socket = i + 3; 
        uint8_t valid = 1; 
        uint8_t handle_len = strlen(handle); 
        add_element(socket,handle_len, (uint8_t*)handle); 
        int result = get_socket(handle, handle_len);
        int size = getsize(); 
        if(size != i + 1){
            printf("wrong length: expected %d, real %d\n", i + 1, size); 
        }
        if(result != (i + 3)){
            printf("wrong socket: expected %d, real %d\n", i + 3, result); 
        }
        
    }
    
    uint8_t* lengths = (uint8_t*)malloc(len); 
    uint8_t* handles = (uint8_t*)malloc(getsize()*100); 
    lengths = get_lengths(); 
    handles = get_handles(); 
    int ptr = 0; 

    for(int i = 0; i < len; i++){
        char handle[10];
        sprintf(handle, "test%d", i);
        int real_len = strlen(handle); 
        if(lengths[i] != real_len){
            printf("wrong length fetched expected %d, real %d\n", real_len, lengths[i]); 
        }
        char* string = (char*)malloc(lengths[i] + 1);
        memcpy(string, handles+ptr, lengths[i]); 
        if(string + lengths[i]){
            string[lengths[i]] = 0; 
        }
        if(strcmp(string, handle)){
            
            printf("wrong handle fetched, expected %s, real %s\n", handle, string); 
        }
        ptr += lengths[i]; 
    }

    for(int i = 0;  i<len; i++){
        char handle[10]; 
        sprintf(handle, "test%d", i);
        int real_len = strlen(handle); 
        remove_element(handle); 
        if(get_socket(handle, real_len)){
            printf("remove failed: %d\n", i);
        }
    }
    printf("all done\n"); 
    clear_table(); 
    return 0; 

}