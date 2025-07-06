#include "csapp.h"

void echo(int connfd)
{
    size_t n;                  // 읽어들인 바이트 수
    char buf[MAXLINE];         // 입출력 버퍼
    rio_t rio;                 // Robust I/O용 버퍼 구조체

    printf("echo 함수 진입\n");

    // --- 클라이언트 주소 정보 출력 ---
    // connfd에서 연결된 클라이언트의 IP/포트 정보 확인
    struct sockaddr_storage peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    char host[NI_MAXHOST], port[NI_MAXSERV];

    if (getpeername(connfd, (struct sockaddr *)&peer_addr, &peer_len) == 0)
    {
        if (getnameinfo((struct sockaddr *)&peer_addr, peer_len,
                        host, sizeof(host), port, sizeof(port), 0) != 0)
        {
            return; // 클라이언트 정보 가져오기 실패 시 함수 종료
        }
    }
    // ----------------------------------

    // rio 버퍼 초기화: connfd를 기반으로 robust I/O 구조체 초기화
    Rio_readinitb(&rio, connfd);

    // 클라이언트로부터 한 줄씩 데이터를 읽고 그대로 다시 돌려줌
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {
        printf("Peer address: %s:%s\n", host, port);          // 클라이언트 주소 출력
        printf("server received %d bytes\n", (int)n);         // 수신 바이트 수 출력
        Rio_writen(connfd, buf, n);                           // 수신한 데이터 그대로 반환
    }
}


/*✅ 핵심 동작 요약
단계	설명
1️⃣	getpeername()으로 connfd의 연결 상대(클라이언트)의 IP/Port를 확인
2️⃣	Rio_readinitb()로 robust I/O 버퍼 초기화
3️⃣	while 루프 내에서 한 줄씩 Rio_readlineb()로 데이터 수신
4️⃣	수신 데이터를 그대로 Rio_writen()으로 클라이언트에게 전송 (echo)
5️⃣	연결 종료 시 루프 탈출 및 함수 종료

🔎 보충 설명
✔️ getpeername()
connfd를 통해 연결된 클라이언트의 주소 정보를 가져오는 시스템 콜

✔️ getnameinfo()
위에서 얻은 주소 구조체(sockaddr)를 사람이 읽을 수 있는 문자열(IP, 포트)로 변환

✔️ Rio_* 시리즈
CS:APP에서 제공하는 robust I/O 함수

내부 버퍼를 사용해 신호(interrupt), partial read/write 등을 처리하는 안정적인 입출력 방식
*/