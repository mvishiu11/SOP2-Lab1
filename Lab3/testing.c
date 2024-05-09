
#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#define ERR(source) \
        (fprintf(stderr,"%s:%d", __FILE__,__LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define SEM_NAME "/sem2"

int main(int argc, char** argv)
{
    int init = 0;
    sem_t* sem;
    if (argc != 2) ERR("Bad params");
    int n = atoi(argv[1]);

    for (int i=0; i < n; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if(init)
            {
                sem = sem_open(SEM_NAME, O_RDWR, 0666);
                if (sem < 0) ERR("sem_open");
            }
            else
            {
                sem = sem_open(SEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666, 1);
                init = 1;
                if (sem < 0) ERR("sem_open");
            }
            sem_wait(sem);
            srand(getpid());
            printf("[%d] Waiting for %d seconds\n", getpid(), rand() % 3 + 2);
            sleep(rand() % 3 + 2);
            sem_post(sem);
        }
        if (pid == -1) ERR("fork");
        else
        {
            printf("Parent registered %d process\n", pid);
        }
    }

    while(wait(NULL) > 0);

    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}