#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
#include "lifeop.h"
#include "lifefld.h"

int processcmd(char *cmd, struct fld_param *fldp, int *msgid, int semid)
{
    char oper[CMDSIZE];
    oper[0] = '\0';
    struct msgb msg;
    msg.data[0] = 0;
    msg.data[1] = 0;
    sscanf(cmd, "%s", oper);
    if(!strcmp(oper, "add") || !strcmp(oper, "добавить") ){
        int x, y;
        if(sscanf(cmd, "%s%d%d", oper, &x, &y) == 3){
            if(x > 0 && x <= fldp->m && y > 0 && x <= fldp->n){
                msg.msgtype = LF_ADD;
                msg.data[0] = x;
                msg.data[1] = y;

            } else {
                printf("ОШИБКА: указанная клетка находится за пределом поля.\n");
                fflush(stdout);
                return LF_ER_OUTOFFLD;
            }
        } else {
            printf("ОШИБКА: неверное количество аргументов.\n");
            fflush(stdout);
            return LF_ER_WRNUMARG;
        }
    } else if(!strcmp(oper, "clear") || !strcmp(oper, "очистить")){

        msg.msgtype = LF_CLR;
    } else if(!strcmp(oper, "start") || !strcmp(oper, "старт")){
        int P;
        msg.msgtype = LF_START;
        if(sscanf(cmd, "%s%d", oper, &P) > 1){
            if(P > 0){
                msg.data[0] = P;
            } else {
                printf("ОШИБКА: количество поколений должно быть больше нуля.\n");
                fflush(stdout);
                return LF_ER_BADARG;
            }

        } else {
            msg.data[0] = -1;
        }
        int x = 0;
        if(strstr(cmd, "-rt") || strstr(cmd, "-рв")){
            x = 1;
        }
        msg.data[1] = x;
    } else if(!strcmp(oper, "stop") || !strcmp(oper, "стоп")){
        msg.msgtype = LF_STOP;
    } else if(!strcmp(oper, "snapshot") || !strcmp(oper, "состояние")){
        msg.msgtype = LF_SNAP;
        int x = 0;
        if(strstr(cmd, "-prcs") || strstr(cmd, "-прцс")){
            x = 1;
        }
        msg.data[0] = x;
    } else if(!strcmp(oper, "quit") || !strcmp(oper, "выход")){
        msg.msgtype = LF_QUIT;
    } else {
        printf("ОШИБКА: команда не найдена.\n");
        fflush(stdout);
        return LF_ER_CMD;
    }
    struct sembuf sem[2];
    sem[0].sem_num = 2;
    sem[0].sem_flg = 0;
    sem[0].sem_op = 0;
    sem[1].sem_num = 1;
    sem[1].sem_flg = 0;
    sem[1].sem_op = 1;
    semop(semid, sem, 2);
    for(int i = 0; i < fldp->k; i++){
        msgsnd(msgid[i], &msg, DATA_SIZE * sizeof(int), 0);
    }
    sem[0].sem_num = 1;
    sem[0].sem_op = -1;
    semop(semid, sem, 1);
    return msg.msgtype;
}

void
clearfld(int **fld, struct border *borders, struct proc_param *proc)
{
    for(int i = 0; i < proc->vert_side; i++){
        for(int j = 0; j < proc->gor_side; j++){
            fld[i][j] = 0;
        }
        borders->left_border[i] = 0;
        borders->right_border[i] = 0;
    }
    return;
}

