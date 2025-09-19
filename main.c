#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_MESSAGE_SIZE 512

// Global socket descriptor for cleanup
int sockfd = -1;

// Thread function to handle receiving messages
void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            printf("\n[Connection lost or peer disconnected]\n");
            close(sock);
            exit(0);
        }
        
        // Print received message immediately
        printf("Peer: %s", buffer);
        fflush(stdout);
    }
    
    return NULL;
}

// Signal handler for clean exit
void signal_handler(int sig) {
    printf("\n[Exiting...]\n");
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

// Server mode: listen for incoming connection
int run_server(int port) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Allow socket reuse
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_sock, 1) < 0) {
        perror("Listen failed");
        close(server_sock);
        return -1;
    }
    
    printf("Server listening on port %d...\n", port);
    printf("Waiting for client to connect...\n");
    
    // Accept one connection
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        perror("Accept failed");
        close(server_sock);
        return -1;
    }
    
    printf("Client connected from %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    printf("Chat started! Type messages and press Enter.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    close(server_sock); // No longer need the listening socket
    return client_sock;
}

// Client mode: connect to server
int run_client(const char* server_ip, int port) {
    int client_sock;
    struct sockaddr_in server_addr;
    
    // Create socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid server IP address\n");
        close(client_sock);
        return -1;
    }
    
    printf("Connecting to %s:%d...\n", server_ip, port);
    
    // Connect to server
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_sock);
        return -1;
    }
    
    printf("Connected to server!\n");
    printf("Chat started! Type messages and press Enter.\n");
    printf("Press Ctrl+C to exit.\n\n");
    
    return client_sock;
}

int main(int argc, char* argv[]) {
    // Set up signal handler for clean exit
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Check command line arguments
    if (argc < 3) {
        printf("Usage:\n");
        printf("  Server mode: %s server <port>\n", argv[0]);
        printf("  Client mode: %s client <server_ip> <port>\n", argv[0]);
        return 1;
    }
    
    int sock;
    
    // Determine mode and establish connection
    if (strcmp(argv[1], "server") == 0) {
        if (argc != 3) {
            printf("Server usage: %s server <port>\n", argv[0]);
            return 1;
        }
        int port = atoi(argv[2]);
        sock = run_server(port);
    }
    else if (strcmp(argv[1], "client") == 0) {
        if (argc != 4) {
            printf("Client usage: %s client <server_ip> <port>\n", argv[0]);
            return 1;
        }
        const char* server_ip = argv[2];
        int port = atoi(argv[3]);
        sock = run_client(server_ip, port);
    }
    else {
        printf("Invalid mode. Use 'server' or 'client'\n");
        return 1;
    }
    
    if (sock < 0) {
        printf("Failed to establish connection\n");
        return 1;
    }
    
    sockfd = sock; // Store for signal handler
    
    // Create thread for receiving messages
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_messages, &sock) != 0) {
        perror("Failed to create receive thread");
        close(sock);
        return 1;
    }
    
    // Main thread handles sending messages
    char message[MAX_MESSAGE_SIZE];
    while (1) {
        // Read message from user
        if (fgets(message, MAX_MESSAGE_SIZE, stdin) == NULL) {
            break; // EOF or error
        }
        
        // Send message to peer
        int bytes_sent = send(sock, message, strlen(message), 0);
        if (bytes_sent <= 0) {
            printf("Failed to send message or connection lost\n");
            break;
        }
    }
    
    // Cleanup
    close(sock);
    return 0;
}