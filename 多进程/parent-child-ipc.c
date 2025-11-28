/*
    实现 ps aux | grep xxx 父子进程间通信
    
    子进程： ps aux, 子进程结束后，将数据发送给父进程
    父进程：获取到数据，过滤
    pipe()
    execlp()  实现ps aux 发送到终端
    子进程将标准输出(stdout_fileno) 重定向到管道的写端。  dup2
*/

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

int main() {

    // 创建一个管道
    int fd[2];
    int ret = pipe(fd);

    if(ret == -1) {
        perror("pipe");
        exit(0);
    }

    // 创建子进程
    pid_t pid = fork();

    if(pid > 0) {
        // 父进程

        // 关闭写端
        close(fd[1]);

        // 从管道中读取
        char buf[1024] = {0};

        int len = -1;

        //一次只能读取1023个字符，使用while来循环读取ps aux的内容,只实现获取数据，未实现过滤
        while((len = read(fd[0], buf, sizeof(buf) - 1)) > 0) //sizeof(buf) - 1表示留出一个字符串的结束符，剩余的数组作为缓冲区大小
        {
            printf("%s", buf);
            memset(buf, 0, 1024); //打印之后清空数组中的内容，为下一次获取数据做准备
        }

        wait(NULL); //回收子进程的资源

    } else if(pid == 0) {
        // 子进程

        // 关闭读端
        close(fd[0]);

        // 文件描述符的重定向 stdout_fileno -> fd[1]
        dup2(fd[1], STDOUT_FILENO);
        // 执行 ps aux
        execlp("ps", "ps", "aux", NULL);
        perror("execlp");
        exit(0);
    } else {
        perror("fork");
        exit(0);
    }


    return 0;
}