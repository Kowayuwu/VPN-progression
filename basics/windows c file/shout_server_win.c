#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#define CLOSE_SOCKET(s) close(s)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

int main()
{
#ifdef _WIN32
    WSADATA wsa_data;
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_result != 0)
    {
        fprintf(stderr, "WSAStartup failed: %d\n", wsa_result);
        exit(1);
    }
#endif

    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "failed to initialize socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

    // Bind the socket
    struct sockaddr_in socket_address;
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(50000);
    socket_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t socket_addr_len = sizeof(socket_address);

    if (bind(udp_socket, (struct sockaddr *)&socket_address, socket_addr_len) == SOCKET_ERROR)
    {
        fprintf(stderr, "failed to bind socket\n");
        CLOSE_SOCKET(udp_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

    // Set timeout for recvfrom
    // Windows uses DWORD milliseconds, POSIX uses struct timeval
#ifdef _WIN32
    DWORD timeout_ms = 10000; // 10 seconds in milliseconds
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO,
                   (const char *)&timeout_ms, sizeof(timeout_ms)) == SOCKET_ERROR)
    {
        fprintf(stderr, "error when calling setsockopt\n");
        CLOSE_SOCKET(udp_socket);
        WSACleanup();
        exit(1);
    }
#else
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("error when calling setsockopt");
        CLOSE_SOCKET(udp_socket);
        exit(1);
    }
#endif

    const int MESSAGE_LIMIT = 5;
    int message_count = 0;

    while (message_count < MESSAGE_LIMIT)
    {
        char buffer[1024] = {0};
        struct sockaddr_in sender; // use sockaddr_in instead of generic sockaddr
        socklen_t sender_addr_length = sizeof(sender);

        int recv_msg_len = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                                    (struct sockaddr *)&sender, &sender_addr_length);
        if (recv_msg_len == SOCKET_ERROR)
        {
#ifdef _WIN32
            // On Windows, check WSAGetLastError() instead of errno
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT)
            {
                printf("no data received within timeout, closing socket\n");
            }
            else
            {
                fprintf(stderr, "error when calling recvfrom: %d\n", err);
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("no data received within timeout, closing socket\n");
            }
            else
            {
                perror("error when calling recvfrom");
            }
#endif
        }
        else
        {
            buffer[recv_msg_len] = '\0';
            printf("received: %s\n", buffer);

            int sent = sendto(udp_socket, buffer, recv_msg_len, 0,
                              (const struct sockaddr *)&sender, sender_addr_length);
            if (sent == SOCKET_ERROR)
            {
                fprintf(stderr, "error when sending back the message\n");
            }
        }

        message_count++;
    }

    if (CLOSE_SOCKET(udp_socket) == SOCKET_ERROR)
    {
        fprintf(stderr, "failed to close socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
