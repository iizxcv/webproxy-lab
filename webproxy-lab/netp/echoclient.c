int main(int argc, char **argv)
{
    int clientfd;                    // 서버에 연결할 소켓 디스크립터
    char *host, *port, buf[MAXLINE]; // 서버 주소와 포트, 입출력 버퍼
    rio_t rio;                       // Robust I/O를 위한 버퍼 구조체

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]); // 사용법 안내
        exit(0);
    }

    host = argv[1]; // 첫 번째 인자: 서버 주소 (IP 또는 도메인)
    port = argv[2]; // 두 번째 인자: 포트 번호

    clientfd = Open_clientfd(host, port); // 서버에 연결 요청, 성공 시 소켓 fd 반환
    Rio_readinitb(&rio, clientfd);        // rio 버퍼 초기화 (clientfd를 기반으로 robust I/O 사용)

    // rio_t 구조체는 다음과 같은 정보들을 가짐:
    // int rio_fd;           // 연결된 소켓 fd
    // int rio_cnt;          // 버퍼에 남은 바이트 수
    // char *rio_bufptr;     // 현재 읽을 위치 포인터
    // char rio_buf[RIO_BUFSIZE]; // 내부 버퍼 공간

    while (Fgets(buf, MAXLINE, stdin) != NULL)
    /*
    사용자가 키보드로 입력한 한 줄을 buf에 저장
    이 함수는 입력이 들어올 때까지 블로킹(멈춤) 됨
*/
    {
        Rio_writen(clientfd, buf, strlen(buf)); // 사용자 입력을 서버로 전송
        //printf("아아 Rio_writen이 실행되었쥬ㅕㅕㅕㅕㅑㅑㅑㅑㅑ");
        Rio_readlineb(&rio, buf, MAXLINE);      // 서버 응답 한 줄을 읽어옴.
        // 별도의 timeout값이 없으면 서버가 보낼때까지 무한대기
       // printf("아아 Rio_readlinb 실행되었져ㅕㅕㅕㅕㅑㅑㅑㅑㅑ");
        Fputs(buf, stdout);                     // 응답을 콘솔에 출력
    }

    Close(clientfd); // 소켓 닫기 (커널 리소스 해제)
    exit(0);
}

/*



✅ 핵심 작동 흐름 요약
단계	설명
1️⃣	명령행 인자로 서버 주소(host)와 포트(port) 받음
2️⃣	Open_clientfd()로 TCP 연결 생성 (3-way handshake 수행)
3️⃣	rio_t 구조체를 이용해 robust buffered I/O를 설정
4️⃣	사용자 입력을 받아 서버에 전송하고, 응답을 출력
5️⃣	입력 종료 시 소켓 닫고 프로그램 종료


*/