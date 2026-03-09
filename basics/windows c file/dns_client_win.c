#include "dns.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SSIZE_T ssize_t;
typedef int socklen_t; // already defined in some versions, guard if needed
#define CLOSE_SOCKET(s) closesocket(s)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#define CLOSE_SOCKET(s) close(s)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

/*
RFC doc: https://www.ietf.org/rfc/rfc1035.txt
Calling DNS server: 8.8.8.8, own by Google
*/

/**
 * Encodes a hostname (e.g., "www.google.com") into DNS qname format.
 * Format: [len]label[len]label[0]
 *
 * Returns the length of the encoded message
 */
int encode_dns_qname(uint8_t *buffer, const char *hostname)
{
    int i, buffer_i = 0;
    char name_copy[256];

    strncpy(name_copy, hostname, 255);
    name_copy[255] = '\0';

    char *token = strtok(name_copy, ".");
    while (token != NULL)
    {
        size_t len = strlen(token);
        buffer[buffer_i++] = (uint8_t)len;
        for (i = 0; i < (int)len; i++)
        {
            buffer[buffer_i++] = token[i];
        }
        token = strtok(NULL, ".");
    }

    buffer[buffer_i] = '\0';
    return buffer_i + 1;
}

/**
 * Decodes a DNS qname formatted string to its original hostname.
 * Qname format: [len]label[len]label[0]
 *
 * Returns the length of qname, including the 0 terminator
 */
int decode_dns_qname(uint8_t *data)
{
    int i = 0;

    while (data[i] != 0)
    {
        uint8_t sub_str_len = data[i];
        i++;

        for (int j = 0; j < sub_str_len; j++)
        {
            putchar(data[i + j]);
        }

        i += sub_str_len;

        if (data[i] != 0)
        {
            putchar('.');
        }
    }
    putchar('\n');

    return i + 1;
}

/**
 * Prints an IPv4 address from raw bytes.
 */
void print_ipv4_address(uint8_t *ptr, uint16_t answer_rdlength)
{
    printf("IP address: [[  ");
    for (int i = 0; i < answer_rdlength; i++)
    {
        printf("%d", *(ptr + i));
        if (i != answer_rdlength - 1)
            printf(".");
    }
    printf("  ]]\n");
}

/**
 * Main entrance!
 */
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "please give me a dest host nameeeeeee\n");
        exit(1);
    }

#ifdef _WIN32
    // Windows requires Winsock to be initialized before any socket calls
    WSADATA wsa_data;
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_result != 0)
    {
        fprintf(stderr, "WSAStartup failed: %d\n", wsa_result);
        exit(1);
    }
#endif

    const int DNS_PORT_NUM = 53;
    const char *destination_url = argv[1];
    printf("Searching the IP for: %s\n", destination_url);

    // Create UDP socket
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == INVALID_SOCKET)
    {
        fprintf(stderr, "failed to initialize socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

#define MTU 1500
    const uint8_t RETRY_LIMIT = 5;
    const uint16_t OUR_CHOSEN_ID = 100;
    const uint16_t QTYPE_HOST_ADDRESS_VALUE = 1;
    const uint16_t QCLASS_INTERNET_VALUE = 1;

    // Header part
    dns_header_t query_header = {
        .id = htons(OUR_CHOSEN_ID),
        .rd = 1,
        .tc = 0,
        .aa = 0,
        .opcode = 0,
        .qr = 0,
        .qdcount = htons(1),
        .ancount = 0,
        .nscount = 0,
        .arcount = 0,
    };

    uint8_t qname[256];
    int qname_length = encode_dns_qname(qname, destination_url);

    uint16_t qtype = htons(QTYPE_HOST_ADDRESS_VALUE);
    uint16_t qclass = htons(QCLASS_INTERNET_VALUE);

    // Build the query packet
    uint8_t query[512];
    uint8_t *query_ptr = query;

    memcpy(query_ptr, &query_header, sizeof(dns_header_t));
    query_ptr += sizeof(dns_header_t);

    memcpy(query_ptr, &qname, qname_length);
    query_ptr += qname_length;

    memcpy(query_ptr, &qtype, sizeof(qtype));
    query_ptr += sizeof(qtype);

    memcpy(query_ptr, &qclass, sizeof(qclass));
    query_ptr += sizeof(qclass);

    // Server address (Google DNS 8.8.8.8)
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DNS_PORT_NUM);
    server_addr.sin_addr.s_addr = inet_addr("8.8.8.8");
    socklen_t server_addr_len = sizeof(server_addr);

    size_t query_size = query_ptr - query;

    // Send DNS query
    ssize_t n = sendto(udp_socket, (const char *)query, (int)query_size, 0,
                       (const struct sockaddr *)&server_addr, server_addr_len);
    if (n == SOCKET_ERROR)
    {
        fprintf(stderr, "Query failed\n");
        CLOSE_SOCKET(udp_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }
    printf("Query succeeded!\n");

    // Receive response
    uint8_t recv_buffer[MTU];
    uint8_t received_message = 0;
    ssize_t recv_length;
    uint8_t retry_count = 0;
    uint8_t *recv_buffer_ptr;
    dns_header_t recv_header;

    while (!received_message && retry_count < RETRY_LIMIT)
    {
        recv_length = recvfrom(udp_socket, (char *)recv_buffer, sizeof(recv_buffer), 0, NULL, NULL);
        if (recv_length == SOCKET_ERROR)
        {
            printf("Error: recvfrom error, retrying\n");
            sendto(udp_socket, (const char *)query, (int)query_size, 0,
                   (const struct sockaddr *)&server_addr, server_addr_len);
            retry_count++;
            continue;
        }

        recv_buffer_ptr = recv_buffer;
        memcpy(&recv_header, recv_buffer_ptr, sizeof(recv_header));
        recv_buffer_ptr += sizeof(recv_header);

        printf("(In big endian) Our query id: %d, received id: %d\n", query_header.id, recv_header.id);
        if (query_header.id != recv_header.id)
        {
            printf("Received unrelated response, skipping...\n");
            sendto(udp_socket, (const char *)query, (int)query_size, 0,
                   (const struct sockaddr *)&server_addr, server_addr_len);
            retry_count++;
            if (retry_count == RETRY_LIMIT)
                printf("Retry limit has been reached\n");
            continue;
        }

        received_message = 1;
    }

    if (!received_message)
    {
        fprintf(stderr, "failed to capture dns query response\n");
        CLOSE_SOCKET(udp_socket);
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

    // Print the question section we received back
    printf("\n----Successfully received response!----\n");
    printf("We asked for:  ");
    int recv_qname_len = decode_dns_qname(recv_buffer_ptr);
    recv_buffer_ptr += recv_qname_len;
    recv_buffer_ptr += (sizeof(qtype) + sizeof(qclass));

    // Parse the answer section
    uint16_t answer_type;
    uint16_t answer_class;
    uint32_t answer_ttl;
    uint16_t answer_rdlength;

    // Skip the name field (stop at 0x00 terminator or pointer)
    while (*recv_buffer_ptr != 0x00)
    {
        recv_buffer_ptr++;
    }
    recv_buffer_ptr += (sizeof(answer_type) + sizeof(answer_class) + sizeof(answer_ttl));

    memcpy(&answer_rdlength, recv_buffer_ptr, sizeof(answer_rdlength));
    answer_rdlength = ntohs(answer_rdlength); // use ntohs for network->host conversion
    recv_buffer_ptr += sizeof(answer_rdlength);

    printf("RDLENGTH = %d\n", answer_rdlength);
    print_ipv4_address(recv_buffer_ptr, answer_rdlength);

    // Close socket
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
