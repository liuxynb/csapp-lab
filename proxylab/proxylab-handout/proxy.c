#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *http_version = "HTTP/1.0";
static const char *default_port = "80";
Cache *cache;

void doit(int fd);
void get_path(char uri[], char host[], char port[], char path[]);
void read_requesthdrs(rio_t *rp);
void *thread(void *vargp);

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr clientaddr;
    pthread_t tid;

    // 判断
    if (argc != 2)
    {
        fprintf(stderr, "you should enter a port number greater than 1,024 and less than 65,536!\n");
        exit(1);
    }

    int port_num = -1;
    port_num = atoi(argv[1]);
    if (port_num == -1)
    {
        fprintf(stderr, "you should enter a port number greater than 1,024 and less than 65,536!\n");
        exit(1);
    }

    cache = (Cache *)malloc(sizeof(Cache));
    signal(SIGPIPE, SIG_IGN);
    init_cache(cache, MAX_CACHE_SIZE, MAX_OBJECT_SIZE);
    listenfd = Open_listenfd(argv[1]);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        // 防止竞争
        connfdp = Malloc(sizeof(int));
        // 返回已连接描述符
        *connfdp = Accept(listenfd, &clientaddr, &clientlen);
        // 将套接字地址结构sa转换成对应的主机和服务名字符串，并将它们复制到host和servcice缓冲区；
        Getnameinfo(&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
    free_cache(cache);
    return 0;
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return;
}

void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char port[MAXLINE];
    char host[MAXLINE];
    char path[MAXLINE];
    rio_t rio;

    // 初始化
    Rio_readinitb(&rio, fd);
    // 无输入
    if (!Rio_readlineb(&rio, buf, MAXLINE))
        return;
    // GET http://www.cmu.edu/hub/index.html HTTP/1.1
    sscanf(buf, "%s %s %s", method, uri, version);
    // 忽略GET以外的方法
    if (strcasecmp(method, "GET"))
        return;
    else
        printf("Not implement!\n");
    if (reader(cache, uri, fd) == 1)
        printf("Read from cache!\n");

    // 获得path
    get_path(uri, host, port, path);
    printf("%s %s %s %s %s\n", method, host, port, path, version);

    // 连接服务器
    rio_t rio_output;
    int clientfd;
    clientfd = Open_clientfd(host, port);
    // 将缓冲区和描述符联系起来
    Rio_readinitb(&rio_output, clientfd);

    // 修改Header
    // GET /hub/index.html HTTP/1.0
    sprintf(buf, "%s %s %s\r\n", method, path, http_version);
    Rio_writen(clientfd, buf, strlen(buf));
    // other
    int n = 0;
    // Host: www.cmu.edu
    sprintf(buf, "Host: %s:%s\r\n", host, port);
    Rio_writen(clientfd, buf, strlen(buf));
    // User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3
    sprintf(buf, "%s\r\n", user_agent_hdr);
    Rio_writen(clientfd, buf, strlen(buf));
    // Connection: close
    sprintf(buf, "Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));
    // Proxy-Connection: close
    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(clientfd, buf, strlen(buf));

    // 返回给客户端
    // 不要使用scanf或rio_readlineb来读二进制文件，像scanf或rio_readlineb这样的函数是专门设计来读取文本文件的
    int size = 0;
    char data[MAX_OBJECT_SIZE];

    while ((n = Rio_readnb(&rio_output, buf, MAXLINE)))
    {

        if (errno == ECONNRESET)
            continue;
        if (size <= MAX_OBJECT_SIZE)
        {
            memcpy(data + size, buf, n);
            size += n;
        }
        // 不能使用Rio_writen(fd, buf, strlen(buf)), 因为不一定是字符串
        Rio_writen(fd, buf, n);
    }
    insert_cache(cache, uri, data);
    Close(clientfd);

    return;
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}

// 解析path
void get_path(char uri[], char host[], char port[], char path[])
{
    // 斜杠计数
    int slash_cnt = 0;
    // 冒号计数
    int colon_cnt = 0;
    // int flag = 0;
    int n = strlen(uri);
    int j = 0;
    int k = 0;
    int l = 0;

    for (int i = 0; i < n; i++)
    {
        if (uri[i] == '/')
        {
            slash_cnt++;
        }
        if (uri[i] == ':')
        {
            colon_cnt++;
            continue;
        }
        if ((colon_cnt == 2) && (slash_cnt < 3))
        {
            port[k++] = uri[i];
        }
        if ((colon_cnt < 2) && (slash_cnt == 2) && (uri[i] != '/'))
        {
            host[j++] = uri[i];
        }
        if (slash_cnt >= 3)
        {
            path[l++] = uri[i];
        }
    }

    host[j] = '\0';
    // 默认端口
    if (k == 0)
        strcpy(port, default_port);
    else
        port[k] = '\0';
    path[l] = '\0';

    return;
}