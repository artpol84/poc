#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

extern int errno;
#include <errno.h>



static int usock_connect(char *path)
{
    struct sockaddr_un address;
    int sd=-1, rc;
    socklen_t addrlen = sizeof(address);

    /* setup the path to the daemon rendezvous point */
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, sizeof(address.sun_path)-1, "%s", path);
    /* Create the new socket */
    sd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sd < 0) {
        printf("Error, can't connect to socket\n");
        abort();
    }

    /* try to connect */
    while( (rc = connect(sd, (struct sockaddr*)&address, addrlen)) < 0 && (errno == EAGAIN) );

    return rc;
}

int main()
{
    int sd = usock_connect("./test.usock");
    printf("sd = %d\n", sd);
    return 0;
}