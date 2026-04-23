#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

  
int doit(int fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse_uri(char *uri, char *host, char *port, char *path);
void read_requesthdrs(rio_t *rp, char *url);

// 일단 메인함수까진 비슷할 듯 함.
// 요청 들어오면 doit에서 현재 갖고 있는 데이터인지 판별 후 줘야 하는거니까
int main(int argc, char **argv)
{
    // 리슨 및 커넥트fd
    int listenfd, connfd;
    // 접속 호스트 및 포트
    char host[MAXLINE], port[MAXLINE];
    // 클라이언트 접속 요청 길이
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 3) {
		fprintf(stderr, "Please, input the port");
		exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
		clientlen = sizeof(clientaddr);
		
		connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		Getnameinfo((SA *) &clientaddr, clientlen, host, MAXLINE, port, MAXLINE, 0);

		printf("Accept connection from (%s, %s)\n", host, port);

		doit(connfd);
		close(connfd);
    }
}

int doit(int fd) {
	// 일단 캐시용으로 사용할 데이터 보관함
	char cachedata[MAX_CACHE_SIZE];
	// 목적지 주소
	char desthost[MAXLINE], destport[MAXLINE], destpath[MAXLINE];
	// id, inode, 크기 및 파일 정보등을 담는 변수
	struct stat sbuf;
	// 내용물 및 메소드 등 데이터들
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	// 파일 이름 / 요청 쿼리
	char filename[MAXLINE], cgiargs[MAXLINE];
	// 선언
	rio_t rio;

	// TODO	
	// 1. 클라이언트 요청 읽기
	//    Rio_readinitb + Rio_readlineb
	//    → method, uri, version 파싱

	// rio 초기화 및 유저 소켓 상태 할당 (connfd = 4)
	Rio_readinitb(&rio, fd);
	// buf에 fd로 가져온 rio데이터 추가
	Rio_readlineb(&rio, buf, MAXLINE);

	// 요청 들어오면 buf 반환 
	printf("Request headers: \n");
	printf("%s", buf);

	sscanf(buf, "%s %s %s", method, uri, version);

	// 2. URI에서 desthost, destport, path 추출
	//    parse_uri() 새로 작성 필요
	//    예) http://www.cmu.edu:8080/hub/index.html
	//        → host: www.cmu.edu
	//        → port: 8080 (없으면 "80")
	//        → path: /hub/index.html

	// 일단 GET만 받게 하기.
	if (strcasecmp(method, "GET") != 0) {
		clienterror(fd, method, "501", "Not implemented", "Proxy server dose not this method");
		return;
	}

	parse_uri(uri, desthost, destport, destpath);

	// 요청 하는 데이터 예시
	// 예) GET http://www.cmu.edu:8080/hub/index.html HTTP/1.1

	// 현재 갖고 있는 데이터
	// method, desthost port path, version

	// read_requesthdrs(&rio, uri); 나중에 처리

	// 3. 서버로 보낼 요청 헤더 조립
	//    GET /hub/index.html HTTP/1.0\r\n
	//    Host: www.cmu.edu\r\n
	//    User-Agent: ...\r\n
	//    Connection: close\r\n
	//    Proxy-Connection: close\r\n

	// 4. 서버에 연결
	//    open_clientfd(desthost, destport)

	// 5. 서버로 요청 전송
	//    Rio_writen

	// 6. 서버 응답 받아서 클라이언트로 전달
	//    Rio_readlineb 루프
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

// uri 받아서 host port path 자름
// 요청 예시 => https://www.naver.com/path경로
int parse_uri(char *uri, char *host, char *port, char *path) {
	char *ptr;

	// strstr로 // 위치부터 찾기
	ptr = strstr(uri, "//");
	strcpy(host, ptr + 2);

	ptr = strchr(host, ':');
	if (ptr) {
		*ptr = '\0';
		strcpy(port, ptr+1);

		ptr = strchr(port, '/');
		if (ptr) {
			strcpy(path, ptr);
			*ptr = '\0';
		}
	} else {
		strcpy(port, "80");
		ptr = strchr(host, '/');
		if (ptr) {
			strcpy(path, ptr);
			*ptr = '\0';
		}
	}
}

// HTTP 요청을 \r\n 나올 때 까지 읽은 후 출력
void read_requesthdrs(rio_t *rp, char *url) {

}