#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

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

    // Bind the socket
    const sa_family_t SIN_FAMILY = AF_INET;
    const in_port_t SIN_PORT = 50000;
    const struct in_addr SIN_ADDR = {
        .s_addr = htonl(INADDR_LOOPBACK)
    };
    const struct sockaddr_in SOCKET_ADDRESS = {
        .sin_family = SIN_FAMILY,
        .sin_port = htons(SIN_PORT),
        .sin_addr = SIN_ADDR

    }; 
    const socklen_t SOCKET_ADDR_LEN = sizeof(SOCKET_ADDRESS);
    
    if(bind(udp_socket, (struct sockaddr *)&SOCKET_ADDRESS, SOCKET_ADDR_LEN) == -1){
        perror("failed to bind socket\n");
        exit(1);
    }

    // Set time out for recvfrom 
    struct timeval tv;
    tv.tv_sec = 10;   // seconds
    tv.tv_usec = 0;  // microseconds

    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("error when calling setsockopt");
        exit(1);
    }

    const int MESSAGE_LIMIT = 5;
    int message_count = 0;
    while(message_count < MESSAGE_LIMIT){
        char buffer[1024] = {0};
        struct sockaddr sender;
        socklen_t sender_addr_length = sizeof(sender);

        ssize_t recv_msg_len = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0, &sender, &sender_addr_length);

        if (recv_msg_len < 0) {
            /* 
            Errno would be different depending on machine, both value means the same thing here in this context
            EAGAIN - "there is no data available right now, try again later" during non-blocking IO
            EWOULDBLOCK - "operation would block" - that is, the operation would have blocked, but the descriptor was placed in non-blocking mode. 
            */ 
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("no data received within timeout, closing socket\n");
                } else {
                    perror("error when calling recvfrom");
                }
        } else {
            buffer[recv_msg_len] = '\0';
            printf("received: %s\n", buffer);
            int sent = sendto(udp_socket, (const char *)buffer, (size_t) recv_msg_len, 0, (const struct sockaddr *) &sender, sender_addr_length);
            if(sent < 0){
                perror("error when sending back the message\n");
            }
        }
        message_count += 1;
    }
   


    if(close(udp_socket) == -1){
        perror("failed to close socket\n"); 
        exit(1);
    }
    return 0;

}
