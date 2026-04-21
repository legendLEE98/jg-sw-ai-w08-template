#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp, char *body);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *version, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void echo_static(int fd, char *body);

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
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], bbuf[MAXLINE];
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

    // body_buf에 초기값 선언
    strcpy(bbuf, buf);
    // method, uri, version에 값 입력
    sscanf(buf, "%s %s %s", method, uri, version);

    // strcasecmp - 대소문자 구분 없이
    // method가 "get"이면 0 반환. 그 외의 경우엔 모두 true니까 실행
    if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {
        clienterror(fd, method, "501", "Not implemented", "Tiny dose not implement this method");
        return;
    }
    // 빈 줄 나올 때 까지 읽는 함수
    // 여기에서 bodybuf를 보내서 해당 변수에 바디값 넣어두려고 함.
    read_requesthdrs(&rio, bbuf);

    // 여기에서 값 채워줌
    is_static = parse_uri(uri, filename, cgiargs);

    if (strcasecmp(uri, "/echo") == 0) {
        echo_static(fd, bbuf);
        return;
    }
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
        // version 추가로 전달해서 버전에 맞게끔 추가
        serve_static(fd, filename, sbuf.st_size, version, method);
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
void read_requesthdrs(rio_t *rp, char *body) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {
        strcat(body, buf);
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

// URI 확인 후 static dynamic 판단
// 여기에서 보낼 경로 파악함
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;

    // uri가 cgi-bin이 포함되지 않았을 때
    if (!strstr(uri, "cgi-bin")) {
        // 문자열 복사함수 strcpy
        // cgiargs == 쓰레기값 상태
        strcpy(cgiargs, "");
        // filename을 .으로 만들기
        strcpy(filename, ".");

        // uri 데이터 추가
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
void serve_static(int fd, char *filename, int filesize, char *version, char *method) {
    if (filesize == 0) {
        Rio_writen(fd, "HTTP/1.0 200 OK\r\nContent-length: 0\r\n\r\n", 38);
        return;
    }
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];

    get_filetype(filename, filetype);
    sprintf(buf, "%s 200 OK\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); 
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers :\n");
    printf("%s", buf);

    // 여기에서 buf값만 반환하면 HEAD 반환. 여기에서 끝내면 된다
    if (!(strcasecmp(method, "HEAD"))) {
        return;
    }


    srcfd = Open(filename, O_RDONLY, 0);
    // 아래는 기존 코드
    // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    // Close(srcfd);
    // Rio_writen(fd, srcp, filesize);
    // Munmap(srcp, filesize);

    // TODO
    // mmap대신 malloc 사용
    srcp = malloc(filesize);
    Rio_readn(srcfd, srcp, filesize);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    free(srcp);
}

// 확장자 확인 후 Content-type 결정
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if(strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if(strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpg");
    else if(strstr(filename, ".mpg") || strstr(filename, ".mpeg"))
        strcpy(filetype, "video/mpeg");
    else if(strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
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
    // cgiargs = 1&2&3 ~ 같은 식의 주소값 
    
    // if문으로 Fork를 했을 때, 부모는 0 이외의 값을
    // 자식은 0을 할당받음.
    // 즉, 부모와 자식이 분리돼서 작업이 시작됨. 부모는 wait, 자식은 내부 함수 실행
    if (Fork() == 0) {
        // query_string은 환경변수 이름. ? 뒤에 있는 요청 내용이며, OS 표준
        // 매개변수 1 -> 설정할 환경변수 이름
        // 매개변수 2 -> 세팅할 환경변수 밸류
        // 매개변수 3 -> 덮어씌우는 유무. 1이 true
        setenv("QUERY_STRING", cgiargs, 1);
        // STDOUT_FILENO가 가리키는 방향을 socket connfd로 변경
        // os가 실행할 요청을 4번으로 가리키게 함
        Dup2(fd, STDOUT_FILENO);

        // 현재 프로세스를 실행할 파일로 대체함
        // fd랑 환경변수(env)는 아래 작업 뒤에도 유지됨
        Execve(filename, emptylist, environ);
    }
    // 이 wait코드가 있어서 부모는 그 사이에 아무 작업도 못함
    Wait(NULL);
}

void echo_static(int fd, char *body) {
    char content[MAXLINE];
    char hdr[MAXLINE];

    sprintf(content, "<html><title>echo</title><pre>%s</pre></html>", body);

    // 이렇게 해도 되나 비권장 방식이라고 함. 버퍼 오버플로우 공격 가능성 있음
    // strcat 혹은 줄마다 rio_writen 쓰는게 좋음
    // 아니면 문자열을 미리 받아서. value += ~ 같은 형태로 미리 정의하면 좋을듯~
    sprintf(hdr, "HTTP/1.0 200 OK \r\n");
    sprintf(hdr, "%sContent-type: text/html\r\n", hdr);
    sprintf(hdr, "%sContent-Length: %d\r\n\r\n", hdr, (int)strlen(content));
    Rio_writen(fd, hdr, strlen(hdr));

    Rio_writen(fd, content, strlen(content));
}