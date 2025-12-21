#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h> // epoll系统调用

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
    listen(lfd, 8);

    // 调用epoll_create()创建一个epoll实例
    int epfd = epoll_create(100); 

    // 将监听的文件描述符相关的检测信息添加到epoll实例中(实际是添加到内核的epoll红黑树rbr中)
    struct epoll_event epev;
    epev.events = EPOLLIN; //将epev.events设置为EPOLLIN，表示需要检测的文件描述符的类型为读事件
    epev.data.fd = lfd; //将epev.data.fd设置为lfd，lfd是用于监听的文件描述符
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);

    struct epoll_event epevs[1024]; //用于保存数据发生改变的文件描述符的结构体,可以自定义大小
    while(1) {

        // 调用epoll_wait()检测哪些文件描述符有数据
        int ret = epoll_wait(epfd, epevs, 1024, -1);
        if(ret == -1) {
            perror("epoll_wait");
            exit(-1);
        }

        printf("ret = %d\n", ret); //ret是返回的发生数据改变的文件描述符的个数

        for(int i = 0; i < ret; i++) { //遍历发生数据改变的文件描述符的结构体

            int curfd = epevs[i].data.fd; //curfd是发生数据改变的文件描述符

            if(curfd == lfd) { //如果是监听的文件描述符，则有客户端连接进来了
                struct sockaddr_in cliaddr;
                int len = sizeof(cliaddr);
                int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);

                epev.events = EPOLLIN; //将epev.events设置为EPOLLIN，表示需要检测的文件描述符的类型为读事件
                                    //如果epev.events不只是EPOLLIN，还有其他事件，则在else中需要分别处理其他事件 
                                    
                epev.data.fd = cfd; //将epev.data.fd设置为cfd，cfd是用于通信的文件描述符
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &epev);//将新的文件描述符(cfd：用于通信的文件描述符)添加到epoll实例中
            } else { //不是监听的文件描述符，而是用于通信的文件描述符
                if(epevs[i].events & EPOLLOUT) {//如果发生数据改变的文件描述符的类型为写事件，则跳过
                    continue;
                }   
                // 有数据到达，需要通信
                char buf[1024] = {0};
                int len = read(curfd, buf, sizeof(buf));
                if(len == -1) {
                    perror("read");
                    exit(-1);
                } else if(len == 0) {
                    printf("client closed...\n");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, NULL);//将文件描述符从epoll实例中删除
                    close(curfd);
                } else if(len > 0) {
                    printf("read buf = %s\n", buf);
                    write(curfd, buf, strlen(buf) + 1);
                }

            }

        }
    }

    close(lfd); //关闭用于监听的文件描述符
    close(epfd); //关闭epoll实例
    return 0;
}