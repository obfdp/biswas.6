#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h> 

void closeProgramSignal(int sig);
void closeProgram();

void clkShareSetup();
#define CLOCKVAR 0
#define sharedMemName "86879"
int sharedClkMemID;
int* sharedClkPtr;

void msgQSetup();
#define QUEUEVAR 0
#define msgQName "86880"
int msgQID;

struct msgBuff { 
    long mtype;
    pid_t pid;
    int location;
    char redWrite;
} msg;

int main (int argc, char *argv[]) {
    signal(SIGINT, closeProgramSignal);
    srand ( time(NULL) );

    msgQSetup();

    int i;
    for(i = 0; i < 300; i++){
        msg.mtype = 1;
        msg.redWrite = (rand() % 2) == 0 ? 'r' : 'w';
        msg.location = rand() % 86870;
        msg.pid = getpid();
        int msgSent = msgsnd(msgQID, &msg, sizeof(msg), 0);
        if (msgSent < 0){
            printf("Child %d: failed to send msg.\n", getpid());
            printf("Error: %d\n", errno);
            closeProgram();
        }

        msgrcv(msgQID, &msg, sizeof(msg), getpid(), 0);
    }

    closeProgram();
}

void closeProgramSignal(int sig){
    closeProgram();
}

void closeProgram(){
    shmdt(sharedClkPtr);
    exit(0);
}

void clkShareSetup(){
    key_t shareClkKey;
    if (-1 != open(sharedMemName, O_CREAT, 0777)) {
        shareClkKey = ftok(sharedMemName, CLOCKVAR);
    } else {
        printf("ftok error in child: clkShareSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    sharedClkMemID = shmget(shareClkKey, sizeof(int)*2, IPC_CREAT | 0777);
    if (sharedClkMemID < 0) {
        printf("shmget error in child: clkShareSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    sharedClkPtr = (int *) shmat(sharedClkMemID, NULL, 0);
    if ((long) sharedClkPtr == -1) {
        printf("shmat error in child: clkShareSetup\n");
        printf("Error: %d\n", errno);
        shmctl(sharedClkMemID, IPC_RMID, NULL);
        exit(1);
    }
}

void msgQSetup(){
    key_t MsgQueue_Key;
    if (-1 != open(msgQName, O_CREAT, 0777)) {
        MsgQueue_Key = ftok(sharedMemName, QUEUEVAR);
    } else {
        printf("ftok error in child: msgQSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    msgQID = msgget(MsgQueue_Key, 0777 |IPC_CREAT);
    if (msgQID < 0) {
        printf("msgget error in child: msgQSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }
}

