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
- server 必须利用 `listen()` 创建监听套接字后输出到 `stdout`
    - 必须输出 server 的信息 `[srv] server[<ip_address>:<port>] is initializing!`
    - 建议 `accept()` 后输出 client 的信息到 `stdout`
    - 建议通过业务函数来做相关处理
- server 必须解析来自客户端的请求报文，并添加前缀信息`[ECH_RQT]`打印出来(尾部包含`\n`)
- server 必须在原始数据前添加验证码`(veri_code)`回传(尾部包含`\n\0`)
- server 必须在完成交互后释放connfd
- server 建议将断开的 client 信息输出到 `stdout`
- 用 `return()` 退出程序

## 实验客户端程序设计

- client 的程序必须可以从命令参数获取 `ip_address`, `port`
- client 必须利用 `connect()` 连接成功后输出相关信息到 `stdout`
    - 必须输出 server 的信息 `[cli] server[<ip_address>:<port>] is connected!`
    - 建议通过业务函数来做相关处理
- client 必须接收用户输入的字符，并添加前缀信息`[ECH_RQT]`发送(尾部包含`\n\0`)
- client 必须接收数据回声，并且以 `[ECH_REP](veri_code)<msg>` 的形式输出到 `stdout`
- client 必须限制输入的字符数在 120 内
- 用 `return()` 退出程序

## PDU 数据收发

- 所有的数据必须通过 `read() & write()` 完成读写操作

## 命名要求

- iter_tcp_srv_echo.c
- iter_tcp_cli_echo.c

server 和 client 命名如上，打包成 `iter_tcp_cs_echo.tar`