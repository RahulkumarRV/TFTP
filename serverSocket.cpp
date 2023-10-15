#include <iostream>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    const char* ip = "142.250.188.46";
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(80);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int result = connect(serverSocket, (struct sockaddr*)&address, sizeof(address));

    if (result == 0)
        printf("Connected successfully\n");
    else
        printf("Connection failed\n");

    
    const char* message = "GET // HTTP/1.1\r\nHost:google.com\r\n\r\n";
    send(serverSocket, message, strlen(message), 0);

    char buffer[1024];
    recv(serverSocket, buffer, 1024, 0);

    printf("response : %s\n", buffer);

    return 0;
}
