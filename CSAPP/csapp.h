// csapp.h - 선언만
#ifndef CSAPP_H

#define CSAPP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

int open_listenfd(char *port);
int open_clientfd(char *hostname, char *port);

// // 아이피 주소를 담는 32비트 공간 정의
// typedef struct IN_ADDR {
//     uint32_t s_addr;
// }in_addr;

// typedef struct SOCKADDR_IN {
//     uint16_t sin_famiiy;
//     uint16_t sin_port;
//     in_addr sin_addr;
//     unsigned char sin_zero[8];
// }sockaddr_in;

// typedef struct SOCKADDR{
//     uint16_t sa_family;
//     char sa_data[14];
// }sockaddr;

// typedef struct ADDRINFO {
//     int ai_flags;
//     int ai_family;
//     int ai_socktype;
//     int ai_protocol;
//     char *ai_canonname;
//     size_t ai_addrlen;
//     sockaddr *ai_addr;
//     struct ADDRINFO *ai_next;
// }addrinfo;


#endif