#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mqueue.h>
#include <unistd.h>
#include <string.h>

#define QUEUE_PERMISSIONS 0660
#define BUFFER_SIZE 128

mqd_t mq_s, mq_d, mq_m;
char queue_name_s[20], queue_name_d[20], queue_name_m[20];

void cleanup() {
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    mq_unlink(queue_name_s);
    mq_unlink(queue_name_d);
    mq_unlink(queue_name_m);
}

void handle_sigint(int sig) {
    cleanup();
    exit(0);
}

void process_message(char *buffer, mqd_t mq, const char *operation) {
    int client_pid, num1, num2;
    sscanf(buffer, "%d %d %d", &client_pid, &num1, &num2);
    int result = 0;

    if (strcmp(operation, "sum") == 0) {
        result = num1 + num2;
    } else if (strcmp(operation, "div") == 0) {
        if (num2 != 0) { 
            result = num1 / num2;
        } else {
            result = 0;
        }
    } else if (strcmp(operation, "mod") == 0) {
        if (num2 != 0) {
            result = num1 % num2;
        } else {
            result = 0;
        }
    }

    char client_queue_name[20], response[20];
    sprintf(client_queue_name, "/%d", client_pid);
    sprintf(response, "%d", result);
    mqd_t mq_client = mq_open(client_queue_name, O_WRONLY);
    if (mq_client != (mqd_t)-1) {
        mq_send(mq_client, response, strlen(response) + 1, 0);
        mq_close(mq_client);
    } else {
        perror("Server: mq_open (client)");
    }
}

void register_notification(mqd_t mq, void (*handler)(int, siginfo_t *, void *));

void message_handler(int signum, siginfo_t *info, void *context) {
    mqd_t *mq = info->si_value.sival_ptr;
    char buffer[BUFFER_SIZE];
    unsigned int sender;
    if (mq_receive(*mq, buffer, BUFFER_SIZE, &sender) >= 0) {
        char operation = ' ';
        if (*mq == mq_s) operation = 's';
        else if (*mq == mq_d) operation = 'd';
        else if (*mq == mq_m) operation = 'm';
        
        process_message(buffer, *mq, &operation);
        
        register_notification(*mq, message_handler);
    }
}

void register_notification(mqd_t mq, void (*handler)(int, siginfo_t *, void *)) {
    static struct sigevent sev;
    memset(&sev, 0, sizeof(struct sigevent));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &mq;
    
    if (mq_notify(mq, &sev) == -1) {
        perror("mq_notify");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

int main() {
    signal(SIGINT, handle_sigint);

    pid_t pid = getpid();
    sprintf(queue_name_s, "/%d_s", pid);
    sprintf(queue_name_d, "/%d_d", pid);
    sprintf(queue_name_m, "/%d_m", pid);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_curmsgs = 0;

    mq_s = mq_open(queue_name_s, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_d = mq_open(queue_name_d, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_m = mq_open(queue_name_m, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);

    printf("Queues: %s, %s, %s\n", queue_name_s, queue_name_d, queue_name_m);

    register_notification(mq_s, message_handler);
    register_notification(mq_d, message_handler);
    register_notification(mq_m, message_handler);

    while (1) {
        pause();
    }

    cleanup();
    return 0;
}