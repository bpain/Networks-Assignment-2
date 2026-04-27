#include "handle_table.h"
#include <stdint.h> 
#include <string.h>
#include <stdio.h> 
#include <stdlib.h>

table_entry* handle_table; 
int table_size; 
uint32_t num_entries; 


void clear_table(){
    if(handle_table){ 
        // for(int i = 0; i < table_size; i++){
        //     free(handle_table[i]); 
        // }
        free(handle_table);
    }
}

uint32_t getsize(){
    return num_entries; 
}

uint8_t* get_lengths(){
    uint8_t* lengths = (uint8_t*)malloc(num_entries);
    int counter = 0; 
    for(int i = 0; i < table_size; i++){
        if(handle_table[i].valid){
            lengths[counter] = handle_table[i].handle_len; 
            counter++; 
        }
    } 
    return lengths; 
}


uint8_t* get_handles(){
    uint8_t* handles = (uint8_t*)malloc(num_entries*100);
    uint8_t* lengths = get_lengths(); 
    int ptr = 0; 
    int counter = 0; 
    for(int i = 0; i < table_size; i++){
        if(handle_table[i].valid){
            memcpy(handles + ptr, handle_table[i].handle, lengths[counter]); 
            ptr += lengths[counter];
            counter++; 
        }
    } 
    return handles; 
}

int* get_sockets(){
    int* sockets = (int*)malloc(num_entries); 
    int counter = 0; 
    for(int i = 0; i < table_size; i++){
        if(handle_table[i].valid){
            sockets[counter] = handle_table[i].socket; 
            counter ++; 
        }
    }
    return sockets; 
}


void add_element(int socket, uint8_t len, uint8_t* handle){
    table_entry new_val;
    memcpy(new_val.handle, handle, len); 
    new_val.socket = socket; 
    new_val.handle_len = len; 
    new_val.valid = 1; 
    if(handle_table){
        handle_table = (table_entry*)realloc(handle_table, ((table_size+1)*(sizeof(table_entry)))); 
        handle_table[table_size] = new_val; 
        table_size ++; 
        num_entries++; 
    }
    else{
        handle_table = (table_entry*)malloc(sizeof(table_entry)); 
        handle_table[0] = new_val; 
        table_size = 1; 
        num_entries = 1; 
    }
    return; 
}

int remove_element(uint8_t* val){
    if(handle_table){
        for(int i = 0; i < table_size; i++){
            if(!memcmp(handle_table[i].handle, val, handle_table[i].handle_len)){
                handle_table[i].valid = 0; 
                num_entries--; 
            }
        }
        return 0; 
    }
    else{
        perror("remove_value"); 
        return -1; 
    }
}

int get_socket(uint8_t* handle, uint8_t len){
    if(handle){
        for(int i = 0; i < table_size; i++){
            if(handle_table[i].handle_len == len){
                if(!memcmp(handle_table[i].handle, handle, handle_table[i].handle_len) && handle_table[i].valid){
                    return handle_table[i].socket; 
                }
            }
        }
        return 0; 
    }
    else{
        perror("get socket"); 
        return -1; 
    }
}

uint8_t* get_handle(int socket){
    for(int i = 0; i < table_size; i++){
        if((handle_table[i].socket == socket) && handle_table[i].valid){
            return handle_table[i].handle; 
        }
    }
    return 0; 
}

