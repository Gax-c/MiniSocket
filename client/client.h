#ifndef _CLIENT_H_ 
#define _CLIENT_H_ 

#include "include.h"

class client {
private:
    int client_fd, msg_id;
    bool connection_state = false; 
    sockaddr_in addr;  
    pthread_t thr; 

    // Fetch the entered operation
    int fetch_operation(); 

    // Connect to the server 
    void server_connect();

    // Cancel connection 
    void remove_connection();

    // Get time 
    void get_time(); 

    // Get server name 
    void get_name(); 

    // List the clients connected to the server 
    void get_client_list(); 

    // Send a message to the server 
    void send_message(); 

    // Quit the client 
    void quit(); 

public:
    client(); 

    ~client();
    
    // Run the client
    void run(); 
};

#endif 