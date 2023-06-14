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
/*
运行范例:
./iter_tcp_srv_echo.o <ip_address> <port> <veri_code>
其中<veri_code>在100-999之间，用于验证客户端的合法性
*/
int sigint_flag = 0;

void handle_sigint(int sig){
    printf("[srv] SIGINT is coming!\n");
    sigint_flag = 1;
}

// 必须输出"[srv](parent_pid)[chd](child_pid) Child has terminated!"的格式
void handle_sigchld(int sig){
    pid_t pid_chld;
    int status;
    while ((pid_chld = waitpid(-1,&status,WNOHANG)) > 0)
    {
        printf("[srv](%d)[chd](%d) Child has terminated!\n",getpid(),pid_chld);
    }
}

// 子进程中的业务函数
void srv_biz(int connfd,int srv_veri_code){
    /*
    业务处理
    */

//  server 子进程必须接收请求报文，识别PDU头部的(ClientID,CID)，并以读取字符串的方式提取消息数据(末尾包含\n\0)，增加前缀信息[ECH_RQT]打印出来
//  客户端请求报文的PDU格式: 2字节的CID
    
    // 读取2字节的PDU header,如果read返回0,则说明客户端已经关闭了连接，打印信息
    // 格式如同`[chd](pid)[ppid](ppid)[cli_sa](ip_address:port) Client is closed!`
    char pduHeader[2] = {0};
    int read_size = read(connfd,pduHeader,sizeof(pduHeader));
    if(read_size == 0){
        struct sockaddr_in cliaddr;
        socklen_t cliaddr_len = sizeof(cliaddr);
        if(getpeername(connfd,(struct sockaddr*)&cliaddr,&cliaddr_len) < 0){
            perror("getpeername error");
            return;
        }
        printf("[chd](%d)[ppid](%d)[cli_sa](%s:%d) Client is closed!\n",getpid(),getppid(),inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
        return;
    }
    // 如果read返回-1,则说明出错，打印信息
    else if (read_size < 0)
    {
        perror("read error");
        return;
    }
    // 其他情况处理PDU
    else
    {
        // 先将pduHeader转换为cid
        unsigned short cid = ntohs(*(unsigned short*)pduHeader);
        // 读取字符串的方式提取消息数据(末尾包含`\n\0`)
        char buf[1024] = {0};
        read_size = read(connfd,buf,sizeof(buf));
        if(read_size < 0){
            perror("read error");
            return;
        }
        // 以读取字符串的方式提取消息数据(末尾包含`\n\0`)，以格式`[chd](pid)[cid](cid)[ECH_RQT] <msg>`打印出来
        printf("[chd](%d)[cid](%d)[ECH_RQT] %s",getpid(),cid,buf);
        //创建PDU，其中发送的PDU包含2字节的veri_code头部
        char send_buf[1024] = {0};
        *(unsigned short*)send_buf = htons(srv_veri_code);
        // 将原始数据以PDU形式传输
        memcpy(send_buf,buf,read_size);
        // 将PDU发送给客户端
        if(write(connfd,send_buf,read_size+2) < 0){
            perror("write error");
            return;
        }
    }
}

int main(int argc,char **argv){
    /*
    读取命令行参数
    */

    // 读取ip地址
    char* srv_ip_address = argv[1];
    // 读取端口号
    int srv_port = atoi(argv[2]);
    // 读取验证码（两字节大小）
    int srv_veri_code = atoi(argv[3]);

    /*
    安装信号处理器
    */

    // 安装SIGINT信号处理器
    struct sigaction act_int;
    act_int.sa_handler = handle_sigint;
    act_int.sa_flags = 0;
    sigemptyset(&act_int.sa_mask);
    sigaction(SIGINT,&act_int,NULL);

    // 安装SIGCHLD信号处理器
    struct sigaction act_chld;
    act_chld.sa_handler = handle_sigchld;
    act_chld.sa_flags = 0;
    sigemptyset(&act_chld.sa_mask);
    sigaction(SIGCHLD,&act_chld,NULL);


    // 安装SIGPIPE信号处理器
    struct sigaction act_pipe;
    act_pipe.sa_handler = SIG_IGN;
    act_pipe.sa_flags = 0;
    sigemptyset(&act_pipe.sa_mask);
    sigaction(SIGPIPE,&act_pipe,NULL);

    /*
    套接字的创建
    */
    int listenfd = socket(AF_INET,SOCK_STREAM,0);
    if(listenfd < 0){
        perror("socket error");
        return -1;
    }

    /*
    绑定套接字地址
    */
    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(srv_port);
    srvaddr.sin_addr.s_addr = inet_addr(srv_ip_address);
    if(bind(listenfd,(struct sockaddr*)&srvaddr,sizeof(srvaddr)) < 0){
        perror("bind error");
        return -1;
    }

    /*
    设定监听模式
    */
    if(listen(listenfd, 1) < 0){
        perror("listen error");
        return -1;
    }

    /*
    server 必须利用 listen() 创建监听套接字后输出到 stdout
    必须输出 server 的信息 [srv](pid)[srv_sa](<ip_address>:<port>)[vcd](vcd) has initialized!
    */
    printf("[srv](%d)[srv_sa](%s:%d)[vcd](%d) has initialized!\n", getpid(), srv_ip_address, srv_port, srv_veri_code);


    /*
    请求受理循环
    */
    while (!sigint_flag)
    {
        /*
        检查 accept 是否被信号中断
        */
        int connfd = accept(listenfd,NULL,NULL);
        if(connfd < 0){
            if(errno == EINTR){
                continue;
            }
            else{
                perror("accept error");
                return -1;
            }
        }
        // accept() 成功后输出 client 的套接字信息到 stdout，格式如[src](pid)[cli_sa](ip_address:port) Client is accepted!
        struct sockaddr_in cliaddr;
        socklen_t cliaddr_len = sizeof(cliaddr);
        if(getpeername(connfd,(struct sockaddr*)&cliaddr,&cliaddr_len) < 0){
            perror("getpeername error");
            return -1;
        }
        printf("[src](%d)[cli_sa](%s:%d) Client is accepted!\n",getpid(),inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
        // server 在 accept() 成功后必须利用fork()创建子进程，并用子进程处理客户端业务
        pid_t pid_chld = fork();
        if (pid_chld == 0){
            // server 必须在子进程中第一时间关闭监听套接字listenfd
            close(listenfd);
            // 子进程被创建后向stdout输出[chd](pid)[ppid](ppid) Child process is created!
            printf("[chd](%d)[ppid](%d) Child process is created!\n",getpid(),getppid());
            // 建议通过业务函数srz_biz()来做读写处理
            srv_biz(connfd,srv_veri_code);
            // server 子进程必须在完成交互后释放connfd，清理资源
            close(connfd);
            // server 子进程在close(connfd)后，return()退出前。输出提示信息[chd](pid)[ppid](ppid) Child process is to return!
            printf("[chd](%d)[ppid](%d) Child process is to return!\n",getpid(),getppid());
            // 用 return() 退出子进程
            return 0;
        }
    }   
    // 收到SIGINT信号后退出请求受理循环，清理资源用return退出程序
    return 0;
}
