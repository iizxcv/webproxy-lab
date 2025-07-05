#include "csapp.h"
void *svr_thread_func(void *vargp);
void echo(int connfd);

int main(int argc, char **argv)
{
    int listenfd; // 변수 선언 리슨fd , 연결 fd
    int *connfd;  // int 포안터형
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    // socklen_t clientlen; // 정수 변수
    // // struct sockaddr_storage clientaddr;                  /* Enough space for any address */
    // char client_hostname[MAXLINE], client_port[MAXLINE]; // argv로 받아오는 ip, port 정보

    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // 프린트하는 함수인데 stderr-2 에 printf함수안에 있는 정보를 전달?
        exit(0);                                        // 종료
    }

    listenfd = Open_listenfd(argv[1]); // 잘되면은 소켓이 생성되고 bind()함수도 동작해서 listenfd의 fd_no가 반환됨. 아니면은 소켓이 열리다가 close 함수로 닫힘
    while (1)
    {
        connfd = (int *)malloc(sizeof(int));
        printf("연결 전");
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // listenfd라는 별도의 fd를 만들어서 socketaddr 구조체로 만들고,
        printf("연결 후");
        pthread_create(&tid, NULL, svr_thread_func, connfd);
    }
}
void *svr_thread_func(void *vargp)
{
    int connfd = *((int *)vargp);

    free(vargp);
    Pthread_detach(pthread_self());

    socklen_t clientlen = sizeof(struct sockaddr_storage); // 128byte 크기의 구조체를 가진 sockaddr_storage 이녀석의 크기를 변수에 저장

    struct sockaddr_storage clientaddr;                  /* Enough space for any address */
    char client_hostname[MAXLINE], client_port[MAXLINE]; // argv로 받아오는 ip, port 정보

    getpeername(connfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, // 구조체 타입을 문자열 타입으로 변환 후 변수들에 저장.
                client_hostname, MAXLINE,
                client_port, MAXLINE, 0);

    printf("Connected to (%s, %s)\n", client_hostname, client_port);
    echo(connfd);
    Close(connfd);
    return NULL;
}
/*
# listen 단계
client   --- connect() --->  [ server: listenfd 대기 상태 ]

# accept 시점
client   <---> [ server: connfd 생성 후 read/write 시작 ]


socket() 함수가 소켓을 만들고, getaddrinfo()는 단지 주소 정보 제공자 역할하는 함수


getaddrinfo():
IP 주소, 포트 번호, 소켓 타입, 프로토콜 등을 포함한 addrinfo 구조체를 만들어 줌.
이 구조체는 이후 socket(), bind(), connect() 등에 활용됨.

socket():
커널 내부에서 새로운 소켓(파일 디스크립터)을 생성.
이 시점에서 진짜로 fd (파일 디스크립터) 가 생깁니다. 예: int sockfd = socket(...);

bind():
소켓을 특정 IP 주소 + 포트에 바인딩.
bind(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen);

listen():
서버 소켓을 “리스닝 상태”로 변경 (수동 모드로).
커널 내부 큐(backlog)를 만들어서 클라이언트 연결 요청을 대기시킴.



accept()
클라이언트의 연결 요청을 수락하고, 새로운 연결용 소켓(fd) 를 리턴합니다.

accept()를 호출하면:
연결 요청 큐에서 하나 꺼냄
커널이 내부적으로 새로운 연결 전용 소켓을 생성 (새 fd 할당)
이 fd(connfd)를 리턴
connfd는 서버와 클라이언트 사이의 실제 데이터 통신 전용 fd가 됨.
echo(connfd)는 read/write 혹은 Rio 버퍼 I/O로 데이터를 처리.
*/
