#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

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

    // 创建一个fd_set的集合，存放的是需要检测的文件描述符
    fd_set rdset, tmp; //rdset可以存放1024个文件描述符,tmp是临时变量，用于存储rdset的副本
    FD_ZERO(&rdset); //将rdset中的所有位都初始化为0，表示没有需要检测的文件描述符
    FD_SET(lfd, &rdset); //将lfd添加到rdset中
    int maxfd = lfd; //maxfd是最大的文件描述符，初始化为lfd，因为lfd是第一个文件描述符，所以maxfd初始化为lfd

    while(1) {

        tmp = rdset; //将rdset的副本赋值给tmp

        //调用select系统函数，让内核帮检测哪些文件描述符有数据
        //只能返回有数据变化的文件描述符的个数，不能返回具体是哪个文件描述符有数据
        int ret = select(maxfd + 1, &tmp, NULL, NULL, NULL); 
        if(ret == -1) {
            perror("select");
            exit(-1);
        } else if(ret == 0) {//如果ret==0,说明超时未检测到
            continue;
        } else if(ret > 0) { // 说明检测到了有文件描述符的对应的缓冲区的数据发生了改变
            if(FD_ISSET(lfd, &tmp)) { //lfd是监听的文件描述符，如果lfd在tmp中被置位，有新的客户端连接进来了
                //接受新的客户端连接
                struct sockaddr_in cliaddr;
                int len = sizeof(cliaddr);
                int cfd = accept(lfd, (struct sockaddr *)&cliaddr, &len);

                // 将新的文件描述符加入到集合中
                FD_SET(cfd, &rdset);

                // 更新最大的文件描述符
                maxfd = maxfd > cfd ? maxfd : cfd;
            }
            //遍历rdset中的所有文件描述符，检测哪些文件描述符有数据
            for(int i = lfd + 1; i <= maxfd; i++) {//socket() 创建 lfd（例如值为 3）,accept() 返回的客户端套接字 cfd 会分配下一个可用值（例如 4、5、6...）
                                                   //lfd 是监听套接字，不能用于 read()/write() 通信,所以需要从lfd + 1开始遍历
                if(FD_ISSET(i, &tmp)) {
                    // 说明这个文件描述符对应的客户端发来了数据，需要读取数据
                    char buf[1024] = {0};
                    int len = read(i, buf, sizeof(buf)); //len表示读取到的字节数
                    if(len == -1) {
                        perror("read");
                        exit(-1);
                    } else if(len == 0) {
                        printf("client closed...\n");
                        close(i); //关闭文件描述符
                        FD_CLR(i, &rdset);//将i从rdset中清除，表示不需要检测i是否有数据
                    } else if(len > 0) {
                        printf("read buf = %s\n", buf);
                        write(i, buf, strlen(buf) + 1);
                    }
                }
            }
        }
    }
    close(lfd);
    return 0;
}

