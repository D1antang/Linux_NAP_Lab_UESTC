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
    /*
    对服务器发起业务请求
    */ 
    
}

void cli_biz(int connfd){
    // 业务数据小循环
    while (1)
    {
        
    }    
}