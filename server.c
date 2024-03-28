#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mqueue.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define QUEUE_PERMISSIONS 0660
#define BUFFER_SIZE 128

mqd_t mq_s, mq_d, mq_m;

mqd_t create_queue(const char *name) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 100;
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(name, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    if (mq == (mqd_t)-1) {
        perror("Queue creation failed");
        exit(1);
    }
    return mq;
}

void destroy_queue(mqd_t mq, const char *name) {
    mq_close(mq);
    mq_unlink(name);
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

void cleanup() {
    mq_close(mq_s);
    mq_close(mq_d);
    mq_close(mq_m);

    char queue_name[20];
    sprintf(queue_name, "/%d_s", getpid());
    mq_unlink(queue_name);

    sprintf(queue_name, "/%d_d", getpid());
    mq_unlink(queue_name);

    sprintf(queue_name, "/%d_m", getpid());
    mq_unlink(queue_name);
}

void handle_sigint(int sig) {
    cleanup();
    exit(0);
}

int main() {
    struct mq_attr attr;
    char buffer[BUFFER_SIZE];
    unsigned int sender;

    signal(SIGINT, handle_sigint);

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_curmsgs = 0;

    pid_t pid = getpid();
    char queue_name_s[20], queue_name_d[20], queue_name_m[20];
    sprintf(queue_name_s, "/%d_s", pid);
    sprintf(queue_name_d, "/%d_d", pid);
    sprintf(queue_name_m, "/%d_m", pid);

    mq_s = mq_open(queue_name_s, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_d = mq_open(queue_name_d, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);
    mq_m = mq_open(queue_name_m, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);

    printf("Queues: %s, %s, %s\n", queue_name_s, queue_name_d, queue_name_m);

    fd_set readset;
    int result, highest_fd = mq_s > mq_d ? mq_s : mq_d;
    highest_fd = highest_fd > mq_m ? highest_fd : mq_m;

    while (1) {
        FD_ZERO(&readset);
        FD_SET(mq_s, &readset);
        FD_SET(mq_d, &readset);
        FD_SET(mq_m, &readset);

        result = select(highest_fd + 1, &readset, NULL, NULL, NULL);

        if (result > 0) {
            if (FD_ISSET(mq_s, &readset)) {
                if (mq_receive(mq_s, buffer, BUFFER_SIZE, &sender) >= 0) {
                    process_message(buffer, mq_s, "sum");
                } else {
                    perror("Server: mq_receive from sum queue");
                }
            }
            if (FD_ISSET(mq_d, &readset)) {
                if (mq_receive(mq_d, buffer, BUFFER_SIZE, &sender) >= 0) {
                    process_message(buffer, mq_d, "div");
                } else {
                    perror("Server: mq_receive from div queue");
                }
            }
            if (FD_ISSET(mq_m, &readset)) {
                if (mq_receive(mq_m, buffer, BUFFER_SIZE, &sender) >= 0) {
                    process_message(buffer, mq_m, "mod");
                } else {
                    perror("Server: mq_receive from mod queue");
                }
            }
        } else if (result == -1 && errno != EINTR) {
            perror("Server: select");
            break;
        }
    }

    cleanup();
    return 0;
}
