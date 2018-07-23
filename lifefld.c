#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include "lifefld.h"

int
coverfld(int m, int n, int k, int *procvert, int *procgor)
{
    int proc_vert = 0;
    int proc_gor = 0;
    int maxarea = 0;
    int maxside = m;
    int minside = n;
    if(n > m){
        maxside = n;
        minside = m;
    }
    int sqrside = 0;
    for(int i = 1; i <= sqrt(k) + 1; i++){
        int side = i;
        int other_side = k / i;
        int tmpsqrside = minside / i;
        if(maxside / other_side < tmpsqrside){
            tmpsqrside = maxside / other_side;
        }
        if(k % i){
            side++;
        }
        if(i <= minside){
            int tmp_area = tmpsqrside * tmpsqrside  * k;
            if (tmp_area > maxarea){
                maxarea = tmp_area;
                sqrside = tmpsqrside;
                if(maxside == m){
                    proc_vert = other_side;
                    proc_gor = side;
                } else{
                    proc_vert = side;
                    proc_gor = other_side;
                }
            }
        }
    }
    *procvert = proc_vert;
    *procgor = proc_gor;
    return sqrside;
}

int **
fldcreate(struct fld_param *fldp, struct proc_param *proc)
{
    int **fld = malloc(proc->vert_side * sizeof(int *));
    for(int i = 0; i < proc->vert_side; i++){
        fld[i] = calloc(proc->gor_side, sizeof(int));
    }
    return fld;
}

void
get_borders(struct border *borders, struct fld_param *fldp, struct proc_param *proc, int **shmid)
{
    if(proc->num_gor != 0){
        borders->left_neighbor = shmat(shmid[0][2 * proc->num_gor - 2], NULL, 0);
        borders->left_neighbor += proc->num_vert * fldp->sqrside;

        borders->left_border = shmat(shmid[0][2 * proc->num_gor - 1], NULL, 0);
        borders->left_border += proc->num_vert * fldp->sqrside;


    } else {
        borders->left_neighbor = calloc(proc->vert_side, sizeof(int));
        borders->left_border = calloc(proc->vert_side, sizeof(int));
    }
    if(fldp->n != proc->gor_side - 2 + proc->num_gor * fldp->sqrside){

        borders->right_border = shmat(shmid[0][2 * proc->num_gor], NULL, 0);
        borders->right_border += proc->num_vert * fldp->sqrside;
        borders->right_neighbor = shmat(shmid[0][2 * proc->num_gor + 1], NULL, 0);
        borders->right_neighbor += proc->num_vert * fldp->sqrside;

    } else {
        borders->right_neighbor = calloc(proc->vert_side, sizeof(int));
        borders->right_border = calloc(proc->vert_side, sizeof(int));
    }
    if(proc->num_vert != 0){
        borders->up_neighbor = shmat(shmid[1][2 * proc->num_vert - 2], NULL, 0);
        borders->up_neighbor += proc->num_gor * fldp->sqrside;

        borders->up_border = shmat(shmid[1][2 * proc->num_vert - 1], NULL, 0);
        borders->up_border += proc->num_gor * fldp->sqrside;
    } else {
        borders->up_neighbor = calloc(proc->gor_side, sizeof(int));
        borders->up_border = calloc(proc->gor_side, sizeof(int));
    }
    if(fldp->m != proc->vert_side - 2 + proc->num_vert * fldp->sqrside){
        borders->down_border = shmat(shmid[1][2 * proc->num_vert], NULL, 0);
        borders->down_border += proc->num_gor * fldp->sqrside;
        borders->down_neighbor = shmat(shmid[1][2 * proc->num_vert + 1], NULL, 0);

        borders->down_neighbor += proc->num_gor * fldp->sqrside;
    } else {
        borders->down_neighbor = calloc(proc->gor_side, sizeof(int));
        borders->down_border = calloc(proc->gor_side, sizeof(int));
    }
    return;
}

void
rmfld(int **fld, struct fld_param *fldp, struct proc_param *proc)
{
    for(int t = 0; t < proc->vert_side; t++){
       free(fld[t]);
    }
    free(fld);
    return;
}

void
rmborder(struct border *borders, struct fld_param *fldp, struct proc_param *proc)
{
    if(proc->num_gor != 0){
        shmdt(borders->left_neighbor - proc->num_vert * fldp->sqrside);
        shmdt(borders->left_border - proc->num_vert * fldp->sqrside);
    } else {
        free(borders->left_neighbor);
        free(borders->left_border);
    }
    if(fldp->n != proc->gor_side - 2 + proc->num_gor * fldp->sqrside){
        shmdt(borders->right_neighbor - proc->num_vert * fldp->sqrside);
        shmdt(borders->right_border - proc->num_vert * fldp->sqrside);
    } else {
        free(borders->right_neighbor);
        free(borders->right_border);
    }
    if(proc->num_vert != 0){
        shmdt(borders->up_neighbor - proc->num_gor * fldp->sqrside);
        shmdt(borders->up_border - proc->num_gor * fldp->sqrside);
    } else {
        free(borders->up_neighbor);
        free(borders->up_border);
    }
    if(fldp->m != proc->vert_side - 2 + proc->num_vert * fldp->sqrside){
        shmdt(borders->down_neighbor - proc->num_gor * fldp->sqrside);
        shmdt(borders->down_border - proc->num_gor * fldp->sqrside);
    } else {
        free(borders->down_neighbor);
        free(borders->down_border);
    }
    return;
}

int
checkhit(int *x, int *y, struct fld_param *fldp, struct proc_param *proc)
{
    int x1 = *x;
    if(proc->num_vert * fldp->sqrside < x1 && x1 <= proc->num_vert * fldp->sqrside + proc->vert_side - 2){
        x1 -= proc->num_vert * fldp->sqrside;
    } else {
        return 0;
    }
    if(y){
        int y1 = *y;
        if(proc->num_gor * fldp->sqrside < y1 && y1 <= proc->num_gor * fldp->sqrside + proc->gor_side - 2){
            y1 -= proc->num_gor * fldp->sqrside;
        } else {
            return 0;
        }
        *y = y1;
    }
    *x = x1;
    return 1;
}
