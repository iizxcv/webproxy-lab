#include "csapp.h"

void echo(int connfd)
{
    size_t n;          // 비부호형 정수
    char buf[MAXLINE]; // 버퍼
    rio_t rio;         // 입출력 fd
    
    printf("echo함수안에 들어감\n");
    
    // --- 추가: 클라이언트 주소 정보 출력 ---
    // connfd 정보만을 가지고 역으로  open file에 저장된 구조체 (클라이언트 정보)를 가져올 수 있음. 
    struct sockaddr_storage peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    char host[NI_MAXHOST], port[NI_MAXSERV];
    if (getpeername(connfd, (struct sockaddr *)&peer_addr, &peer_len) == 0)
    {
        if (getnameinfo((struct sockaddr *)&peer_addr, peer_len,
                        host, sizeof(host), port, sizeof(port), 0) == 0);
        else{
            return ;}
    }
    // --------------------------------------

    Rio_readinitb(&rio, connfd); // connfd 라는 fd를  rio(open file table)구조체에 매핑
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    { // rio에 있는 걸 buf에 저장
        printf("Peer address: %s:%s\n", host, port);
        printf("server received %d bytes \n", (int)n);
        Rio_writen(connfd, buf, n); // 버퍼에서 fd로 쓰기 - 받은 그대로 다시 전송?!?
    }
}