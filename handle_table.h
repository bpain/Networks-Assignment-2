#ifndef HANDLE_TABLE
#define HANDLE_TABLE 
#include <stdint.h>

typedef struct{
    uint8_t valid; 
    int socket; 
    uint8_t handle_len; 
    uint8_t handle[100]; 
}table_entry; 


extern void clear_table(); 
extern void add_element(int socket, uint8_t len, uint8_t* handle); 
extern int remove_element(uint8_t* val); 
extern int get_socket(uint8_t* handle, uint8_t len);
extern int* get_sockets(); 
extern uint8_t* get_handle(int socket);
extern uint32_t getsize(); 
extern uint8_t* get_handles();
extern uint8_t* get_lengths(); 



#endif


