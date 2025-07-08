/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp, char *head_buf);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

/* Non-blocking operation 자식 프로세서 종료 시 시그널 받아서 */
void sigchld_handler(int sig)
{
  pid_t f_pid;
  while (f_pid = waitpid(-1, NULL, WNOHANG) > 0);
     /* 파라메터 설명
        -1 : PID 가 오게되며 -1의 의미는 먼저 들어온 프로세서의 pid를 뜻함
        성공하면 PID를 반환하고 죽은 자식의 상태를 STAT_LOC에 저장 - 지금은 NULL이니 상태가 어떻든 저장되는 값 없음.
       optional 자리에 WNOHANG로 설정되어 있을 때 , 자식이 죽지 않았다면 return 0;
    */
}

int main(int argc, char **argv)
{
  signal(SIGCHLD, sigchld_handler);

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
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

    // signal(SIGCHLD, close);

    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen); // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);  // line:netp:tiny:doit
    Close(connfd); // line:netp:tiny:close
  }
}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char head_buf[MAXLINE] = ""; // 헤더 echo용
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  char videofile[MAXLINE] = "US.mp4";

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);           // fd를 rio에 매핑되게 초기화
  Rio_readlineb(&rio, buf, MAXLINE); // fd = connfd 버퍼에 데이터가 있으면, rio로 data 옮김
  sprintf(head_buf, "%s", buf);      // head_buf에 buf 요청라인 저장 내용 저장.

  printf("Request headers:\n");
  printf("%s", buf); // GET /godzilla.gif HTTP/1.1

  sscanf(buf, "%s %s %s", method, uri, version); // 문자열 쪼개서 저장

  if (strcasecmp(method, "GET")) // 문자열 검사
  {
    clienterror(fd, method, "501", "Not implemented", // error문 출력
                "Tiny does not implement this method");
    // printf("fd_no: %d, buf-string:%s", fd, buf);
    return;
  }

  read_requesthdrs(&rio, &head_buf); // 버퍼 한줄 출력 하고 라인 정리?

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs);
  if (stat(filename, &sbuf) < 0)
  {
    // printf("========================\n");
    // printf("DEBUG:: 파일 이름: %s", filename);
    // printf("========================\n");
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");

    return;
  }

  if (is_static)
  { /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }

    serve_static(fd, filename, sbuf.st_size);

    // stat(videofile, &vbuf);
    // serve_static_video(fd,videofile, vbuf.st_size);
    // serve_echo(fd, &head_buf); 11.6 ABCD완
  }
  else
  { /* Serve dynamic content */ // 나중에 GCI? 할때 수정할 부분
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

void read_requesthdrs(rio_t *rp, char *head_buf) // rp는 fd 정보를 가진 구조체 openfile table에 들어가는 형식?!?
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    strcat(head_buf, buf); // heaf_buf에 rp 값 한줄 씩 추가
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  { /* Static content */
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");
    return 1;
  }
  else
  { /* Dynamic content */
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
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

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);
  sprintf(buf, "HTTP/1.1 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));

  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // file 정보를 메모리에 저장.
  // printf("==========Fix That debug================\n");

  Close(srcfd);
  // Rio_writen(fd,head_buf,strlen(head_buf));
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

// void serve_static_video(int fd, char *filename, int filesize)
// {
//   int srcfd;
//   char *srcp, filetype[MAXLINE], buf[MAXBUF];

//   /* Send response headers to client */
//   get_filetype(filename, filetype);
//   sprintf(buf, "HTTP/1.0 200 OK\r\n");
//   sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
//   sprintf(buf, "%sConnection: close\r\n", buf);
//   sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
//   sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
//   Rio_writen(fd, buf, strlen(buf));

//   printf("Response headers:\n");
//   printf("%s", buf);

//   /* Send response body to client */
//   // char video_path[MAXLINE];
//   // sprintf(video_path, "./video/%s", filename); // 폴더 안에 있는 경로 가져오기
//   srcfd = Open(filename, O_RDONLY, 0); // 파일을 메모리에 올리고 그 디스크립터 idx 번호
//   srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // file 정보를 메모리에 저장.
//   // printf("==========Fix That debug================\n");

//   Close(srcfd);
//   // Rio_writen(fd,head_buf,strlen(head_buf));
//   Rio_writen(fd, srcp, filesize);
//   Munmap(srcp, filesize);
// }

void serve_echo(int fd, char *head_buf)
{
  char buf[MAXBUF];

  /* Send response headers to client */

  sprintf(buf, "HTTP/1.2 200 OK\r\n");
  sprintf(buf + strlen(buf), "Server: Tiny Web Server\r\n");
  sprintf(buf + strlen(buf), "Content-type: text/html\r\n\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // printf("Response headers:\n");
  // printf("%s", buf);

  sprintf(buf + strlen(buf), "<html><title>Echo response</title>");
  sprintf(buf + strlen(buf), "<body bgcolor=\"ffffff\">\r\n");
  // sprintf(buf + strlen(buf), "<body>");
  sprintf(buf + strlen(buf), "<p>%s</p>\r\n", head_buf);
  sprintf(buf + strlen(buf), "<hr><em>The Tiny Web server</em>\r\n</body></html>\r\n");

  Rio_writen(fd, buf, strlen(buf));
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO);              /* Redirect stdout to client */
    sleep(10);                            // 수면 걸어버리면 부모도 아무고토 못하는 상태.
    Execve(filename, emptylist, environ); /* Run CGI program */
  }
  // waitpid + sigchild 활용

  // Wait(NULL); /* Parent waits for and reaps child */
  /*자식들이 종료하는 것을 무지성으로 기다리지 않고,
  SIGCHLD 핸들러 선언 후 자식이 종료 되면 핸들러 함수에서 자식 프로세서 정상 종료 시
  할당된 리소스 수거를 위함.
  ps -ef | grep defunct 로 좀비 프로세서 확인 가능.


  예외 처리는 따로 해줘야함.

  한줄요약: 비동기 진행 시, 부모 프로세스가 종료된 자식 프로세스를 수거(reap)하여 좀비 프로세스가 생기지 않도록 처리하기 위함.
  
  */
}
