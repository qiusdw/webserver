// TCP通信的客户端

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main() {

    // 1.创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        perror("socket");
        exit(-1);
    }

    // 2.连接服务器端
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.193.128", &serveraddr.sin_addr.s_addr); //要连接的服务器的IP地址
    serveraddr.sin_port = htons(9999); //要连接的服务器的端口号
    int ret = connect(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); //连接服务器，将serveraddr转换成struct sockaddr *类型

    if(ret == -1) {
        perror("connect");
        exit(-1);
    }

    
    // 3. 通信
    char recvBuf[1024] = {0};
    while(1) {

        char * data = "hello,i am client";
        // 给服务端发送数据
        write(fd, data , strlen(data));
        printf("send to server: %s\n", data);

        sleep(1);
        
        int len = read(fd, recvBuf, sizeof(recvBuf));
        if(len == -1) {
            perror("read");
            exit(-1);
        } else if(len > 0) {
            printf("recv from server (echo): %s\n", recvBuf);
        } else if(len == 0) {
            // 表示服务器端断开连接
            printf("server closed...");
            break; //退出循环
        }

    }

    // 关闭连接
    close(fd);

    return 0;
}