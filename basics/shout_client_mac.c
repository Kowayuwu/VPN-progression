#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

int main(){
    /*
    AL_INET - IPv4 Internet protocal\
    SOCK_DGRAM - UDP
    0 - just says that only a single protocal exist to support this particular socket, revisit socket man page for more detail
    */
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket == -1){
        perror("failed to initialize socket\n");
        exit(1);
    }

    // Server address
    const sa_family_t SERVER_SIN_FAMILY = AF_INET;
    const in_port_t SERVER_SIN_PORT = 50000;
    const struct in_addr SERVER_SIN_ADDR = {
        .s_addr = htonl(INADDR_LOOPBACK)
    };
    const struct sockaddr_in SERVER_SOCKET_ADDRESS = {
        .sin_family = SERVER_SIN_FAMILY,
        .sin_port = htons(SERVER_SIN_PORT),
        .sin_addr = SERVER_SIN_ADDR

    }; 
    const socklen_t SERVER_SOCKET_ADDR_LEN = sizeof(SERVER_SOCKET_ADDRESS);

    char line[256];
    int i;
    while(1){
        printf("message: ");
        if (fgets(line, sizeof(line), stdin)) {
            // fgets adds a new line at the end, replace it with \0 so it is a valid string
            line[strcspn(line, "\n")] = 0;
            int n = sendto(udp_socket, line, strlen(line), 0, (const struct sockaddr *) &SERVER_SOCKET_ADDRESS, (socklen_t)SERVER_SOCKET_ADDR_LEN);
            
            if(n < 0){
                perror("Send message failed, maybe the server is not up yet?\n");
                continue;
            }
            printf("waiting for server to respond\n");
            // No need to extract senders' addr info over here so we set last 2 parameters as null
            ssize_t recv_msg_len = recvfrom(udp_socket, line, sizeof(line) - 1, 0, NULL, NULL);
            if(recv_msg_len < 0){
                perror("error while receiving from server\n");
                continue;
            }

            line[recv_msg_len] = '\0';
            printf("recieved: %s\n", line);

                
        }else{
            // EOF or error
            break;
        }
    }

    if(close(udp_socket) == -1){
        perror("failed to close socket\n"); 
        exit(1);
    }
    return 0;

}