#include "csapp.h"

int open_listenf(char *port) {
    // 주소 담는 데이터들 선언
    struct addrinfo *p, *listp, hints;
    
    int listenfd, optval=1;

    // Get a list potential server addresses
    // 서버 주소 가져오기
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    


    return listenfd;
}