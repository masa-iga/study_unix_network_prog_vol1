#include <cstdio>
#include <unistd.h> // read(), write()
#include <fcntl.h> // O_RDONLY
#include <sys/types.h>
#include <sys/socket.h>

#include "util.h"

static const int kBuffSize = 256;
static const char* const kPath = "14_2_domain_bind.cpp";

int my_open(const char*, int);
int read_fd(int, void*, size_t, int*);

int main()
{

    int fd, n;
    char buff[kBuffSize];

    if ((fd = my_open(kPath, O_RDONLY)) < 0)
    {
        VPRINTF("error. cannot open %s\n", kPath);
    }

    while ((n = read(fd, buff, kBuffSize)) > 0)
    {
        write(STDOUT_FILENO, buff, n);
    }
}


int my_open(const char* pathname, int mode)
{
    int sockfd[2];
    socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);

    pid_t childpid;

    if ((childpid = fork()) == 0)
    {
        close(sockfd[0]);

        char argsockfd[10], argmode[10];

        snprintf(argsockfd, sizeof(argsockfd), "%d", sockfd[1]);
        snprintf(argmode, sizeof(argmode), "%d", mode);

        execl("./openfile", "openfile", argsockfd, pathname, argmode, (char*)NULL);

        err_sys("execl error");
    }

    /* Parent process: wait until child process ends */
    close(sockfd[1]);

    int status;
    waitpid(childpid, &status, 0 /* options */);

    if (WIFEXITED(status) == 0) // return true if child process ends without any problem
    {
        err_quit("child id not terminate");
    }

    int fd;
    char c;

    if ((status = WEXITSTATUS(status)) == 0) // WEXITSTATUS() returns exit status of child process
    {
        read_fd(sockfd[0], &c, 1, &fd); // todo: implement
    }
    else
    {
        errno = status;
        fd = -1;
    }

    close(sockfd[0]);
    return(fd);
}


int read_fd(int fd, void* ptr, size_t nbytes, int* recvfd)
{

    union {
        struct cmsghdr  cm;
        char            control[CMSG_SPACE(sizeof(int))];
    } control_un;

    struct msghdr msg;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    struct iovec iov[1];
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    ssize_t n;
    if ((n = recvmsg(fd, &msg, 0)) <= 0)
    {
        return (n);
    }

    struct cmsghdr* cmptr;
    if ((cmptr = CMSG_FIRSTHDR(&msg)) == NULL || cmptr->cmsg_len != CMSG_LEN(sizeof(int)))
    {
        *recvfd = 1;
    }

    if (cmptr->cmsg_level != SOL_SOCKET)
    {
        err_quit("control level != SOL_SOCKET");
    }

    if (cmptr->cmsg_type != SCM_RIGHTS)
    {
        err_quit("control type != SCM_RIGHTS");
    }

    *recvfd = *((int*)CMSG_DATA(cmptr));

    VPRINTF("recvfd %d\n", *recvfd);

    return (n);
}
