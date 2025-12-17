// TCP 通信的服务器端

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main() {

    // 1.创建socket(用于监听的套接字)
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    if(lfd == -1) { 
        perror("socket");
        exit(-1);
    }

    // 2.绑定
    struct sockaddr_in saddr; //使用专用socket地址sockaddr_in
    saddr.sin_family = AF_INET;
    // inet_pton(AF_INET, "192.168.193.128", saddr.sin_addr.s_addr);
    //saddr.sin_addr.s_addr是uint32_t类型，所以需要转换成网络字节序整数
    //inet_pton():点分十进制转换成网络字节序整数

    saddr.sin_addr.s_addr = INADDR_ANY;  // 0.0.0.0 服务器绑定到 0.0.0.0 时，可接受来自任意网络接口的连接
    saddr.sin_port = htons(9999); // 将主机字节序转换成网络字节序 9999是端口号
    int ret = bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr)); //绑定socket地址到socket ,将saddr转换成struct sockaddr *类型

    if(ret == -1) {
        perror("bind");
        exit(-1);
    }

    // 3.监听
    ret = listen(lfd, 8); //监听socket，8是最大连接数
    if(ret == -1) {
        perror("listen");
        exit(-1);
    }

    // 4.接收客户端连接
    struct sockaddr_in clientaddr;
    int len = sizeof(clientaddr);
    int cfd = accept(lfd, (struct sockaddr *)&clientaddr, &len); //接受客户端连接，将clientaddr转换成struct sockaddr *类型
    
    if(cfd == -1) {
        perror("accept");
        exit(-1);
    }

    // 输出客户端的信息
    char clientIP[16];//存储客户端IP地址,最少是16个字符
    inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientIP, sizeof(clientIP)); //将网络字节序整数转换成点分十进制IP地址字符串
    unsigned short clientPort = ntohs(clientaddr.sin_port); //端口：将网络字节序转换成主机字节序
    printf("client ip is %s, port is %d\n", clientIP, clientPort);

    // 5.通信
    char recvBuf[1024] = {0};
    while(1) {
        
        // 1.获取客户端的数据
        int num = read(cfd, recvBuf, sizeof(recvBuf)); 
        if(num == -1) {
            perror("read");
            exit(-1);
        } else if(num > 0) {
            printf("recv client data : %s\n", recvBuf);
            // 2.回射：将接收到的数据原样返回给客户端
            write(cfd, recvBuf, num);
            // 清空缓冲区，准备接收下一次数据
            memset(recvBuf, 0, sizeof(recvBuf));
        } else if(num == 0) { 
            // 表示客户端断开连接
            printf("clinet closed...");
            break; //退出循环
        }
    }
   
    // 关闭文件描述符
    close(cfd);
    close(lfd);

    return 0;
}