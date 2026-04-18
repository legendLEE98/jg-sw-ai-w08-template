#include "csapp.h"

// 요청할 ip, 매개변수 받음
int main (int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    // 인수가 3개가 아닐 경우
    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    // 요청할 IP = argv[1]
    // 요청할 포트 = argv[2]
    host = argv[1];
    port = argv[2];

    // 클라이언트가 접속할 때 포트는 알아서 정해줌
    clientfd = Open_clientfd(host, port);
    // &rio -> rid_fd = clientfd 같은 형태로 인식
    Rio_readinitb(&rio, clientfd);

    // 저장할 버퍼, 최대 크기, 어디서 읽을지
    // 읽을거 없다면 NULL 반환. null이 아닐 때 true임
    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}