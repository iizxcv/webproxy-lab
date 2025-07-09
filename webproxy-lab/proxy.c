#include <stdio.h>
#include "csapp.h"

void doit(int fd);
void read_requesthdrs_proxy(rio_t *rp, char *head_buf);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void sigchld_handler(int sig);
void *svr_thread_func(void *vargp);
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  printf("%s", user_agent_hdr);

  signal(SIGCHLD, sigchld_handler);
  signal(SIGPIPE, SIG_IGN);
  pid_t pid;
  int listenfd;

  pthread_t tid;

  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]);
  while (1)
  {
    int *connfdp = (int *)malloc(sizeof(int));
    clientlen = sizeof(clientaddr);
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen); // line:netp:tiny:accept
    // Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
    //             0);
    // printf("Accepted connection from (%s, %s)\n", hostname, port);
    // if(pid=fork() == 0){
    // doit(connfd);  // line:netp:tiny:doit
    // Close(connfd); // line:netp:tiny:close
    // }
    pthread_create(&tid, NULL, svr_thread_func, connfdp); // 스레드 생성 후 연결 fd 전달
  }
  return 0;
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char head_buf[MAXLINE] = ""; // 헤더 echo용
  char filename[MAXLINE], cgiargs[MAXLINE];
  char proxy_make_header[MAXLINE] = "";
  char to_hostname[MAXLINE], to_port[MAXLINE];
  rio_t rio;

  // char videofile[MAXLINE] = "US.mp4";

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);           // fd를 rio에 매핑되게 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // fd = connfd 버퍼에 데이터가 있으면, buf 옮김
  // sprintf(head_buf, "%s", buf);      // head_buf에 buf 요청라인 저장 내용 저장.

  printf("Request headers:\n");
  printf("%s", buf); // GET http://localhost:7000/ HTTP/1.1

  sscanf(buf, "%s %s %s", method, uri, version); // 문자열 쪼개서 저장

  if (strcasecmp(method, "GET")) // 문자열 검사
  {
    clienterror(fd, method, "501", "Not implemented", // error문 출력
                "Tiny does not implement this method");
    // printf("fd_no: %d, buf-string:%s", fd, buf);
    return;
  }
  make_proxy_data(uri, &to_hostname, &to_port); // 안에서 uri, to_hostname, to_port 바꿔줌
  sprintf(head_buf, "%s %s %s\r\n", method, uri, version);
  read_requesthdrs_proxy(&rio, &head_buf); // 나머지 요청 헤더를 head_buf에 저장
  request_to_real_server(fd, &head_buf, to_hostname, to_port);
}

void read_requesthdrs_proxy(rio_t *rp, char *head_buf) // rp는 fd 정보를 가진 구조체 openfile table에 들어가는 형식?!?
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);

    if (strstr(buf, "User-Agent:"))
    {
      // memset(buf, 0,sizeof(buf)); //  memset(buf, 0,strlen(buf));
      // sprintf(head_buf + strlen(head_buf), "User-Agent: %s\r\n", user_agent_hdr);
      // sprintf(head_buf + strlen(head_buf), "%s", user_agent_hdr);
      strcat(head_buf, user_agent_hdr);
      // printf("===========%s==========\n",head_buf);
      //  strcat(head_buf,user_agent_hdr);
      // printf("===========\n%s==========\n",head_buf);
    }

    //   else if (!strstr(buf, "Host:")) {
    //   sprintf(head_buf + strlen(head_buf), "Host: %s:%s\r\n", to_hostname, to_port);
    // }

    else if (strstr(buf, "Content-type:"))
    {
      strcat(buf, "Connection: Keep-Alive\r\n");
    }
    else if (strstr(buf, "Connection:"))
    {
      continue;
    }
    else if (strstr(buf, "Proxy-Connection:"))
    {
      continue;
    }
    else
    {
      strcat(head_buf, buf); // heaf_buf에 rp 값 한줄 씩 추가
    }
  }
  sprintf(head_buf + strlen(head_buf), "Connection: close\r\n");
  sprintf(head_buf + strlen(head_buf), "Proxy-Connection: close\r\n");
  strcat(head_buf, "\r\n");

  printf("%s", head_buf);
  return;
}
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
/* Non-blocking operation 자식 프로세서 종료 시 시그널 받아서 */
void sigchld_handler(int sig)
{
  pid_t f_pid;
  while (f_pid = waitpid(-1, NULL, WNOHANG) > 0)
    ;
  /* 파라메터 설명
     -1 : PID 가 오게되며 -1의 의미는 먼저 들어온 프로세서의 pid를 뜻함
     성공하면 PID를 반환하고 죽은 자식의 상태를 STAT_LOC에 저장 - 지금은 NULL이니 상태가 어떻든 저장되는 값 없음.
    optional 자리에 WNOHANG로 설정되어 있을 때 , 자식이 죽지 않았다면 return 0;
 */
}

