# 实验任务

## 实验流程

- client 连接 server 后进入命令行交互模式 
- client 从 stdin 读取用户输入信息并且发送给服务器
- server 将信息原封不动发还给 client
- client 收到服务端消息后将其显示出来(写入stdout)
- client 收到  `EXIT` 指令后退出
- 启动一个 client 进程后再启动一个 client 观察效果

## 实验服务器程序设计

- server 的程序必须可以从命令参数获取 `ip_address`, `port`, `veri_code`
- server 必须安装 `sigint` 信号处理器
    - 用全局变量 `sigint_flag` 标记有无收到 `sigint` 信号
    - 进程起始就应用 `sigaction()` 安装 `sigint` 信号处理器
    - 将请求受理循环的判定条件设定为 `sigint_flag`，检查 `accept()` 返回值，若 `errno` 为 `EINTR` 则被信号中断，重新执行
- server 必须安装`sigchld`信号处理器
    - 用于避免子进程先于主进程结束产生僵尸进程
    - 必须设置对应的结构体的sa_flags为0，若服务器进程在`accept()`挂起，则返回-1，`errno`为`EINTR`，重新执行
    - 必须在信号处理函数中输出提示信息`[srv](parent_id)[chd](child_id) Child has terminated!`
- server 必须利用 `listen()` 创建监听套接字后必须输出 server 的信息 `[srv](pid)[srv_sa](<ip_address>:<port>)[vcd](vcd) has initialized!`
- 建议 `accept()` 后输出 client 的信息到 `stdout`，格式如`[src](pid)[cli_sa](ip_address:port) Client is accepted!`
- server 在 `accept()` 成功后必须利用`fork()`创建子进程，并用子进程处理客户端业务
- 子进程被创建后向`stdout`输出`[chd](pid)[ppid](ppid) Child process is created!`
- 建议通过业务函数`srz_biz()`来做读写处理
- server 必须在子进程中第一时间关闭监听套接字`listenfd`
- server 子进程必须接收请求报文，识别PDU头部的(ClientID ，CID)，并以读取字符串的方式提取消息数据(末尾包含`\n\0`)，以格式`[chd](pid)[cid](cid)[ECH_RQT] <msg>`打印出来
- 必须将原始数据镜像回送给客户端
- server 子进程必须在完成交互后释放connfd，清理资源
- server 子进程在`read()`时，若返回值为0，则说明客户端已经关闭连接，此时应该输出已断开连接的客户端信息`[chd](pid)[ppid](ppid)[cli_sa](ip_address:port) Client is closed!`
- server 子进程在`close(connfd)`后，`return()`退出前。输出提示信息`[chd](pid)[ppid](ppid) Child process is to return!`

## 实验客户端程序设计

- client 的程序必须可以从命令参数获取 `ip_address`, `port`和`cid`
- client 必须利用 `connect()` 连接成功后输出相关信息到 `stdout`
    - 必须输出 server 的信息 `[cli](pid)[srv_sa](<ip_address>:<port>) Server is connected!`
- 建议通过业务函数`cli_biz()`来做相关处理
    - client 必须接收用户输入的字符(尾部包含`\n\0`)，并以格式`[cli](pid)[cid](cid)[ECH_RQT] <msg>`输出到 `stdout`,并将以消息头为2字节的`CID`，消息体为原始数据的PDU发送给 server
    - client 必须接收数据回声，读取PDU头部的`VCD`，并以格式`[cli](pid)[vcd](vcd)[ECH_REP] <msg>`输出到 `stdout`
    - client 必须限制输入的字符数在 60 内
- client 在读取到`EXIT`指令时清理资源(包括关闭connfd)用return()退出(不能发送给server)
    - 关闭`connfd`后输出提示信息`[cli](pid) connfd is closed!`
    - 退出客户端前打印信息 `[cli](pid) Client is to return!`

## PDU 数据收发

- 所有的数据必须通过 `read() & write()` 完成读写操作

## 命名要求

- iter_tcp_srv_echo.c
- iter_tcp_cli_echo.c

server 和 client 命名如上，打包成 `iter_tcp_cs_echo.tar`