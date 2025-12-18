#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>

void recyleChild(int arg) {
    while(1) { //因为子进程可能有很多个，所以需要一直循环回收子进程
        int ret = waitpid(-1, NULL, WNOHANG); //waitpid() 是阻塞函数，如果子进程没有结束，会一直阻塞在这里,每次回收一个子进程
        if(ret == -1) {
            // 所有的子进程都回收了
            break;
        }else if(ret == 0) {
            // 还有子进程活着
            break;
        } else if(ret > 0){
            // 子进程被回收了
            printf("子进程 %d 被回收了\n", ret);
        }
    }
}

int main() {

    struct sigaction act; //定义信号捕捉结构体
    act.sa_flags = 0;//设置信号捕捉标志位
    sigemptyset(&act.sa_mask);//清空信号捕捉集
    act.sa_handler = recyleChild;//设置信号捕捉处理函数
    //注册信号捕捉，当子进程结束时，会调用recyleChild函数
    sigaction(SIGCHLD, &act, NULL);
    

    // 创建socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    if(lfd == -1){
        perror("socket");
        exit(-1);
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    int ret = bind(lfd,(struct sockaddr *)&saddr, sizeof(saddr));
    if(ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 监听
    ret = listen(lfd, 128);
    if(ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 不断循环等待客户端连接
    while(1) {

        struct sockaddr_in cliaddr; //客户端不需要初始化，因为accept会自动填充
        int len = sizeof(cliaddr);
        // 接受连接
        int cfd = accept(lfd, (struct sockaddr*)&cliaddr, &len);//accept是阻塞函数，如果客户端没有连接进来，会一直阻塞在这里
        if(cfd == -1) {
            if(errno == EINTR) { //避免因为信号中断导致accept返回-1，进而执行exit(-)导致程序退出。
                continue;
            }
            perror("accept"); 
            exit(-1);
        }

        // 每一个连接进来，创建一个子进程跟客户端通信
        pid_t pid = fork();
        if(pid == 0) {
            // 子进程

            // 获取所连接上的客户端的信息
            char cliIp[16];
            inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, cliIp, sizeof(cliIp)); //将网络字节序整数转换成点分十进制IP地址字符串
            unsigned short cliPort = ntohs(cliaddr.sin_port); //将网络字节序转换成主机字节序
            printf("client ip is : %s, port is %d\n", cliIp, cliPort);

            // 接收客户端发来的数据
            char recvBuf[1024];
            while(1) {
                int len = read(cfd, &recvBuf, sizeof(recvBuf)+1);//注意：read() 读取的字节数包括 '\0'，所以需要 +1 确保字符串以 '\0' 结尾

                if(len == -1) {
                    perror("read");
                    exit(-1);
                }else if(len > 0) {
                    printf("recv client : %s\n", recvBuf);
                } else if(len == 0) {
                    printf("client closed....\n");
                    break;
                }
                write(cfd, recvBuf, strlen(recvBuf) + 1);//注意：strlen(recvBuf) 返回字符串长度，不包括 '\0'，导致服务器端接收到的数据没有字符串结束符
                //所以需要在strlen(recvBuf) +1，确保字符串以 '\0' 结尾
            }
            close(cfd);
            exit(0);    // 退出当前子进程
        }

    }
    close(lfd);
    return 0;
}