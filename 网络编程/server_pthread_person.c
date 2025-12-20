#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//6.创建结构体用于存储客户端的信息
struct socketinfo{
    int fd;//用于通信的文件描述符
    struct socketaddr_in addr; //存储客户端的信息（ip，port）
    pthread_t tid; //线程号
}
//7.子线程与客户端通信的函数
void *working(void *arg){
    //将arg转换成struct socketinfo *类型
    struct socketinfo *pinfo = (struct socketinfo *)arg;

    //获取客户端的信息
    char clientIp[16];//存储客户端的ip地址
    inet_ntop(AF_INET,&pinfo->addr.sin_addr.s_addr,clinetIp,sizeof(clientIp));
    unsigned short clinetPort = ntohs(pinfo->addr.sin_port);//将网络字节序转换成主机字节序是为了后续打印
    printf("clinet ip is %s,port is %d\n",clinetIp,clinetPort);

    //接收客户端发来的数据并给客户端发送数据
    char recvBuf[1024];
    while(1){
        //读取客户端发来的数据
        int  len = read(pinfo->fd,recvBuf,sizeof(recvBuf)); //读取数据的sizeof(recvBuf)个字节，并存储到recvBuf中

        if(len == -1){
            perror("read");
        }
        else if(len > 0){
            printf("recv clinet message is %s\n",recvBuf);
       }
       else if(len == 0){
        printf("clinet closed ...\n");
        break;
       }

       //给客户端发送数据
       write(pinfo->fd,recvBuf,sizeof(recvBuf)+1);//注意：write() 写入的字节数包括 '\0'，所以需要 +1 确保字符串以 '\0' 结尾
       //总结：发送数据时长度需要手动+1，读取数据时长度不需要手动+1
    }
    close(pinfo->fd);
    return NULL;
}
//8.规定最多允许接入的客户端数量
struct socketinfo socketinfos[128];//最多允许接入128个客户端

int main() {
    //1.创建socket
     int lfd = socket(PF_INET,SOCK_STREAM,0);
     if(lfd==-1){
        perror("socket");
        exit(-1);
    }

    struct socketaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999); //将主机字节序转换成网络字节序是为了后续封装TCP头部
    addr.sin_addr.s_addr = INADDR_ANY; //将IP地址转换成网络字节序是为了后续封装TCP头部  

    //2.bind绑定
    int ret = bind(lfd,(struct socketaddr*)&addr,sizeof(addr));

    //3.listen监听
    int ret = listen(lfd,128);
    if(ret==-1){
        perror("listen");
        exit(-1);    
    }

    //9.初始化socketinfos数组
    int max = sizeof(socketinfos) / sizeof(socketinfos[0]); //数组长度数除以每个元素的长度，得到数组长度
    for(int i=0;i<max;i++){
        bzero(&socketinfos[i],sizeof(socketinfos[i])); //将socketinfos[i]中的每个字节都初始化为0
        socketinfos[i].fd = -1; //将socketinfos[i].fd初始化为-1，表示文件描述符无效(0-1023是系统保留可使用的文件描述符)，表示可以被使用
        socketinfos[i].tid = -1; //将socketinfos[i].tid初始化为-1，表示线程号无效(线程号是pthread_t类型，是一个无符号整数)，表示没有线程与之对应
    }

    while(1){

        struct socketaddr_in clientaddr;
        int len = sizeof(clientaddr);
        //4.循环等待客户端连接
        int cfd = accept(lfd,(struct socketaddr*)&clientaddr,&len); //第二个参数是客户端的信息
        if(cfd==-1){
            perror("accept");
            exit(-1);
        }

        //由于pthread_create最后一个参数只能传递一个参数，所以需要传递一个结构体
        struct socketinfo *pinfo;
        //10.从socketinfos数组中找到一个可以使用的元素
        for(int i=0;i<max;i++){
            if(socketinfos[i].fd==-1){
                pinfo = &socketinfos[i]; //将pinfo指向socketinfos[i]，即pinfo指向了可以被使用的元素
                break;
            }
            else if(i == max-1){
                sleep(1); //如果没找到可以被使用的元素，则等待1秒后继续遍历
                i = -1; //将i重置为-1，以便于继续遍历
            }
        }

        //5.创建子线程用于实现与客户端的通信
        pinfo->fd = cfd;
        memcpy(&pinfo->affr,&clientaddr,sizeof(clientaddr));
        pthread_create(&pinfo->tid,NULL,working,pinfo); //子线程要实现与客户端通信，必须知道客户端的信息（ip，port，文件描述符）

        //11.线程分离，回收子线程资源
        pthread_detach(pinfo->tid);
    }
    close(lfd);
    return 0;
}