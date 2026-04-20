#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


// 포트 열고 대기
// 연결 요청 시 doit 함수 호출
int main(int argc, char **argv) {
    // 요청 변수 / 커넥션 변수 선언
    int listenfd, connfd;
    // http://hostname:port
    char hostname[MAXLINE], port[MAXLINE];
    // socklen_t 소켓 길이
    socklen_t clientlen;
    // 클라이언트 접속 경로
    struct sockaddr_storage clientaddr;

    // 잘못 입력 시
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // 서버 열고 listenfd에 대기. 포트는 argv 1로 받은 포트번호
    listenfd = Open_listenfd(argv[1]);
    while (1) { // 반복
        // clientaddr 정의
        clientlen = sizeof(clientaddr);
        // Accept로 connfd에 데이터 전송

        // 실제 연결 대기 (os가 채워줄 때 까지 대기함.)
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        // Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); //원본 코드
        // getnameinfo
        // clientaddr 확인해서 clientlen, hostname, port 채움
        if (getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0) != 0) {
            strcpy(hostname, "unknown");
            strcpy(port, "unknown");
        }
        // getnameinfo로 채운 hostname, port 반환
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

// 클라이언트 작업 요청 처리
// GET 요청 확인 후 URI 파싱, 파일 존재 확인
// static, dynamic 분기
void doit(int fd) {
    // 정적 페이지인지 체크하는 변순듯?
    int is_static;
    // 장치 id, inode, chmod 및 chown, 수정 시간 등 정보를 담는 구조체
    struct stat sbuf;
    // buf는 임시보관용 string
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    // 파일이름 및 경로 / dynamic일 경우, adder의 x값 y값 대입받는 변수
    char filename[MAXLINE], cgiargs[MAXLINE];
    // 입력값 바이트단위가 아닌 줄단위로 읽으려고 선언
    rio_t rio;

    // rio 초기화 및 fd값에 4할당
    Rio_readinitb(&rio, fd);
    
    // &rio의 값을 읽고 buf에 값을 할당함. MAXLINE만큼
    // 더 길면 자름. 에러코드 반환하는게 좋긴 함.
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("Request headers:\n");
    // buf에 GET / HTTP/1.1 입력돼있음.
    printf("%s", buf);

    // method, uri, version에 값 입력
    sscanf(buf, "%s %s %s", method, uri, version);

    // strcasecmp - 대소문자 구분 없이
    // method가 "get"이면 0 반환. 그 외의 경우엔 모두 true니까 실행
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Tiny dose not implement this method");
        return;
    }
    // 빈 줄 나올 때 까지 읽는 함수
    read_requesthdrs(&rio);

    // 여기에서 값 채워줌
    is_static = parse_uri(uri, filename, cgiargs);
    // stat(filename, &sbuf이 0이 아닐 때)
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    // is_static 값이 1일 때,
    if (is_static) {
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        // 
        serve_static(fd, filename, sbuf.st_size);
    }
    else {
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

// 에러 호출
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

// HTTP 요청을 \r\n 나올 때 까지 읽은 후 출력
void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

// URI 확인 후 static dynamic 판단
// 여기에서 보낼 경로 파악함
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;

    // cgi-bin이 아닐 때
    if (!strstr(uri, "cgi-bin")) {
        // 문자열 복사함수 strcpy
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        
        // 여기에서 home.html 채워줌
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else {
            strcpy(cgiargs, "");
        }
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

// 파일 mmap으로 메모리에 올려서 http 응답 헤더 + 파일 내용 전송
void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers :\n");
    printf("%s", buf);

    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

// 확장자 확인 후 Content-type 결정
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if(strstr(filename, "jpg"))
        strcpy(filetype, "image/jpg");
    else
        strcpy(filetype, "text/plain");
}

// 자식 프로세스 fork
// QUERY_STRING 환경 변수 설정
// stdout을 소켓으로 리다이렉트
// CGI 프로그램 execve, 부모는 wait
void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emptylist[] = { NULL };
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }

    Wait(NULL);
}