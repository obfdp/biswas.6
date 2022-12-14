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
#include "queue.c"

void closeChildSignal(int sig);
int closeChild();
void closeProgramSignal(int sig);
void closeProgram();

void setupOutputFile();

void timeGap();

void createProc();

void clkShareSetup();
#define CLOCKVAR 0
#define sharedMemName "86879"
int sharedClkMemID;
int* sharedClkPtr;

void msgQSetup();
#define QUEUEVAR 0
#define msgQName "86880"
int msgQID;

void receiveMsg();

struct msgBuff { 
    long mtype;
    pid_t pid;
    int location;
    char redWrite;
} msg;

FILE* outputFile;

int currentProc;
pid_t openProc[18] = {0};
pid_t blkdProc[18] = {0};
int maxProc;


struct memBlk {
    int pid;
    int referBit;
    int dirtBit;
    int readBit;
    int writeBit;
};

#define totFrames 5
int PAGE[18][32] = {0};
int openFrames[totFrames] = {0};
struct memBlk frames[totFrames];

int nxtProcLocation = -1;

int freeUpFrame();

struct msgQ* QPage;

int main (int argc, char *argv[]) {
    srand ( time(NULL) );
    /*setting signals*/
    signal(SIGCHLD, closeChildSignal);
    signal(SIGINT, closeProgramSignal);

    /*setting the default values and get command line inputs*/
    int c;
    int maxRunTime = 20;
    int maxProc = 18;
    char* logFile = "logFile.txt";

    while ((c = getopt (argc, argv, "hs:l:t:")) != -1){
        switch (c){
            case 'h':
                printf("Help Options:\n-h: Help\n-l: The given argument(string) specifies the name of the logfile.\n-t: The given number(int) specifies max amount of time the program will run for.\n");
                exit(0);
                break;
            case 's':
                if (atoi(optarg) <= 0 || atoi(optarg) > 18){
                    maxProc = 18;
                } else {
                    maxProc = atoi(optarg);
                }
                break;
            case 'l':
                logFile = optarg;
                break;
            case 't':
                maxRunTime = atoi(optarg);
                break;
            default:
                printf("there was an error with arguments");
                exit(1);
                break;
        }
    }

    /*setting up the output file*/
    setupOutputFile();

    /*display run parameters*/
    printf("Log file name: %s\n", logFile);
    fprintf(outputFile, "Log file name: %s\n", logFile);
    printf("Max run time: %d\n", maxRunTime);
    fprintf(outputFile, "Max run time: %d\n", maxRunTime);

    /*Initializing various shared memory*/
    clkShareSetup();
    sharedClkPtr[0] = 0;
    sharedClkPtr[1] = 0;

    msgQSetup();

    QPage = createQ(totFrames);

    while(sharedClkPtr[0] < 1){
    /* while(1==1){*/
        if ((currentProc < maxProc)){
            createProc();
        }
        timeGap();
        receiveMsg();
    }

    closeProgram();
}

void closeChildSignal(int sig){
    closeChild();
}

int closeChild(){
    pid_t closedChild = wait(NULL);
    if (closedChild > 0){
        int i;
        for(i = 0; i < 18; i++){
            if (openProc[i] == closedChild){
                openProc[i] = 0;
            }
        }
        currentProc--;
    }
    return closedChild;
}

void closeProgramSignal(int sig){
    closeProgram();
}

void closeProgram(){
    msgctl(msgQID, IPC_RMID, NULL);
    shmctl(sharedClkMemID, IPC_RMID, NULL);
    /* shmdt(sharedClkPtr);*/
    fclose(outputFile);
    int i;
    for(i = 0; i < 18; i++){
        if (openProc[i] != 0){
            kill(openProc[i], SIGINT);
        }
    }
    printf("Exiting.\n");
    while (closeChild() > 0){}
    exit(0);
}

void setupOutputFile(){
    char* logFile = "logFile.txt";
    outputFile = fopen(logFile, "w");
    if (outputFile == NULL){
        printf("Failed to open output file.\n");
        fprintf(outputFile, "Failed to open output file.\n");
        closeProgram();
    }
}

