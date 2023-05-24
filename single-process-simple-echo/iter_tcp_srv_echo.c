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
// server 的验证码
int srv_veri_code;

// 用全局变量 sigint_flag 标记有无收到 sigint 信号
int sigint_flag = 0;

int main(int argc, char **argv)
{
    /*
    server 的初始化
    */

    // server ip地址（第一个参数）
    srv_ip_address = argv[1];
    // server 端口号（第二个参数）
    srv_port = atoi(argv[2]);
    // server 验证码（第三个参数）
    srv_veri_code = atoi(argv[3]);

    /*
    信号处理器的注册初始化
    */

    struct sigaction act;
    void handle_sigint(int sig);
    act.sa_handler = handle_sigint;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    /*
    套接字的创建
    */
    int listenfd= socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        perror("socket error");
        return -1;
    }
    /*
    绑定套接字地址
    */
    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(srv_port);
    inet_pton(AF_INET, srv_ip_address, &srvaddr.sin_addr.s_addr);
    if(bind(listenfd, (struct sockaddr *)&srvaddr, sizeof(srvaddr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    /*
    设定监听模式
    */
    if(listen(listenfd, 1) < 0)
    {
        perror("listen error");
        return -1;
    }

    /*
    server 必须利用 listen() 创建监听套接字后输出到 stdout
    必须输出 server 的信息 [srv] server[<ip_address>:<port>] is initializing!
    */
    printf("[srv] server[%s:%d] is initializing!\n", srv_ip_address, srv_port);


    /*
    请求受理循环
    */
    while (!sigint_flag)
    {
        /*
        将请求受理循环的判定条件设定为 sigint_flag，
        检查 accept() 返回值，
        若 errno 为 EINTR 则被信号中断，重新执行
        */
        int connfd = accept(listenfd, NULL, NULL);
        // 建议 accept() 后输出 client 的信息到 stdout 
        // 格式如 [srv] client[<ip_address>:<port>] is accepted!
        if(connfd < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            perror("accept error");
            return -1;
            /*
            输出 client 的连接信息到 stdout
            */
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            if(getpeername(connfd, (struct sockaddr *)&cliaddr, &cliaddr_len) < 0)
            {
                perror("getpeername error");
                return -1;
            }
            char cli_ip_address[16] = {0};
            inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cli_ip_address, sizeof(cli_ip_address));
            int cli_port = ntohs(cliaddr.sin_port);
            printf("[srv] client[%s:%d] is accepted!\n", cli_ip_address, cli_port);
        }
        
        /*
        server 的业务逻辑
        */
        srv_biz(connfd);
        /*
        server 必须在完成交互后释放connfd
        */
        close(connfd);
    }
    
}

void handle_sigint(int sig)
{
    sigint_flag = 1;
};

void srv_biz(int connfd)
{
    /*
    server 的业务逻辑
    */
    
    // 收发数据小循环
    while(1){
        // 接收自定义应用层协议的数据(一个字符数在120以内加上\n\0的字符串)，用read读取
        char recv_buf[128] = {0};
        int ret = read(connfd, recv_buf, sizeof(recv_buf));
        if(ret < 0)
        {
            perror("read error");
            return;
        }
        else if(ret == 0)
        {
            /*
            打印 连接到的client 的断开信息到 stdout
            */
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            if(getpeername(connfd, (struct sockaddr *)&cliaddr, &cliaddr_len) < 0)
            {
                perror("getpeername error");
                return -1;
            }
            char cli_ip_address[16] = {0};
            inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cli_ip_address, sizeof(cli_ip_address));
            int cli_port = ntohs(cliaddr.sin_port);
            printf("[srv] client[%s:%d] is disconnected!\n", cli_ip_address, cli_port);
            return;
        }
        else
        {
            /*
            server 必须解析来自客户端的请求报文，
            以读区字符串的形式提取消息内容，
            添加前缀信息[ECH_RQT]打印到 stdout(尾部包含\n)(格式："[ECH_RQT]<msg>")
            server 必须在原始数据前添加验证码(veri_code)回传
            格式为"(veri_code)<msg>"(尾部包含\n\0)
            */
            
            // 解析请求报文
            char *msg = recv_buf;
            // 打印请求报文
            printf("[ECH_RQT]%s\n", msg);
            // 添加验证码
            char send_buf[128] = {0};
            sprintf(send_buf, "(%d)%s", srv_veri_code, msg);
            // 发送数据
            ret = write(connfd, send_buf, strlen(send_buf));
            if(ret < 0)
            {
                perror("write error");
                return;
            }
            else if(ret == 0)
            {
                return;
            }
        }
    }
}