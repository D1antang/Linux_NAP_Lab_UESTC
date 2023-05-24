#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

// server 的ip地址
char *srv_ip_address;
// server 的端口号
int srv_port;

int main(int argc,char** argv){
    /*
    client 的初始化
    */

    // server ip地址（第一个参数）
    srv_ip_address = argv[1];
    // server 端口号（第二个参数）
    srv_port = atoi(argv[2]);

    // 创建套接字
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    if(connfd < 0){
        perror("socket error");
        return -1;
    }
    // 发起客户端连接请求
    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(srv_port);
    inet_pton(AF_INET, srv_ip_address, &srvaddr.sin_addr);
    int ret = connect(connfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
    if(ret < 0){
        perror("connect error");
        return -1;
    }
    // 打印 server 相关信息 [cli] server[<ip_address>:<port>] is connected!
    else{
        printf("[cli] server[%s:%d] is connected!\n", srv_ip_address, srv_port);
    }
    /*
    对服务器发起业务请求
    */ 
    void cli_biz(int connfd);
    cli_biz(connfd);
    /*
    用 return 打印相关信息，退出客户端进程
    */
    printf("[cli] client is to return!\n");
    return 0;
}

void cli_biz(int connfd){
    // 业务数据小循环
    while (1)
    {
        // 获取用户输入的一行字符 ，限制在120字符内
        char sendbuf[120];
        memset(sendbuf, 0, sizeof(sendbuf));
        fgets(sendbuf, sizeof(sendbuf), stdin);
        if(sendbuf[119] != '\0'){
            perror("input too long\n");
            return;
        }
        // 如果用户输入 EIXT\n\0 则退出
        if (strncmp(sendbuf, "EXIT\n", 5) == 0)
        {
            // 清理相应资源
            close(connfd);
            printf("[cli] connfd is closed!\n");
            return;
        }
        // 以"[ECH_RQT]<msg>"打印到 stdout
        printf("[ECH_RQT]%s", sendbuf);
        // 将用户输入的原始数据发送到服务器
        write(connfd, sendbuf, strlen(sendbuf));
        // 从服务器读回 echo 回显数据(包括verify code 和 echo data)
        char recvbuf[140];
        memset(recvbuf, 0, sizeof(recvbuf));
        read(connfd, recvbuf, sizeof(recvbuf));
        // 以"[ECH_REP]<msg>"打印到 stdout
        printf("[ECH_REP]%s", recvbuf);
    }   
}