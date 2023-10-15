#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main(){
    
    int socket_connection = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9092);
    
    bind(socket_connection, (struct sockaddr *)&address, sizeof(address));
    listen(socket_connection, 5);

    int client_socket;
    client_socket = accept(socket_connection, NULL, NULL);
    const char *message = "Hello, you are now connected with server!";
    send(client_socket, message, strlen(message), 0 );

    return 0;
}