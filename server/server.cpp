#include "include.h" 
#include "server.h" 
using namespace std;


// Lock for multi-process 
mutex mtx;


server::server() { 
    // Create the socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[ERROR] socket failed!!!"); 
        exit(EXIT_FAILURE);
    } 
}


server::~server() { 
    // Close the file descriptor 
    close(server_fd); 
}


// Thread function, handle the request from a single client 
void *thread_func(void *arg) {
    int client_fd = *(int*)arg; 
    
    // Send the hello message 
    char hello_string[] = "Hello~"; 
    send(client_fd, hello_string, sizeof(hello_string), 0); 
    cout << "Sent hello to client " << client_fd << endl; 

    // Set connection state to true 
    // The varible connection_state means the connection state between server and the client 
    bool connection_state = true; 
    packet pck; 
    while (connection_state == true) { // While the connection exsits
        if ((recv(client_fd, &pck, sizeof(pck), 0)) < 0) {
            perror("[ERROR] fail to receive the message!!!"); 
            continue; 
        }

        // Attain a lock to handle the multi-process 
        mtx.lock(); 

        // Handle different requests 
        if (pck.type == REMOVE_CONNECT) { 
            // Scan the client list to find the one with the same fd 
            for (int i = 0; i < client_queue.size(); ++ i) {
                if (client_queue[i].socket_id == client_fd) { 
                    // Remove it from the client list 
                    vector<element>::iterator it = client_queue.begin() + i; 
                    client_queue.erase(it); 
                    break; 
                }
            } 
            cout << "removed connection from " << client_fd << endl; 

            // Close the file descriptor, and set the connection state to false 
            close(client_fd); 
            connection_state = false; 
        }

        else if (pck.type == GET_TIME) {
            // Get time 
            time_t t; 
            time(&t); 

            // Assemble message 
            packet pck_send; 
            pck_send.type = GET_TIME; 
            sprintf(pck_send.data, "%ld", t); 
            
            // Send message 
            if ((send(client_fd, &pck_send, sizeof(pck_send), 0)) < 0) {
                perror("[ERROR] fail to send the message of the gotten time!!!"); 
            }
            cout << "Sent time data to client " << client_fd << endl; 
        }

        else if (pck.type == GET_NAME) {
            // Assemble message 
            packet pck_send; 
            pck_send.type = GET_NAME; 
            gethostname(pck_send.data, sizeof(pck_send.data)); 
            // cout << "get host name" << endl; 
            // Send message 
            if ((send(client_fd, &pck_send, sizeof(pck_send), 0)) < 0) {
                perror("[ERROR] fail to send the message of the server name!!!"); 
            }
            cout << "Send server name to client" << client_fd << endl; 
        }

        else if (pck.type == GET_LIST) {
            // Assemble message 
            packet pck_send; 
            memset(&pck_send, 0, sizeof(pck_send)); 
            pck_send.type = GET_LIST; 

            // Scan the client list to synthesize the response 
            for (int i = 0; i < client_queue.size(); ++ i) {
                string tmp = "client: " + to_string(client_queue[i].socket_id) + ", " + client_queue[i].addr + ":" + to_string(client_queue[i].port) + "\n"; 
                strcat(pck_send.data, tmp.c_str()); 
            }

            // Send message 
            if ((send(client_fd, &pck_send, sizeof(pck_send), 0)) < 0) {
                perror("[ERROR] fail to send the message of the server list!!!"); 
            } 
            cout << "Send client list data to client" << client_fd << endl;
        }

        else if (pck.type == SEND_MSG) {
            // Analyse the receive data 
            string data = string(pck.data); 
            string ip = data.substr(0, data.find(":")); 
            data = data.substr(data.find(":") + 1); 
            int port = stoi(data.substr(0, data.find(","))); 
            data = data.substr(data.find(",") + 1); 

            // Determine the target client 
            int ind = -1; 
            for (int i = 0; i < client_queue.size(); ++ i) {
                if (client_queue[i].addr == ip && client_queue[i].port == port) {
                    ind = i; 
                    break; 
                }
            }

            // Two things to do: 
            // 1. send message to the target client(if have) 
            // 2. send feedback message to the the source client 
            packet pck_feedback; 
            pck_feedback.type = SEND_MSG; 
            if (ind == -1) { // Can't find the target client 
                sprintf(pck_feedback.data, "[ERROR] there is no such client!!!"); 
            }
            else {
                sprintf(pck_feedback.data, "[INFO] message sent seccessfully!"); 

                // Assemble the message 
                packet pck_forward; 
                pck_forward.type = RECEIVE_MSG; 
                strcpy(pck_forward.data, data.c_str()); 

                // Send message to another client 
                if ((send(client_queue[ind].socket_id, &pck_forward, sizeof(pck_forward), 0)) < 0) {
                    cout << "[ERROR] fail to send the message to another client!!!" << endl; 
                }
                else {
                    cout << "Resend message to another client" << endl;
                }
                
            }

            // Send feedback message 
            if ((send(client_fd, &pck_feedback, sizeof(pck_feedback), 0)) < 0) {
                cout << "[ERROR] fail to send the feedback message to the source client!!!" << endl; 
            }
        } 

        // Clear message structure, get ready for the next request 
        memset(&pck, 0, sizeof(pck)); 
        mtx.unlock(); 
    } 
}


void server::run() {
    // Ready for connection 
    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET; 
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    addr.sin_port = htons(PORT); 

    // Connect 
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)); 

    // Listen
    listen(server_fd, QUEUE_LENGTH); 
    cout << "Start listening..." << endl; 

    while (1) {
        struct sockaddr_in client_addr; 
        socklen_t client_addr_len = sizeof(client_addr); 
        cout << "Waiting for connection..." << endl; 

        // Accept the connection from client 
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len); 

        // Compile the client data, and store it into the list 
        struct element client_info; 
        client_info.socket_id =  client_fd; 
        client_info.addr = inet_ntoa(client_addr.sin_addr); 
        client_info.port = ntohs(client_addr.sin_port); 
        client_queue.push_back(client_info); 
        cout << "connect from " + client_info.addr + ":" + to_string(client_info.port) + " at " + to_string(client_fd) << endl; 

        // Create a thread to handle the request from this client 
        pthread_t thr; 
        pthread_create(&thr, NULL, thread_func, &client_fd); 
    }
}


int main() {
    server sev; 
    sev.run(); 
    return 0; 
}
