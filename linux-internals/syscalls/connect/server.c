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

#define SLURM_ERROR -1
#define PMIXP_ERROR_STD printf

int pmixp_usock_create_srv(char *path)
{
	static struct sockaddr_un sa;
	int ret = 0;

	if (strlen(path) >= sizeof(sa.sun_path)) {
		PMIXP_ERROR_STD("UNIX socket path is too long: %lu, max %lu",
				(unsigned long) strlen(path),
				(unsigned long) sizeof(sa.sun_path) - 1);
		return SLURM_ERROR;
	}

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		PMIXP_ERROR_STD("Cannot create UNIX socket");
		return SLURM_ERROR;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, path);
	if ((ret = bind(fd, (struct sockaddr *)&sa, SUN_LEN(&sa)))) {
		PMIXP_ERROR_STD("Cannot bind() UNIX socket %s", path);
		goto err_fd;
	}

	if ((ret = listen(fd, 64))) {
		PMIXP_ERROR_STD("Cannot listen(%d, 64) UNIX socket %s", fd,
				path);
		goto err_bind;

	}
	return fd;

err_bind:
	unlink(path);
err_fd:
	close(fd);
	return ret;
}

int main()
{
    int lsd = pmixp_usock_create_srv("./test.usock");
    while(1){
        int sd = accept(lsd, NULL, 0);
        printf("sd = %d\n", sd);
        close(sd);
    }
    return 0;
}