#include "csapp.h"

void *svr_thread_func(void *vargp); // 스레드가 실행할 함수
void echo(int connfd);              // 클라이언트 요청을 처리하는 함수

int main(int argc, char **argv)
{
    int listenfd;     // 클라이언트 연결을 대기할 리슨 소켓
    int *connfd;      // 클라이언트 연결이 성립된 후 사용할 데이터 송수신용 소켓
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; // 클라이언트 주소 정보 저장
    pthread_t tid;       // 스레드 ID

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // 인자 부족 시 사용법 출력
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 소켓 생성 + 바인드 + listen 까지 완료
    while (1)
    {
        connfd = (int *)malloc(sizeof(int)); // 각 스레드가 사용할 connfd를 동적 할당
        clientlen = sizeof(struct sockaddr_storage);
        printf("연결 전\n");
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 요청 수락
        printf("연결 후\n");
        pthread_create(&tid, NULL, svr_thread_func, connfd); // 스레드 생성 후 연결 fd 전달
    }
}
void *svr_thread_func(void *vargp)
{
    int connfd = *((int *)vargp); // 전달받은 fd 복사
    free(vargp);                  // 동적 할당 해제
    Pthread_detach(pthread_self()); // 스레드 종료 시 자동 자원 회수

    // 클라이언트 주소 확인용 변수 선언
    socklen_t clientlen = sizeof(struct sockaddr_storage);
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];

    getpeername(connfd, (SA *)&clientaddr, &clientlen); // 연결된 소켓의 상대 주소 확인
    Getnameinfo((SA *)&clientaddr, clientlen,
                client_hostname, MAXLINE,
                client_port, MAXLINE, 0); // 문자열로 변환

    printf("Connected to (%s, %s)\n", client_hostname, client_port); // 접속 정보 출력
    echo(connfd);    // 데이터 송수신 처리
    Close(connfd);   // 소켓 닫기
    return NULL;
}
/*
✅ 핵심 동작 요약
단계	설명
1️⃣	listenfd를 만들어 서버는 연결 요청 대기 상태에 진입
2️⃣	클라이언트 연결 요청 시, accept()가 새로운 연결 fd(connfd) 생성
3️⃣	connfd는 새로운 스레드에 인자로 전달됨
4️⃣	스레드에서는 getpeername()과 Getnameinfo()로 클라이언트 정보 출력
5️⃣	echo(connfd)를 통해 통신 처리 후, Close(connfd)로 종료
6️⃣	pthread_detach()로 스레드는 종료 시 자원 자동 회수

✅ 주요 함수 설명
함수	역할
Open_listenfd()	서버 리슨 소켓 생성 및 리턴 (socket → bind → listen)
Accept()	클라이언트 연결 수락, 새 fd 반환
pthread_create()	클라이언트 처리용 스레드 생성
pthread_detach()	스레드 종료 후 자원 자동 반환 (join 불필요)
getpeername()	연결된 상대(클라이언트)의 주소 정보 조회
Getnameinfo()	클라이언트 주소를 문자열로 변환
echo()	클라이언트 요청 처리 루틴 (보낸 메시지를 그대로 다시 보냄)
*/