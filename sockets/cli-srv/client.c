#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>

char message[] = "Hello therE!\n";
char buf[sizeof(message)] = { 0 };

int main(int argc, char **argv)
{
    int sock, n;
    struct sockaddr_in addr;
    struct addrinfo *res, *t;
    struct addrinfo hints = { 0 };
    char *service, *servername;

    if( argc < 3 ){
        printf("Need server name and port\n");
        exit(1);
    }

    servername = strdup(argv[1]);
    service = strdup(argv[2]);

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    n = getaddrinfo(servername, service, &hints, &res);

    if (n < 0) {
        fprintf(stderr, "%s for %s:%s\n", gai_strerror(n), servername, service);
        free(service);
        printf("ERROR(%s:%d): getaddrinfo failed!\n",__FILE__,__LINE__);
        exit(1);
    }

    for (t = res; t; t = t->ai_next) {
        sock = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
        if (sock >= 0) {
            if (!connect(sock, t->ai_addr, t->ai_addrlen))
                break;
            close(sock);
            sock = -1;
        }
    }

    freeaddrinfo(res);
    free(servername);
    free(service);

    send(sock, message, sizeof(message), 0);

    while( 1 ){
        struct pollfd pfd;
        int timeout = 100; // in milliseconds 
        int rc; 
        pfd.fd = sock;
        pfd.events = POLLIN | POLLHUP;
        if( ( rc = poll(&pfd, 1, timeout) ) < 0 ){
            if( rc == EINTR ){
                rc = 0;
            } else {
                printf("Fatal error in poll: %s:%d\n", __FILE__, __LINE__);
            }
        }
        if( rc > 0 ){
            recv(sock, buf, sizeof(message), 0);
            printf("Success: %s", buf);
            break;
        }
        printf("Do Muxamor JOB\n");
    }    
    close(sock);

    return 0;
}
