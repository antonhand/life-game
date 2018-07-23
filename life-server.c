#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lifefld.h"
#include "lifeop.h"

int
main(int argc, char *argv[])
{
    //mkfifo("stoc", RIGHTS);
    mkfifo("ctos", RIGHTS);
   /* int fd1 = open("stoc", O_WRONLY, 0);
    dup2(fd1, 1);
    close(fd1);*/
    int fd1 = open("ctos", O_RDONLY, 0);
    dup2(fd1, 0);
    close(fd1);
    int m = atoi(argv[1]);
    int n = atoi(argv[2]);
    int k = atoi(argv[3]);
    if(m < 1){
        printf("ОШИБКА: Слишком маленькая ширина поля, минимальная допустимая ширина поля - 1.\n");
        fflush(stdout);
        m = 1;
    }
    if(n < 1){
        printf("ОШИБКА: Слишком маленькая длина поля, минимальная допустимая длина поля - 1.\n");
        fflush(stdout);
        n = 1;
    }
    printf("Установлены размеры поля: %dx%d\n", m, n);
    fflush(stdout);
    if(k > m * n){
        k = m * n;
        printf("ОШИБКА: Слишком много процессов, максимальное допустимое количество процессов - %d.\n", k);
        fflush(stdout);
    }
    printf("Будет создано %d процессов\n", k);
    fflush(stdout);
    struct fld_param fldp;
    fldp.m = m;
    fldp.n = n;
    fldp.k = k;
    fldp.sqrside = coverfld(m, n, k, &fldp.proc_vert, &fldp.proc_gor);
    printf("%d %d\n", fldp.proc_vert, fldp.proc_gor);
    int *shmid[2];
    shmid[0] = malloc(2 * (fldp.proc_gor - 1) * sizeof(int));
    for(int i = 0; i < 2 * (fldp.proc_gor - 1); i++){
        shmid[0][i] = shmget(IPC_PRIVATE, (m + 2) * sizeof(int), RIGHTS | IPC_CREAT);
        int *tmp_at = shmat(shmid[0][i], NULL, 0);
        int ert = 0;
        for(int j = 0; j < m + 2; j++){
            if(!(j % fldp.sqrside))
                ert += 10;
            tmp_at[j] = 0;
        }
        shmdt(tmp_at);
    }
    shmid[1] = malloc(2 * (fldp.proc_vert - 1) * sizeof(int));
    for(int i = 0; i < 2 * (fldp.proc_vert - 1); i++){
        shmid[1][i] = shmget(IPC_PRIVATE, (n + 2) * sizeof(int), RIGHTS | IPC_CREAT);
        int *tmp_at = shmat(shmid[1][i], NULL, 0);
        int ert = 0;
        for(int j = 0; j < n + 2; j++){
            if(!(j % fldp.sqrside))
                ert += 1000;
            tmp_at[j] = 0;
        }
        shmdt(tmp_at);
    }
    int *msgid = malloc(fldp.proc_vert * fldp.proc_gor * sizeof(int));
    for(int i = 0; i < k; i++){
        msgid[i] = msgget(IPC_PRIVATE, IPC_CREAT | RIGHTS);
    }
    int semid = semget(IPC_PRIVATE, NSEM, IPC_CREAT | RIGHTS);
    int num = 0;
    for(int i = 0; i < fldp.proc_vert; i++){
        for(int j = 0; j < fldp.proc_gor; j++){
            struct proc_param proc;
            proc.num_vert = i;
            proc.num_gor = j;
            proc.vert_side = fldp.sqrside + 2;
            proc.gor_side = fldp.sqrside + 2;
            if (i == fldp.proc_vert - 1){
                proc.vert_side = m - fldp.sqrside * i + 2;
            }
            if (j == fldp.proc_gor - 1){
                proc.gor_side = n - fldp.sqrside * j + 2;
            }
            int br = 0;
            if(m >= n && i == fldp.proc_vert - 1){
                if(j != fldp.proc_gor - 1 && i * fldp.proc_gor + j + 1 == k){
                    proc.gor_side = n - j * fldp.sqrside + 2;
                    br = 1;
                }
            }
            if(n > m && j == fldp.proc_gor - 1){
                if(k % fldp.proc_vert - 1 >= 0 && i == k % fldp.proc_vert - 1){
                    proc.vert_side = m - i * fldp.sqrside + 2;
                }
                if(k % fldp.proc_vert - 1 >= 0 && i > k % fldp.proc_vert - 1){
                    continue;
                }
            }
            if(!fork()){
                int **fld = fldcreate(&fldp, &proc);
                struct border borders;
                get_borders(&borders, &fldp, &proc, shmid);
                int work = 1;
                while(work){
                    struct msgb msg;
                    msgrcv(msgid[num], &msg, DATA_SIZE * sizeof(int), 0, 0);
                    int x, y, ok = 0;
                    if(num == 0){
                        ok = 1;
                    }
                    switch(msg.msgtype){
                    case LF_ADD:
                        x = msg.data[0];
                        y = msg.data[1];
                        if(checkhit(&x, &y, &fldp, &proc)){
                            ok = 1;
                            fld[x][y] = 1;
                            if(y == 1){
                                borders.left_border[x] = 1;
                            }
                            if(y == proc.gor_side - 2){
                                borders.right_border[x] = 1;
                            }
                            if(x == 1){
                                borders.up_border[y] = 1;
                            }
                            if(x == proc.vert_side - 2){
                                borders.down_border[y] = 1;
                            }

                        } else {
                            ok = 0;
                        }
                        break;
                    case LF_CLR:
                        clearfld(fld, &borders, &proc);
                        break;
                    case LF_START:
                        x = msg.data[0];
                        int mode = msg.data[1];
                        ok = 0;
                        if(num == 0){
                            printf("Выполнено.\n");
                            fflush(stdout);
                        }
                        if(start(x, mode, fld, &borders, &fldp, &proc, semid, num, msgid) == LF_QUIT){
                            work = 0;
                        }
                        break;
                    case LF_STOP:

                        if(num == 0){
                            printf("ОШИБКА: игра не была запущена.\n");
                            fflush(stdout);
                        }
                        ok = 0;
                        break;
                    case LF_SNAP:
                        snap(fld, msg.data[0], &fldp, &proc, semid, num);
                        break;
                    case LF_QUIT:
                        work = 0;
                        break;
                    default:
                        break;
                    }
                    if(ok){
                        printf("Выполнено.\n");
                        fflush(stdout);
                    }
                }
                rmfld(fld, &fldp, &proc);
                rmborder(&borders, &fldp, &proc);
                exit(0);
            }
            num++;
            if(br){
                break;
            }
        }
    }
    int work = 1;
    while(work){
        char cmd[CMDSIZE];
        fgets(cmd, CMDSIZE, stdin);
        if(processcmd(cmd, &fldp, msgid, semid) == LF_QUIT){
            break;
        }
    }
    while(wait(NULL) > 0);
    for(int i = 0; i < 2 * (fldp.proc_vert); i++){
        shmctl(shmid[0][i], IPC_RMID, NULL);
    }
    free(shmid[0]);
    for(int i = 0; i < 2 * (fldp.proc_gor - 1); i++){
        shmctl(shmid[1][i], IPC_RMID, NULL);
    }
    free(shmid[1]);
    semctl(semid, NSEM, IPC_RMID, NULL);
    for(int i = 0; i < k; i++){
        msgctl(msgid[i], IPC_RMID, NULL);
    }
    free(msgid);
    return 0;
}
