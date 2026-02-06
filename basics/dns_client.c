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
 */
void encode_dns_name(uint8_t *buffer, const char *hostname) {
    int i, buffer_i = 0;
    char name_copy[256]; // TODO: is 256 long enough
    
    strncpy(name_copy, hostname, 255);
    
    // Get the substring that ends before the next '.'
    char *token = strtok(name_copy, ".");
    while (token != NULL) {
        size_t len = strlen(token);
        
        // Length octect
        buffer[buffer_i++] = (uint8_t)len;
        
        // The substring
        for (i = 0; i < len; i++) {
            buffer[buffer_i++] = token[i];
        }
        
        token = strtok(NULL, ".");
    }
    
    // add termmiation, there's no padding needed according to the documentation 
    buffer[buffer_i] = '\0';
}


int main(int argc, char *argv[]){

    if(argc != 1){
        perror("please give me a dest host nameeeeeee");
        exit(1);
    }

    const char *destination_url = argv[0];
    printf("Searching the IP for: %s\n", destination_url);

    // Create udp socket, usually DNS query is small so we can just use udp
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(udp_socket == -1){
        perror("failed to initialize socket\n");
        exit(1);
    }

    // Header part
    const int OUR_CHOSEN_ID = 100;
    dns_header_t query_header = {
        .id = OUR_CHOSEN_ID,
        .rd = 1,
        .tc = 0,
        .aa = 0,
        .opcode = 0,
        .qr = 0,
        .qdcount = 1,
        .ancount = 0,
        .nscount = 0,
        .arcount = 0,
    };


    // Question part

    /* 
        QNAME structure
            a domain name represented as a sequence of labels, where
            each label consists of a length octet followed by that
            number of octets.  The domain name terminates with the
            zero length octet for the null label of the root.  Note
            that this field may be an odd number of octets; no
            padding is used.
    */
    // TODO: build QNAME
    const uint16_t QTYPE_HOST_ADDRESS_VALUE = 1;
    const uint16_t QCLASS_INTERNET_VALUE = 1;

    const uint16_t QTYPE = QTYPE_HOST_ADDRESS_VALUE;
    const uint16_t QCLASS = QCLASS_INTERNET_VALUE;
    
    // TODO: send the request to the DNS server


    if(close(udp_socket) == -1){
        perror("failed to close socket\n"); 
        exit(1);
    }

    return 0;
}