#include <sys/types.h> // socket()
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h> // struct sockaddr_un
#include <signal.h>
#include <unistd.h> // unlink()
#include <errno.h> // EINTR
#include <cstring> // bzero()
#include <cstdio> // perror()
#include <cstdlib> // exit(9

#include "util.h"

const char* kPath = "/tmp/14_3_stream_sd";

/* Catch a child process death, and print the info */
void sig_chld(int signo)
{
    int stat;
    pid_t pid = wait(&stat);

    VPRINTF("child %d terminated\n", pid);
}


/*
   sigcation() defines how to handle signal, but quit complicated to call sigcation().
   For easy to use, provide mysignal() wrapper func.
 */
typedef void Sigfunc(int); // to simplify

// void (*mysignal(int signo, void (*func)(int)))(int)
Sigfunc *mysignal(int signo, Sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask); // no signals blocked
    act.sa_flags = 0;

    if (signo == SIGALRM)
    {
#ifdef SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;
#endif // SA_INTERRUPT
    }
    else
    {
        act.sa_flags |= SA_RESTART;
    }

    if (sigaction(signo, &act, &oact) < 0)
        return(SIG_ERR);

    return(oact.sa_handler);
}


void str_echo(int sockfd)
{
    const int kMaxline = 10;

    for ( ; ; )
    {
        ssize_t n;
        char line[kMaxline];
        if ((n = readline(sockfd, line, kMaxline)) == 0) // todo: imple readline()
            return; // connection closed at the other end

        writen(sockfd, line, n);
    }
}


int main()
{
    const int listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    unlink(kPath);

    struct sockaddr_un  servaddr;

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, kPath);

    bind(listenfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr));

    const int backlog = 10;

    if (listen(listenfd, backlog) < 0)
    {
        perror("listen() failed.");
    }

    /* Prevent child process being a zombie */
    void sig_chld(int);
    mysignal(SIGCHLD, sig_chld);

    for ( ; ; )
    {
        struct sockaddr_un  cliaddr;
        socklen_t clilen = sizeof(cliaddr);

        int connfd;
        if ((connfd = accept(listenfd, reinterpret_cast<sockaddr*>(&cliaddr), &clilen)) < 0)
        {
            if (errno == EINTR) /* Interrupt happened */
                continue;
            else
                perror("accept error");
        }

        /* child process */
        pid_t childpid;
        if ((childpid = fork()) == 0)
        {
            close(listenfd);
            str_echo(connfd);
            exit(0);
        }

        close(connfd);
    }

    return 0;
}
