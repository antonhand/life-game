#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>

enum {
    RIGHTS = 0666,
    CMDSIZE = 32
};

int main(void)
{
    mkfifo("ctos", RIGHTS);
   // mkfifo("stoc", RIGHTS);
    /*if(!fork()){
        int fd1 = open("stoc", O_RDONLY, 0);
        dup2(fd1, 0);
        close(fd1);
        int work = 1;
        while(work){
            char c[70];
            fgets(c, 70, stdin);
            //if(c[0] != '\n')
            puts(c);
        }printf("ee\n");
        exit(0);
    }*/
    int fd2 = open("ctos", O_WRONLY, 0);
    dup2(fd2, 1);
    close(fd2);
    char cmd[CMDSIZE];
    int work = 1;
    while(work){
        cmd[0] = '\0';
        fgets(cmd, CMDSIZE, stdin);
        if(strstr(cmd, "quit") || strstr(cmd, "выход")){
            work = 0;
        }
        cmd[strlen(cmd) - 1] = '\0';
        puts(cmd);
        fflush(stdout);
    }
    //wait(NULL);
    return 0;
}
