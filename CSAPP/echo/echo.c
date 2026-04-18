#include "csapp.h"

// 클라이언트가 연결 끊을 때까지 데이터 받고 돌려주는 함수
void echo(int connfd) {
    size_t n;
    char buf[MAXLINE];  // 한 줄씩 꺼내서 작업하는 8KB 버퍼
    rio_t rio;          // 내부 버퍼를 가진 안전한 읽기 구조체

    // connfd와 rio 연결 (이후 읽기는 rio 통해서)
    Rio_readinitb(&rio, connfd);
    
    // 한 줄 읽어서 buf에 담고, 클라이언트가 끊으면 n==0으로 탈출
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);  // buf에 담긴 걸 그대로 돌려보냄
    }
}