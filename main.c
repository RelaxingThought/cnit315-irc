
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 6667
/* Has to be >= 512 per IRC spec */
#define RECV_BUF_LEN 1024

void irc_send(int, int, ...);
char *example_handler_function(const char *message);

char *address = "irc.freenode.net";
char *channel = "#cnit315_bot_test";
char *nickname = "test_bot_569";

int main(int argc, char *argv[]) {
    int sock, numbytes;
    char *message;
    char *message_out;
    int buf_index = 0;
    int buf_remain;
    char buf[RECV_BUF_LEN + 1]; /* +1 just in case, I think its possible to overrun */
    char save;
    struct hostent *host;
    struct sockaddr_in remote_addr;

    if ((host = gethostbyname(address)) == NULL) {
        perror("gethostbyname()");
        exit(1);
    } else {
        printf("Client-The remote host is: %s\n", address);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        exit(1);
    } else {
        printf("Client-The socket() sock is OK...\n");
    }

    remote_addr.sin_family = AF_INET;
    printf("Connecting to %s : %d\n", address, PORT);

    remote_addr.sin_port = htons(PORT);
    remote_addr.sin_addr = *((struct in_addr *)host->h_addr);

    memset(&(remote_addr.sin_zero), 0, 8);

    if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect()");
        exit(1);
    } else {
        printf("Client-The connect() is OK...\n");
    }

    /* For now, I just assume send() works, we can change this if we want */
    irc_send(sock, 4, "USER ", nickname, " 0 * :", nickname);
    irc_send(sock, 2, "NICK ", nickname);
    irc_send(sock, 2, "JOIN ", channel);

    while (1) {
        buf_remain = RECV_BUF_LEN - buf_index;

        if ((numbytes = recv(sock, buf + buf_index, buf_remain, 0)) == -1) {
            perror("recv()");
            exit(1);
        }
        buf_index += numbytes;

        while ((message = strchr(buf, '\n')) != NULL) {
            /* OVERLOADING numbytes */
            numbytes = message - buf + 1;
            printf("%i\n", numbytes);

            save = buf[numbytes];
            buf[numbytes] = '\0';
            printf(">> %s", buf);

            if (strncmp(buf, "PING", 4) == 0) {
                irc_send(sock, 2, "PONG ", &buf[5]);
            }

            if ((message = strchr(buf, ' ')) != NULL) {
                printf("%s", message);
                if (strncmp(message + 1, "376", 3) == 0) {
                    irc_send(sock, 2, "JOIN ", channel);
                }
            }

            if ((message = strchr(&buf[1], ':')) != NULL) {

                /* This is the code that hooks into your handlers */
                /* See below for implementation of example */

                message_out = example_handler_function(message + 1);
                if (message_out != NULL) {
                    irc_send(sock, 4, "PRIVMSG ", channel, " :", message_out);
                }
                free(message_out);

                /* repeat for each handler */
            }

            buf[numbytes] = save;
            memmove(buf, buf + numbytes, RECV_BUF_LEN - numbytes);
            buf_index -= numbytes;

            memset(buf + buf_index, 0, RECV_BUF_LEN - buf_index);
        }
    }

    printf("Client-Closing sock\n");
    close(sock);
    return 0;
}

void irc_send(int sock, int count, ...) {
    int i;
    va_list v;

    va_start(v, count);
    printf("<< ");

    for (i=0; i<count; i++) {
        char *buf = va_arg(v, char *);
        /* TODO Error check this */
        send(sock, buf, strlen(buf), 0);
        printf("%s", buf);
    }
    va_end(v);

    send(sock, "\r\n", 2, 0);
    printf("\n");
}

/*
 * Following is an example of the kind of 'main' function you should
 * write for your parts.  You can have as many helper functions called
 * from this function as you want.
 *
 * Please do your part in a seperate .c file in this directory,
 * we can link it all together and everything will be more organized.
 *
 * This is the only function you need to worry about, this this same
 * funtion signature,
 *
 * char *xyz_handler(const char *message)
 *
 * and be sure to return a point your got from malloc.
 * you won't be able to return local strings anyway, so just standardizing
 * on a malloc()'d string makes it easier for me to free the memory and
 * lowers our chances of leaking like the Titanic.
 *
 * Do let me know if you have any questions, comments, or concerns.
 *
 * -Tyler
 *
 */

char *example_handler_function(const char *message) {
    char *ret = NULL;
    if (strncmp(message, "~test", 5) == 0) {
        ret = malloc(512);
        strcpy(ret, "You did ~test!");
    }
    return ret;
}

