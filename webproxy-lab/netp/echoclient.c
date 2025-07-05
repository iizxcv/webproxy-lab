#include "csapp.h"

int main(int argc, char **argv){

    int clientfd; //클라이언트 정보가 저장될 fd의 no
    char *host, *port, buf[MAXLINE]; 
    rio_t rio; //open file table에 저장될 정보 구조체로 관리

    if (argc != 3){ // argument가 3가 아니면, 즉 input으로 전달 받는 값이 3개가 아니면 오류로 판단
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]); 
        exit(0);
    }
    host = argv[1]; //input으로 전달 받은 첫번째 인자를 host에 넣음 (아마 ip나 도메인정보일 듯)
    port = argv[2]; // input 두번째 인자 port정보 받나봄
    
    clientfd = Open_clientfd(host, port); // host port 정보로 내부 FD entry를 만들어서, fd_no를 변수에 저장 나중에 이 변수값으로 접근
    Rio_readinitb(&rio, clientfd);          // client_fd 입출력 rio_t 로 초기화

    /* 강 open file table 메타데이터를 구조체를 표현
    typedef struct rio_t
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
    */
   // rio == socket이 되는거임

    while (Fgets(buf, MAXLINE,stdin) != NULL){  //Fgets: buf에 MAXLINE 만큼 stdin(입력받는데이터)를 저장 - NULL 아닐때까지 반복.
        Rio_writen(clientfd, buf, strlen(buf)); // buf에 담긴 문자열을 clientfd로 연결된 서버 소켓에 전송(write)
        Rio_readlineb(&rio, buf, strlen(buf)); // 서버로부터 한줄의 응답을 읽어서 buf에 저장
        Fputs(buf, stdout);                     // buf를 stdout(출력)
    }
    Close(clientfd);                            // 소켓(fd) descriptor table에서 제거 (닫기)
    exit(0);                                    // 정상 종료

}