void make_proxy_data(char *uri, char *to_hostname, char *to_port)
{ // GET http://localhost:7000/ HTTP/1.1
  char protocol[MAXLINE], path[MAXLINE];
  sscanf(uri, "%[^:]://%[^:]:%[^/]%s", protocol, to_hostname, to_port, path);
  // sprintf(uri, "/%s",path);
  if (strlen(path) == 0)
    strcpy(uri, "/");
  else
    sprintf(uri, "/%s", path);
  // printf("=================================\n");
  // printf("%s:::%s:::%s \n",to_hostname,to_port,uri); localhost:::7000:::/
  // printf("=================================\n");
}

void request_to_real_server(int proxy_fd, char *proxy_buf, char *to_hostname, char *to_port)
{
  int clientfd; // 서버에 연결할 소켓 디스크립터
  // char *host, *port,  // 서버 주소와 포트, 입출력 버퍼
  char buf[MAXLINE];
  rio_t rio; // Robust I/O를 위한 버퍼 구조체
  printf("host:: %s, port:: %s", to_hostname, to_port);
  clientfd = Open_clientfd(to_hostname, to_port); // 서버에 연결 요청, 성공 시 소켓 fd 반환
  Rio_readinitb(&rio, clientfd);                  // rio 버퍼 초기화 (clientfd를 기반으로 robust I/O 사용)

  Rio_writen(clientfd, proxy_buf, strlen(proxy_buf)); // 사용자 입력을 서버로 전송

  // Rio_readlineb(&rio, buf, MAXLINE); // 서버 응답 한 줄을 읽어옴.
  // printf("\n==%s==\n", buf);             // 응답을 콘솔에 출력
  // Rio_readlineb(&rio, buf, MAXLINE); // 서버 응답 한 줄을 읽어옴.
  // printf("==%s==\n", buf);             // 응답을 콘솔에 출력
  int content_length = -1;

  while (Rio_readlineb(&rio, buf, MAXLINE) > 0)
  {
    Rio_writen(proxy_fd, buf, strlen(buf));
    printf("%s", buf);

    if (strstr(buf, "Content-length"))
    {
      char *p = strchr(buf,':');
      content_length = atoi(p+1);
    }
    if(strcmp(buf,"\r\n") == 0)
      break;
  }
    // printf("%d", content_length);

    int remaining = content_length;
    while (remaining > 0) {

      int chunksize = remaining < MAXLINE ? remaining : MAXLINE;
      int n = Rio_readnb(&rio, buf,chunksize);
      printf("===remaining: %d ======\n", remaining);
      Rio_writen(proxy_fd,buf,n);
      remaining =remaining- n;

      if (remaining <= 0) break;

}

  Close(clientfd); // 소켓 닫기 (커널 리소스 해제)
  // exit(0);
}

void *svr_thread_func(void *vargp)
{
  int connfd = *((int *)vargp); // 전달받은 fd 복사
  char hostname[MAXLINE], port[MAXLINE];

  free(vargp);                    // 동적 할당 해제
  Pthread_detach(pthread_self()); // 스레드 종료 시 자동 자원 회수

  // socklen_t clientlen = sizeof(struct sockaddr_storage);
  socklen_t clientlen = 0;
  struct sockaddr_storage clientaddr;

  getpeername(connfd, (SA *)&clientaddr, &clientlen); // 연결된 소켓의 상대 주소 확인
  getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
              0);
  printf("Accepted connection from (%s, %s)\n", hostname, port);
  doit(connfd);
  close(connfd);
}