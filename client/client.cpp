#include "client.h" 
#include "include.h"
using namespace std; 


// Thread function, run the receive process 
void *thread_func(void *arg) {
	int sockfd = *(int*)arg; 
	packet pck; 

	// Create the message queue to achieve inter-process communication 
	key_t key = ftok(filepath, 5); 
	int msg_id = msgget(key, IPC_CREAT | 0666); 
	if (msg_id < 0) {
		perror("[ERROR] fail to create the message queue!!!");
	} 

	// Receive the response message 
	recv(sockfd, &pck.data, sizeof(pck.data), 0); 
	cout << "received message: " << pck.data << endl;

	while (1) { 
		memset(&pck, 0, sizeof(pck)); 

		// Receive the request from server  
		if ((recv(sockfd, &pck, sizeof(pck), 0)) < 0) {
			perror("[ERROR] fail to receive the message!!!");
		}

		// Print the message if the message type is RECEIVE_MSG 
		if (pck.type == RECEIVE_MSG) {
			cout << "received message: " << pck.data << endl; 
			continue; 
		}

		// Send the message to main process
		msgsnd(msg_id, &pck, MAXLEN, 0); 
	}
}


client::client() { 
	// Create a socket handler 
	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("[ERROR] socket failed");
        exit(EXIT_FAILURE);
	}

	// Clear sockaddr_in 
	memset(&addr, 0, sizeof(addr)); 

	// Create the message queue to achieve inter-process communication 
	key_t key = ftok(filepath, 5); 
	msg_id = msgget(key, IPC_CREAT | 0666); 
	if (msg_id < 0) {
		perror("[ERROR] fail to create the message queue!!!");
		return; 
	} 
	
	// Filter the remaining messages 
	char pck[MAXLEN]; 
	while (msgrcv(msg_id, &pck, MAXLEN, 0, IPC_NOWAIT) > 0); 
}


client::~client() {
	// Close the file descriptor
	close(client_fd); 
}


int client::fetch_operation() {
	int op = 0; 

	// Print the menu 
	cout << "---------------------------------------" << endl; 
	cout << "|Please enter your requirement:       |" << endl; 
	cout << "|1. connect to the client.            |" << endl; 
	cout << "|2. remove connection.                |" << endl; 
	cout << "|3. get time.                         |" << endl; 
	cout << "|4. get server name.                  |" << endl; 
	cout << "|5. list all client info.             |" << endl; 
	cout << "|6. send message to another client.   |" << endl; 
	cout << "|7. exit.                             |" << endl;
	cout << "---------------------------------------" << endl;  

	// Fetch the operation 
	cin >> op; 
	return op; 
}


void client::run() {
	while (1) {
		// Fetch the operation 
		int op = fetch_operation(); 

		// If no operation is fetched, then continue 
		if (op == 0) {
			continue; 
		}

		// Handle different operations 
		else if (op == 1) {
			server_connect(); 
		}
		else if (op == 2) {
			remove_connection(); 
		}
		else if (op == 3) {
			get_time(); 
		}
		else if (op == 4) {
			get_name(); 
		}
		else if (op == 5) {
			get_client_list(); 
		}
		else if (op == 6) {
			send_message(); 
		}
		else if (op == 7) {
			quit(); 
		}

		// Not a valid operation 
		else {
			cout << "[ERROR] Please enter the valid operation!!!" << endl; 
		}
	}
}


void client::server_connect() { 
	char ip[MAXLEN];
	int port; 

	// If the connection already exist 
	if (connection_state == true) {
		perror("[ERROR] connection already exist!!!");
		return; 
	}

	// Fetch the ip and port 
	cout << "Please enter the ip address: "; 
	scanf("%s", ip); 
	cout << "Please enter the port: "; 
	scanf("%d", &port); 

	// Ready for connection 
	addr.sin_family = AF_INET; 
	addr.sin_port = htons(port); 
	addr.sin_addr.s_addr = inet_addr(ip); 

	// Connect to the target server 
	if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("[ERROR] fail to create the message queue!!!");
        return; 
	}

	// Create a new thread 
	if (pthread_create(&thr, NULL, thread_func, &client_fd) != 0) {
		perror("[ERROR] fail to create the thread!!!"); 
	}

	// Change connection state to true, and print the success message 
	connection_state = true; 
	cout << "[INFO] connection success!!!" << endl; 
}


