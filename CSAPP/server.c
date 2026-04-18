#include "csapp.h"

void echo(int connfd);

// 실행시킬 때, 어떤 경로의 프로그램인지, 어떤 포트를 사용하는지 정의해야함.
int main(int argc, char**argv) {
    // listenfd, connfd 정의만
    int listenfd, connfd;
    // clientlen
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // ipv4 v6 요청을 둘 다 체크할 수 있게 넓게 잡는 공간
    char client_hostname[MAXLINE], client_port[MAXLINE]; // maxline은 8mb라고 함.

    // 맨 처음 입력받을 때, 경로 포트 둘 중 하나 비어있으면 반환.
    // 근데 경로를 다르게 할 순 없으니 포트 오류인 경우밖에 없음.
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // stderr?
        exit(0);
    }

    // 소켓, 바인드, 리슨 상태를 Open_listenfd에서 모두 정의함.
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        // sockaddr_storage 크기 정해서 clientlen에 대입
        clientlen = sizeof(struct sockaddr_storage);
        // Accept로 실제 소켓 생성. 
        // 여기에서 accept 선언하면 요청 오기 전 까지 슬립상태라고 함.
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // listenfd, *sockaddr, *addrlen
        // 값 전달. client_hostname, client_port
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);

        // 연결된 정보 안내
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        // echo함수 실행
        echo(connfd);

        Close(connfd);
    }
    exit(0);
}