int // 0, 3 - мёртв, 1, 2 - жив
start(
    int P,
    int mode,
    int **fld,
    struct border *borders,
    struct fld_param *fldp,
    struct proc_param *proc,
    int semid,
    int num,
    int *msgid)
{
    struct sembuf sem[2];
    sem[0].sem_flg = 0;
    sem[1].sem_flg = 0;
    int iter = abs(P);
    while(iter){
        sem[0].sem_num = 0;
        sem[1].sem_num = 0;
        for(int i = 0; i < proc->vert_side; i++){
            fld[i][0] = borders->left_neighbor[i] + 2;
            fld[i][proc->gor_side - 1] = borders->right_neighbor[i] + 2;
        }
        for(int j = 0; j < proc->gor_side; j++){
            if(fld[0][j] != 3){
                fld[0][j] = borders->up_neighbor[j];
            } else {
                fld[0][j] = 1;
            }
            if(fld[proc->vert_side - 1][j] != 3){
                fld[proc->vert_side - 1][j] = borders->down_neighbor[j];
            } else {
                fld[proc->vert_side - 1][j] = 1;
            }
        }
        for(int i = 0; i < proc->vert_side; i++){
            fld[i][0] %= 2;
            fld[i][proc->gor_side - 1] %= 2;
        }
        for(int i = 1; i < proc->vert_side - 1; i++){
            for(int j = 1; j < proc->gor_side - 1; j++){
                int count_alive = 0;
                for(int t = -1; t <= 1; t++){
                    for(int f = -1; f <= 1; f++){
                        if((t != 0 || f != 0) && fld[i + t][j + f] % 2){
                            count_alive++;
                        }
                    }
                }
                if(fld[i][j] && (count_alive > LF_MAXNEED || count_alive < LF_MINNEED)){
                    fld[i][j] += 2;
                } else if(!fld[i][j] && count_alive == LF_BORN){
                    fld[i][j] += 2;
                }
            }
        }
        for(int i = 1; i < proc->vert_side - 1; i++){
            for(int j = 1; j < proc->gor_side - 1; j++){
                if(fld[i][j] == 3){
                    fld[i][j] = 0;
                }
                if(fld[i][j] == 2){
                    fld[i][j] = 1;
                }
            }
         }
        sem[0].sem_op = 1;
        semop(semid, sem, 1);
        if(num == 0){
            sem[0].sem_op = -fldp->k;
            semop(semid, sem, 1);
        } else {
            sem[0].sem_op = 0;
            semop(semid, sem, 1);
        }
        for(int i = 1; i < proc->vert_side - 1; i++){
            borders->left_border[i] = fld[i][1];
            borders->right_border[i] = fld[i][proc->gor_side - 2];
        }
        for(int j = 1; j < proc->gor_side - 1; j++){
            borders->up_border[j] = fld[1][j];
            borders->down_border[j] = fld[proc->vert_side - 2][j];
        }
        if(P > 0){
            iter--;
        }
        sem[0].sem_num = 5;
        sem[0].sem_op = 1;
        semop(semid, sem, 1);
        if(num == 0){
            sem[0].sem_op = -fldp->k;
            semop(semid, sem, 1);
        } else {
            sem[0].sem_op = 0;
            semop(semid, sem, 1);
        }
        if(mode){
            usleep(100000);
            system("clear");
            snap(fld, 0, fldp, proc, semid, num);
        }
        sem[0].sem_num = 2;
        sem[0].sem_op = 1;
        sem[1].sem_num = 1;
        sem[1].sem_op = 0;
        semop(semid, sem, 2);
        struct msgb msg;
        if(msgrcv(msgid[num], &msg, DATA_SIZE * sizeof(int), 0, IPC_NOWAIT) == -1){
            sem[0].sem_num = 2;
            sem[0].sem_op = 1;
            semop(semid, sem, 1);
            if(num == 0){
            sem[0].sem_op = -2 * fldp->k;
            semop(semid, sem, 1);
            }
            continue;
        }
        sem[0].sem_num = 2;
        sem[0].sem_op = 1;
        semop(semid, sem, 1);
        if(num == 0){
            sem[0].sem_op = -2 * fldp->k;
            semop(semid, sem, 1);
        }
        int x, y;
        int ok = 0;
        switch(msg.msgtype){
        case LF_ADD:
            x = msg.data[0];
            y = msg.data[1];
            if(checkhit(&x, &y, fldp, proc)){
                fld[x][y] = 1;
                ok = 1;
                if(y == 1){
                    borders->left_border[x] = 1;
                }
                if(y == proc->gor_side - 2){
                    borders->right_border[x] = 1;
                }
                if(x == 1){
                    borders->up_border[y] = 1;
                }
                if(x == proc->vert_side - 2){
                    borders->down_border[y] = 1;
                }
            }
            break;
        case LF_CLR:
            clearfld(fld, borders, proc);
            break;
        case LF_START:
            if(num == 0){
                printf("ОШИБКА: Игра уже запущена.\n");
                fflush(stdout);
            }
            ok = 0;
            break;
        case LF_STOP:
            iter = 0;
            if(num == 0){
                printf("Выполнено.\n");
                fflush(stdout);
            }
            break;
        case LF_SNAP:
            snap(fld, msg.data[0], fldp, proc, semid, num);
            break;
        case LF_QUIT:
            if(num == 0){
                printf("Выполнено.\n");
                fflush(stdout);
            }
            return LF_QUIT;
            break;
        default:
            break;
        }
        if(ok){
            printf("Выполнено.\n");
            fflush(stdout);
        }
    }
    return 0;
}

void
snap(int **fld, int mode, struct fld_param *fldp, struct proc_param *proc, int semid, int num)
{
    key_t key = ftok("life-server", 'r');
    int shmprint = shmget(key, (fldp->n + 1) * sizeof(char), RIGHTS | IPC_CREAT);
    char *shm = shmat(shmprint, NULL, 0);
    struct sembuf sem[2];
    sem[0].sem_num = 3;
    sem[0].sem_flg = 0;
    sem[1].sem_num = 4;
    sem[1].sem_flg = 0;
    if(num == 0){
            shm[fldp->n] = '\0';
    }
    shm += proc->num_gor * fldp->sqrside;
    for(int curline = 1; curline <= fldp->m; curline++){
        sem[1].sem_op = 0;
        semop(semid, &sem[1], 1);

        int x = curline;
        if(checkhit(&x, NULL, fldp, proc)){
            for(int i = 1; i < proc->gor_side - 1; i++){
                if(!mode){
                    if(fld[x][i]){
                        shm[i - 1] = '*';
                    } else {
                        shm[i - 1] = '.';
                    }
                } else {
                    shm[i - 1] = num;
                }
            }
        }

        sem[0].sem_op = 1;
        semop(semid, sem, 1);
        if(num == 0){
            sem[0].sem_op = -fldp->k;
            semop(semid, sem, 1);
            if(!mode){
                puts(shm);
            } else {
                for(int i = 0; i < fldp->n; i++){
                    printf("%2d ", shm[i]);
                }
                printf("\n");
                fflush(stdout);
            }
            fflush(stdout);
            sem[1].sem_op = -fldp->k + 1;
            semop(semid, &sem[1], 1);
        }
        if(num != 0){
            sem[0].sem_op = 0;
            semop(semid, sem, 1);
            sem[1].sem_op = 1;
            semop(semid, sem, 2);
        }
    }
    shmdt(shm - proc->num_gor * fldp->sqrside);
    shmctl(shmprint, IPC_RMID, NULL);
    return;
}