void clkShareSetup(){
    key_t shareClkKey;
	
    if (-1 != open(sharedMemName, O_CREAT, 0666)) {
        shareClkKey = ftok(sharedMemName, CLOCKVAR);
    } else {
        printf("ftok error in parrent: clkShareSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    sharedClkMemID = shmget(shareClkKey, sizeof(int)*2, IPC_CREAT | 0666);
    if (sharedClkMemID < 0) {
        printf("shmget error in parrent: clkShareSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    sharedClkPtr = (int *) shmat(sharedClkMemID, NULL, 0);
    if ((long) sharedClkPtr == -1) {
        printf("shmat error in parrent: clkShareSetup\n");
        printf("Error: %d\n", errno);
        shmctl(sharedClkMemID, IPC_RMID, NULL);
        exit(1);
    }
}

void timeGap(){
    sharedClkPtr[1] += 1000;
    while (sharedClkPtr[1] >= 1000000000){
        sharedClkPtr[1] -= 1000000000;
        sharedClkPtr[0]++;
        printf("%d:%d\n", sharedClkPtr[0], sharedClkPtr[1]);
        fprintf(outputFile, "%d:%d\n", sharedClkPtr[0], sharedClkPtr[1]);
    }
}

void createProc(){
    if (nxtProcLocation < 0){
        int randNumber = 1000;
        nxtProcLocation = randNumber + sharedClkPtr[1];
    }

    if ((sharedClkPtr[1] > nxtProcLocation) && (nxtProcLocation > 0)){
        int i;
        int openSpace;
        for(i = 0; i < 18; i++){
            if (openProc[i] == 0){
                openSpace = i;
                break;
            }
        }
        pid_t newForkPid;
        newForkPid = fork();
        if (newForkPid == 0){
            execlp("./user","./user", NULL);
            fprintf(stderr, "Failed to exec user!\n");
            fprintf(outputFile, "Failed to exec user!\n");
            exit(1);
        }
        openProc[openSpace] = newForkPid;
        nxtProcLocation = -1;
        currentProc++;
    }
}

void msgQSetup(){
    key_t MsgQueue_Key;
    if (-1 != open(msgQName, O_CREAT, 0777)) {
        MsgQueue_Key = ftok(sharedMemName, QUEUEVAR);
    } else {
        printf("ftok error in parrent: msgQSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }

    msgQID = msgget(MsgQueue_Key, (IPC_CREAT | 0777));
    if (msgQID < 0) {
        printf("msgget error in parrent: clkShareSetup\n");
        printf("Error: %d\n", errno);
        exit(1);
    }
}

void receiveMsg(){
    int msgRecived  = msgrcv(msgQID, &msg, sizeof(msg), 1, IPC_NOWAIT);
    if (msgRecived == -1){
        return;
    }

    pid_t reqPID = msg.pid;
    int requestedAddress = msg.location;
    int requestedPage = requestedAddress / 1024;
    char redWrite = msg.redWrite;

    printf("Master: P%d requesting read of address %d at time %d:%d\n", reqPID, requestedAddress, sharedClkPtr[0], sharedClkPtr[1]);
    fprintf(outputFile, "Master: P%d requesting read of address %d at time %d:%d\n", reqPID, requestedAddress, sharedClkPtr[0], sharedClkPtr[1]);

    int j;
    int procLocation;
    for(j = 0; j < 18; j++){
        if (openProc[j] == reqPID){
            procLocation = j;
            break;
        }
    }

    if (PAGE[procLocation][requestedPage] == 0){
        printf("Master: Address %d is not in a frame, pagefault.\n", requestedAddress);
        fprintf(outputFile, "Master: Address %d is not in a frame, pagefault.\n", requestedAddress);
        int j;
        int openFrame = -1;
        for(j = 0; j < totFrames; j++){
            if (openFrames[j] == 0){
                openFrame = j;
                printf("Master: Frame %d is open, page %d will be loaded here.\n", openFrame, requestedPage);
                fprintf(outputFile, "Master: Frame %d is open, page %d will be loaded here.\n", openFrame, requestedPage);
                break;
            }
        }
        if (openFrame < 0){
            openFrame = freeUpFrame();
            printf("Master: Clearing frame %d and swapping in P%d page %d \n", openFrame, reqPID, requestedPage);
            fprintf(outputFile, "Master: Clearing frame %d and swapping in P%d page %d .\n", openFrame, reqPID, requestedPage);
			
			     printf("--------------Frame %d is NOT OCCUPIED --------------\n", openFrame);
            fprintf(outputFile, "----------Frame %d is NOT OCCUPIED ---------------\n", openFrame);
        }

        if (redWrite == 'w'){
            PAGE[procLocation][requestedPage] = openFrame;
            struct memBlk newBlk;
            newBlk.dirtBit = 1;
            newBlk.pid = reqPID;
            newBlk.readBit = 0;
            newBlk.referBit = 1;
            newBlk.writeBit = 1;
            frames[openFrame] = newBlk;
            openFrames[openFrame] = 1;
            enQueue(QPage, openFrame);
			
			printf("--------------Dirty Bit is set to 1 for frame %d at time %d:%d --------------\n", openFrame, sharedClkPtr[0], sharedClkPtr[1]);
            fprintf(outputFile, "--------------Dirty Bit is set to 1 for frame %d at time %d:%d -----------\n",openFrame, sharedClkPtr[0], sharedClkPtr[1]);
			
			
	
			
			printf("Master: Address %d in frame %d, writing data to frame at time %d:%d\n", requestedAddress, openFrame, sharedClkPtr[0], sharedClkPtr[1]);
            fprintf(outputFile, "Master: Address %d in frame %d, writing data to frame at time %d:%d\n", requestedAddress, openFrame, sharedClkPtr[0], sharedClkPtr[1]);
			
			
        } else {
            PAGE[procLocation][requestedPage] = openFrame;
            struct memBlk newBlk;
            newBlk.dirtBit = 1;
            newBlk.pid = reqPID;
            newBlk.readBit = 1;
            newBlk.referBit = 1;
            newBlk.writeBit = 0;
            frames[openFrame] = newBlk;
            openFrames[openFrame] = 1;
            enQueue(QPage, openFrame);

			printf("Master: Address %d in frame %d, giving data to P%d at time %d:%d\n", requestedAddress, openFrame, reqPID, sharedClkPtr[0], sharedClkPtr[1]);
			fprintf(outputFile, "Master: Address %d in frame %d, giving data to P%d at time %d:%d\n", requestedAddress, openFrame, reqPID, sharedClkPtr[0], sharedClkPtr[1]);
          
	;
        }
    } else {
        if (redWrite == 'w'){
            int blockInFrame = PAGE[procLocation][requestedPage];
            struct memBlk block = frames[blockInFrame];
            block.dirtBit = 1;
            block.writeBit = 1;
            block.referBit = 1;
			
            printf("Master: Dirty bit of frame %d set, adding additional time to the clock\n", blockInFrame);
            fprintf(outputFile, "Master: Dirty bit of frame %d set, adding additional time to the clock\n", blockInFrame);
        } else {
            int blockInFrame = PAGE[procLocation][requestedPage];
            struct memBlk block = frames[blockInFrame];
            block.dirtBit = 1;
            block.readBit = 1;
            block.referBit = 1;
            printf("Master: Process %d read from page %d: already in frame %d.\n", reqPID, requestedPage, blockInFrame);
            fprintf(outputFile, "Master: Process %d read from page %d: already in frame %d.\n", reqPID, requestedPage, blockInFrame);
        }
    }

    msg.mtype = reqPID;

    int msgSent = msgsnd(msgQID, &msg, sizeof(msg), 0);
    if (msgSent < 0){
        printf("Parrent: failed to send msg.\n");
    }
	
    printf("Master: Indicating to P%d that write has happened to address %d.\n", reqPID, requestedAddress);
    fprintf(outputFile, "Master: Indicating to P%d that write has happened to address %d.\n", reqPID, requestedAddress);
}

int freeUpFrame(){
    for (;;){
        int delCand = first(QPage);

        if (frames[delCand].referBit == 0 && frames[delCand].dirtBit == 0){
            openFrames[delCand] = 0;
            deQueue(QPage);
			
			printf("--------------Dirty Bit is set to 0 for frame %d at time %d:%d ------------------\n", delCand, sharedClkPtr[0], sharedClkPtr[1]);
            fprintf(outputFile, "---------------Dirty Bit is set to 0 for frame %d at time %d:%d------------------\n",delCand, sharedClkPtr[0], sharedClkPtr[1]); 
			
            return delCand;
        } else {
            frames[delCand].referBit = 0;
            frames[delCand].dirtBit = 0;
            deQueue(QPage);
            enQueue(QPage, delCand);
            printf("----------Frame %d is OCCUPIED at time %d:%d--------------*\n", delCand, sharedClkPtr[0], sharedClkPtr[1]);
            fprintf(outputFile, "----------Frame %d is OCCUPIED at time %d:%d-------------- \n", delCand, sharedClkPtr[0], sharedClkPtr[1]);
        }
    }
}
