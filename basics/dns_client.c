#include "dns.h"
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
int encode_dns_name(uint8_t *buffer, const char *hostname) {
    int i, buffer_i = 0;
    char name_copy[256]; // Is 256 long enough: apparently yes, max is 253 characters according to Google search
    
    strncpy(name_copy, hostname, 255);
    
    // Get the substring that ends before the next '.'
    char *token = strtok(name_copy, ".");
    while (token != NULL) {
        size_t len = strlen(token);
        
        // Length octect - how long is the upcoming substring
        buffer[buffer_i++] = (uint8_t)len;
        
        // The substring
        for (i = 0; i < len; i++) {
            buffer[buffer_i++] = token[i];
        }
        
        token = strtok(NULL, ".");
    }
    
    // Add termmiation, there's no padding needed according to the documentation 
    buffer[buffer_i] = '\0';

    return buffer_i + 1;
}


int main(int argc, char *argv[]){

    if(argc != 2){
        perror("please give me a dest host nameeeeeee");
        exit(1);
    }

    const int DNS_PORT_NUM = 53;
    const char *destination_url = argv[1];
    printf("Searching the IP for: %s\n", destination_url);

    // Create udp socket, usually DNS query is small so we should just use udp
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket == -1){
        perror("failed to initialize socket\n");
        exit(1);
    }

    // Header part
    const uint16_t OUR_CHOSEN_ID = 100;
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


    // Question part - [ qname qtype qclass ]

    /* 
        QNAME structure
            a domain name represented as a sequence of labels, where
            each label consists of a length octet followed by that
            number of octets.  The domain name terminates with the
            zero length octet for the null label of the root.  Note
            that this field may be an odd number of octets; no
            padding is used.
    */
    uint8_t qname[256];
    int qname_length = encode_dns_name(qname, destination_url);

    const uint16_t QTYPE_HOST_ADDRESS_VALUE = 1;
    const uint16_t QCLASS_INTERNET_VALUE = 1;
    uint16_t qtype, qclass;
    // Be aware of the endianess!!!!
    qtype = htons(QTYPE_HOST_ADDRESS_VALUE);
    qclass = htons(QCLASS_INTERNET_VALUE);
    

    // Create the query
    uint8_t query[512];
    uint8_t *query_ptr = query;

    // Concat header
    memcpy(query_ptr, &query_header, sizeof(dns_header_t));
    query_ptr += sizeof(dns_header_t);

    // Concat question
    memcpy(query_ptr, &qname, qname_length);
    query_ptr += qname_length;

    memcpy(query_ptr, &qtype, sizeof(qtype));
    query_ptr += sizeof(qtype);

    memcpy(query_ptr, &qclass, sizeof(qclass));
    query_ptr += sizeof(qclass);

    // Server information
    const sa_family_t SERVER_SIN_FAMILY = AF_INET;
    const in_port_t SERVER_SIN_PORT = htons(DNS_PORT_NUM);
    const struct in_addr SERVER_SIN_ADDRESS = {
        .s_addr = inet_addr("8.8.8.8")
    };
    const struct sockaddr_in SERVER_SOCKET_ADDRESS = {
        .sin_family = SERVER_SIN_FAMILY,
        .sin_port = SERVER_SIN_PORT,
        .sin_addr = SERVER_SIN_ADDRESS
    };
    const socklen_t SERVER_SOCKET_ADDR_LEN = sizeof(SERVER_SOCKET_ADDRESS);
    size_t query_size = query_ptr - query;

    // Send DNS query!
    ssize_t n = sendto(udp_socket, query, query_size, 0,(const struct sockaddr *) &SERVER_SOCKET_ADDRESS, (socklen_t)SERVER_SOCKET_ADDR_LEN);

    if(n < 0){
        printf("Query failed\n");
    }else{
        printf("Query successed!\n");
    }


    // Close socket
    if(close(udp_socket) == -1){
        perror("failed to close socket\n"); 
        exit(1);
    }

    return 0;
}