void client::remove_connection() {
	// If there is no connection 
	if (connection_state == false) {
		perror("[ERROR] no connection!!!"); 
		return; 
	}

	// Compile the REMOVE_CONNECT request 
	char data = REMOVE_CONNECT; 
	if (send(client_fd, &data, sizeof(data), 0) < 0) { 
		perror("[ERROR] fail to remove connection!!!"); 
		return; 
	}

	// Cancel the thread process 
	pthread_cancel(thr); 

	// Switch the connection state to false 
	connection_state = false; 
	cout << "[INFO] remove connection seccess!!!" << endl; 
}


void client::get_time() {
	// Check the connection state first 
	if (connection_state == false) {
		perror("[ERROR] no such connection!!!"); 
		return; 
	}

	// Send the GET_TIME message 
	char data = GET_TIME; 
	if (send(client_fd, &data, sizeof(data), 0) < 0) {
		perror("[ERROR] fail to send the request of getting time!!!"); 
		return; 
	}

	// Receive the messsage sent from thread process 
	packet pck; 
	if (msgrcv(msg_id, &pck, MAXLEN, (long)GET_TIME, 0) < 0) {
		perror("[ERROR] fail to receive the data of getting time!!!"); 
		return; 
	}

	// Process the received time string
	time_t t; 
	sscanf(pck.data, "%ld", &t); 

	// Print the time message 
	cout << "Server time: " << ctime(&t) << endl; 
} 


void client::get_name() { 
	// Check the connection state first 
	if (connection_state == false) {
		perror("[ERROR] no such connection!!!"); 
		return; 
	}

	// Send the GET_NAME message 
	char data = GET_NAME; 
	if (send(client_fd, &data, sizeof(data), 0) < 0) {
		perror("[ERROR] fail to send the request of getting name!!!"); 
		return; 
	}

	// Receive the message sent from thread process 
	packet pck; 
	if (msgrcv(msg_id, &pck, MAXLEN, (long)GET_NAME, 0) < 0) {
		perror("[ERROR] fail to receive the data of getting name!!!"); 
		return; 
	}

	// Print the name 
	cout << "Servel name: ";
	puts(pck.data); 
}


void client::get_client_list() {
	// Check the connection state first 
	if (connection_state == false) {
		perror("[ERROR] no such connection!!!"); 
		return; 
	}

	// Send the GET_LIST message 
	char data = GET_LIST; 
	if (send(client_fd, &data, sizeof(data), 0) < 0) {
		perror("[ERROR] fail to send the request of getting list!!!"); 
		return; 
	}

	// Receive the message sent from thread process 
	packet pck; 
	if (msgrcv(msg_id, &pck, MAXLEN, (long)GET_LIST, 0) < 0) {
		perror("[ERROR] fail to receive the data of getting list!!!"); 
		return; 
	}

	// Print the name 
	cout << "Client list: " << endl; 
	puts(pck.data); 
}


void client::send_message() {
	// Check the connection state first 
	if (connection_state == false) {
		perror("[ERROR] no such connection!!!"); 
		return; 
	}

	// Assemble the packet 
	// The format is ip:port,data
	packet pck; 
	pck.type = SEND_MSG; 
	char ip[MAXLEN];
	int port; 
	cout << "Please enter the target ip address: "; 
	scanf("%s", ip); 
	cout << "Please enter the port: "; 
	scanf("%d", &port); 
	sprintf(pck.data, "%s:%d,", ip, port); //',' means the end of address 
	
	char msg[MAXLEN - 30]; // Spare some space for address 
	cout << "Please enter the message content: "; 
	scanf("%s", msg); 
	sprintf(pck.data + strlen(pck.data), "%s", msg); 

	// Send the SEND_MSG message 
	if (send(client_fd, &pck, sizeof(pck), 0) < 0) {
		perror("[ERROR] fail to send messages to other client!!!"); 
		return; 
	}

	// Receive the response 
	memset(&pck, 0, sizeof(pck));
	if (msgrcv(msg_id, &pck, MAXLEN, (long)SEND_MSG, 0) < 0) {
		perror("[ERROR] fail to receive the response of server when sending a message!!!"); 
		return; 
	}

	// Print the response 
	cout << "Server response: ";
	puts(pck.data); 
}


void client::quit() {
	// If the connection state is true, need to remove it first 
	if (connection_state == true) {
		// Send the REMOVE_CONNECT request 
		char data = REMOVE_CONNECT; 
		if (send(client_fd, &data, sizeof(data), 0) < 0) {
			perror("[ERROR] fail to send the request of quiting!!!"); 
			return; 
		} 
	}

	// Exit the program 
	exit(0); 
} 

int main() { 
	// Run the client 
	client cli; 
	cli.run(); 
	return 0; 
}