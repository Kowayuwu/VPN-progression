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

    // Server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t server_addr_len = sizeof(server_addr);

    char line[256];
    while (1)
    {
        printf("message: ");
        if (fgets(line, sizeof(line), stdin))
        {
            // fgets adds a newline at the end, replace it with \0
            line[strcspn(line, "\n")] = 0;

            int n = sendto(udp_socket, line, (int)strlen(line), 0,
                           (const struct sockaddr *)&server_addr, server_addr_len);
            if (n == SOCKET_ERROR)
            {
                fprintf(stderr, "Send message failed, maybe the server is not up yet?\n");
                continue;
            }

            printf("waiting for server to respond\n");

            int recv_msg_len = recvfrom(udp_socket, line, sizeof(line) - 1, 0, NULL, NULL);
            if (recv_msg_len == SOCKET_ERROR)
            {
                fprintf(stderr, "error while receiving from server\n");
                continue;
            }

            line[recv_msg_len] = '\0';
            printf("received: %s\n", line);
        }
        else
        {
            // EOF or error
            break;
        }
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
