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
int encode_dns_qname(uint8_t *buffer, const char *hostname) {
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

/**
 * Decodes a DNS qname formatted string to its original hostname.
 * Qname format: [len]label[len]label[0]
 * 
 * Returns the length of qname, including the 0 terminator
 */
int decode_dns_qname(uint8_t *data) {
    int i = 0;

    while (data[i] != 0) {
        uint8_t sub_str_len = data[i]; 
        i++;

        // Print the segment characters
        for (int j = 0; j < sub_str_len; j++) {
            putchar(data[i + j]);
        }

        i += sub_str_len;

        // Print a dot if there is another segment following
        if (data[i] != 0) {
            putchar('.');
        }
    }
    putchar('\n');

    return i+1;
}

/**
 * This is straightforward hehe
 */
void print_ipv4_address(uint8_t *ptr, uint16_t answer_rdlength){

    printf("IP address: [[  ");
    for(int i = 0; i<answer_rdlength; i++){
        char str_buf[4];
        sprintf(str_buf, "%d", *(ptr+i));
        printf("%s", str_buf);
        i != answer_rdlength-1 ? printf(".") : printf("");
    }
    printf("  ]]\n");
} 

/**
 * Main entrance!
 */
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


    #define MTU 1500 // MTU is usually 1500, btw for future me: if I use const int it gives "warning: variable length array folded to constant array as an extension "
    const uint8_t RETRY_LIMIT = 5;
    const uint16_t OUR_CHOSEN_ID = 100; // should be random and secure if it's a serious application, https://stackoverflow.com/a/39475626/2224584
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
    int qname_length = encode_dns_qname(qname, destination_url);

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
        exit(1);
    }else{
        printf("Query successed!\n");
    }

    // Receive response
    uint8_t recv_buffer[MTU];
    uint8_t received_message = 0;
    ssize_t recv_length;
    uint8_t retry_count = 0;
    uint8_t *recv_buffer_ptr;
    dns_header_t recv_header;

    while(!received_message && retry_count < RETRY_LIMIT){
        recv_length = recvfrom(udp_socket, recv_buffer, sizeof(recv_buffer), 0, NULL, NULL);
        if(recv_length < 0){
            printf("Error: recvfrom error, retrying");
            sendto(udp_socket, query, query_size, 0,(const struct sockaddr *) &SERVER_SOCKET_ADDRESS, (socklen_t)SERVER_SOCKET_ADDR_LEN);
            retry_count += 1;
            continue;
        }
        recv_buffer_ptr = recv_buffer;
        memcpy(&recv_header, recv_buffer_ptr, sizeof(recv_header));
        recv_buffer_ptr += sizeof(recv_header);

        // check if we are getting the actual response of our query
        printf("(In big endian) Our query id: %d, received id: %d\n", query_header.id, recv_header.id);
        if(query_header.id != recv_header.id){
            printf("Received unrelated repsonse, skipping this...\n");

            // resend query just to make sure we didn't miss it
            sendto(udp_socket, query, query_size, 0,(const struct sockaddr *) &SERVER_SOCKET_ADDRESS, (socklen_t)SERVER_SOCKET_ADDR_LEN);
            retry_count += 1;

            if(retry_count == 5){
                printf("Retry limit has been reached");
            }
            continue;
        }

        received_message = 1;
    }

    if(!received_message){
        perror("failed to capture dns query response");
        exit(1);
    }


    // Received Question part, print out what we asked
    printf("\n----Successfully received response!----\n");
    printf("We asked for:  ");
    int recv_qname_len = decode_dns_qname(recv_buffer_ptr);
    recv_buffer_ptr += recv_qname_len;
    recv_buffer_ptr += (sizeof(qtype) + sizeof(qclass)); // skip these, don't care about them ~ 


    // Received Answer part, print out the ip address
    uint16_t answer_type;
    uint16_t answer_class;
    uint32_t answer_ttl;
    uint16_t answer_rdlength;
    // skip name, type, class ans ttl
    while(*recv_buffer_ptr != 0x00){
        recv_buffer_ptr++;
    }
    recv_buffer_ptr += (sizeof(answer_type) + sizeof(answer_class) + sizeof(answer_ttl));

    // get rdlength and the ip address after it
    memcpy(&answer_rdlength, recv_buffer_ptr, sizeof(answer_rdlength));
    answer_rdlength = htons(answer_rdlength);
    recv_buffer_ptr += sizeof(answer_rdlength);
    printf("RDLENGTH = %d\n", answer_rdlength);
    print_ipv4_address(recv_buffer_ptr, answer_rdlength);

    // Close socket
    if(close(udp_socket) == -1){
        perror("failed to close socket\n"); 
        exit(1);
    }

    return 0;
}