/* file server */

#include "http.h"
#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
int main(int argc, char **argv)
{
    int fd;
    if (argc != 2 && argc != 4)
        errx(1, "Wrong arguments");
    fd = atoi(argv[1]);

    if (argc == 4) {
        int uid = atoi(argv[2]);
        int gid = atoi(argv[3]);
        warnx("cgi uid %d, gid %d", uid, gid);
        http_set_executable_uid_gid(uid, gid);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    for (;;)
    {
        char envp[4096];
        int sockfd = -1;
	size_t len;
        const char *errmsg;

        /* receive socket and envp from zookd */
        len = recvfd(fd, envp, sizeof(envp), &sockfd);
        if ( len <= 0 || sockfd < 0)
            err(1, "recvfd");

        switch (fork())
        {
        case -1: /* error */
            err(1, "fork");
        case 0:  /* child */
            /* set envp */
            env_deserialize(envp, len, sizeof(envp));
            /* get all headers */
            if ((errmsg = http_request_headers(sockfd)))
                http_err(sockfd, 500, "http_request_headers: %s", errmsg);
            else
            {   if(getenv("REQUEST_URI") == NULL)
                  http_serve(sockfd, "");
                else
                  http_serve(sockfd, getenv("REQUEST_URI"));
            }
            return 0;
        default: /* parent */
            close(sockfd);
            break;
        }
    }
}
