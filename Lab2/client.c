#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#define QUEUE_PERMISSIONS 0660
#define BUFFER_SIZE 128
#define TIMEOUT 1000

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_queue_name>\n", argv[0]);
        exit(1);
    }

    struct mq_attr attr;
    mqd_t mq_server, mq_client;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    int num1, num2;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFFER_SIZE;
    attr.mq_curmsgs = 0;

    char client_queue_name[20];
    sprintf(client_queue_name, "/%d", getpid());
    mq_client = mq_open(client_queue_name, O_CREAT | O_RDONLY, QUEUE_PERMISSIONS, &attr);

    mq_server = mq_open(argv[1], O_WRONLY);
    if (mq_server == (mqd_t)-1) {
        perror("Client: mq_open (server)");
        mq_unlink(client_queue_name);
        exit(1);
    }

    while (scanf("%d %d", &num1, &num2) != EOF) {
        sprintf(message, "%d %d %d", getpid(), num1, num2);
        mq_send(mq_server, message, strlen(message) + 1, 0);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT * 1000;
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(mq_client, &readset);

        int activity = select(mq_client + 1, &readset, NULL, NULL, &tv);

        if (activity > 0) {
            if (mq_receive(mq_client, buffer, BUFFER_SIZE, NULL) >= 0) {
                printf("Result: %s\n", buffer);
            } else {
                perror("Client: mq_receive");
                break;
            }
        } else if (activity == 0) {
            printf("Client: Response timeout\n");
            break;
        } else {
            perror("Client: select");
            break;
        }
    }

    mq_close(mq_server);
    mq_close(mq_client);
    mq_unlink(client_queue_name);

    return 0;
}