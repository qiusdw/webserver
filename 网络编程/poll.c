#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h> // poll系统调用


int main() {

    // 创建socket
    int lfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in saddr;
    saddr.sin_port = htons(9999);
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;

    // 绑定
    bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));

    // 监听
    listen(lfd, 8); //

    // 初始化检测的文件描述符数组
    struct pollfd fds[1024]; //可以自定义检测的文件描述符的个数，而select只能检测1024个文件描述符
    for(int i = 0; i < 1024; i++) {//初始化
        fds[i].fd = -1; //将fds[i].fd初始化为-1，表示文件描述符无效，表示可以被使用
        fds[i].events = POLLIN; //将fds[i].events初始化为POLLIN，表示需要检测的文件描述符的类型为读事件
    }
    fds[0].fd = lfd; //将用于监听的文件描述符加入到数组中
    int nfds = 0; //nfds是数组中有效文件描述符的最大索引，初始化为0

    while(1) {

        // 调用poll系统函数，让内核帮检测哪些文件描述符有数据
        int ret = poll(fds, nfds + 1, -1); //nfds + 1表示检测的文件描述符的个数，-1表示永久阻塞，直到有文件描述符有数据到达
        if(ret == -1) {
            perror("poll");
            exit(-1);
        } else if(ret == 0) {
            continue;
        } else if(ret > 0) { // 说明检测到了有文件描述符的对应的缓冲区的数据发生了改变
            
            if(fds[0].revents & POLLIN) {//fds[0]是存放用于监听的文件描述符的结构体
            //为什么使用 & 运算符而不是 == 运算符？
            //poll() 返回后，fds[i].revents 会被设置为实际发生的事件，可能同时包含多个标志（如 POLLIN | POLLERR）。
            //如果发生了多个事件，则使用 & 运算符可以检查是否发生了特定的事件，而 == 运算符只能检查是否发生了特定的单一事件。
            //如果 revents & POLLIN 非零，说明 POLLIN 被设置;如果为零，说明未设置。

                // 表示有新的客户端连接进来了
                struct sockaddr_in cliaddr;
                int len = sizeof(cliaddr);
                int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);

                // 将新的文件描述符加入到集合中
                int index = -1; //保存找到的索引位置
                for(int i = 1; i < 1024; i++) { //从1开始是因为0是用于监听的文件描述符,使用for循环找到一个最小的可以被使用的文件描述符的索引
                    if(fds[i].fd == -1) {
                        fds[i].fd = cfd;
                        fds[i].events = POLLIN;//将新的文件描述符的类型设置为POLLIN，表示需要检测的文件描述符的类型为读事件
                        index = i; //保存找到的索引位置
                        break;//找到一个最小的可以被使用的文件描述符的索引后，退出循环
                    }
                }

                // 更新数组中有效文件描述符的最大索引
                if(index != -1) {
                    nfds = nfds > index ? nfds : index;
                }
            }

            for(int i = 1; i <= nfds; i++) {
                if(fds[i].revents & POLLIN) {
                    // 说明这个文件描述符对应的客户端发来了数据
                    char buf[1024] = {0};
                    int len = read(fds[i].fd, buf, sizeof(buf));
                    if(len == -1) {
                        perror("read");
                        exit(-1);
                    } else if(len == 0) {
                        printf("client closed...\n");
                        close(fds[i].fd);
                        fds[i].fd = -1; //将文件描述符设置为-1，表示文件描述符无效，表示可以被使用
                    } else if(len > 0) {
                        printf("read buf = %s\n", buf);
                        write(fds[i].fd, buf, strlen(buf) + 1);
                    }
                }
            }

        }

    }
    close(lfd);
    return 0;
}

