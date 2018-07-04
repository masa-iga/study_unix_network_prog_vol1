#include <sys/types.h> // open()
#include <sys/stat.h> // open()
#include <fcntl.h> // open()
#include <sys/socket.h>

#include "util.h"

ssize_t write_fd(int, char*, size_t, int);

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        err_quit("openfile <sockfd#> <filename> <mode>");
    }

    int fd;
    if ((fd = open(argv[2], atoi(argv[3]))) < 0)
    {
        exit((errno > 0) ? errno : 255);
    }

    ssize_t n;
    char c = 'a';
    if ((n = write_fd(atoi(argv[1]), &c, 1, fd)) < 0)
    {
        exit((errno > 0) ? errno : 255);
    }

    VPRINTF("sendfd %d\n", fd);

    exit(0);
}


ssize_t write_fd(int fd, char* cptr, size_t nbytes, int sendfd)
{
    void* ptr = reinterpret_cast<char*>(cptr);

    union {
        struct cmsghdr  cm;
        char            control[CMSG_SPACE(sizeof(int))];
    } control_un;

    struct msghdr   msg;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    struct cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int*)CMSG_DATA(cmptr)) = sendfd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    struct iovec iov[1];
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return (sendmsg(fd, &msg, 0));
}
