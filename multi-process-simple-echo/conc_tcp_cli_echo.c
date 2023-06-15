#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

void cli_biz(int connfd, int cid)
{
    while (1)
    {
        // client 必须接收用户输入的字符(60以内）(尾部包含\n\0)，并以格式[cli](pid)[cid](cid)[ECH_RQT] <msg>输出到 stdout,并将以消息头为2字节的CID，消息体为原始数据的PDU发送给 server
        char buf[1024];
        fgets(buf, 1024, stdin);
        printf("[cli](%d)[ECH_RQT]%s", getpid(), buf);
        // 如果输入为EXIT则清理资源(包括关闭connfd)用return()退出(不能发送给server)
        if (strncmp(buf, "EXIT", 4) == 0)
        {
            close(connfd);
            // 关闭connfd后输出提示信息[cli](pid) connfd is closed!
            printf("[cli](%d) connfd is closed!\n", getpid());
            return;
        }
        // 创建一个 PDU
        int len = strlen(buf);
        char pduSend[1024];
        // 消息头为2字节的CID
        short cid_net = htons(cid);
        memcpy(pduSend, &cid_net, 2);
        // 消息体为原始数据
        memcpy(pduSend + 2, buf, len);
        // 利用write函数将PDU发送给server
        if (write(connfd, pduSend, len + 2) < 0)
        {
            perror("write error");
            return;
        }
        // 读取 server 回射的PDU头部的两字节VCD
        short vcd;
        read(connfd, &vcd, 2);
        // 读取 server 回射的PDU体
        char pduRecv[1024];
        read(connfd, pduRecv, 1024);
        // 将 server 回射的PDU体输出到 stdout
        // 格式为`[cli](pid)[vcd](vcd)[ECH_REP] <msg>`
        printf("[cli](%d)[vcd](%d)[ECH_REP]%s", getpid(), ntohs(vcd), pduRecv);
    }
}

int main(int argc, char **argv)
{
    // 先读取到 server 的 ip 地址,端口号和cid
    char *srv_ip_address = argv[1];
    int srv_port = atoi(argv[2]);
    int cid = atoi(argv[3]);
    // 创建套接字
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connfd < 0)
    {
        perror("socket error");
        return -1;
    }
    // 发起客户端连接请求
    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(srv_port);
    inet_pton(AF_INET, srv_ip_address, &srvaddr.sin_addr);
    int ret = connect(connfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if (ret < 0)
    {
        perror("connect error");
        return -1;
    }
    // 必须输出 server 的信息 [cli](pid)[srv_sa](<ip_address>:<port>) Server is connected!
    else
    {
        struct sockaddr_in srv_sa;
        socklen_t srv_sa_len = sizeof(srv_sa);
        getpeername(connfd, (struct sockaddr *)&srv_sa, &srv_sa_len);
        printf("[cli](%d)[srv_sa](%s:%d) Server is connected!\n", cid, inet_ntoa(srv_sa.sin_addr), ntohs(srv_sa.sin_port));
    }
    // 业务逻辑
    cli_biz(connfd, cid);
    // 输出退出客户端前打印信息 [cli](pid) Client is to return!
    printf("[cli](%d) Client is to return!\n", getpid());
    // 关闭客户端
    return 0;
}