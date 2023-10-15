#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    const char* ip = "121.0.0.1";
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(9092);
    // inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_addr.s_addr = INADDR_ANY;

    int result = connect(serverSocket, (struct sockaddr*)&address, sizeof(address));

    if (result == 0)
        printf("Connected successfully\n");
    else
        printf("Connection failed\n");

    char buffer[1024];

    recv(serverSocket, &buffer, sizeof(buffer), 0);
    printf("response : %s\n", buffer);

    return 0;
}
