#include <sys/types.h> // socket()
#include <sys/socket.h>
#include <sys/un.h> // struct sockaddr_un
#include <cstring> // bzero()
#include <cstdio>
#include <algorithm> // std::max

#include "util.h"

using namespace std;

const char* kPath = "/tmp/14_3_stream_sd";
const int kMaxLine = 4;


void str_cli(FILE *fp, int sockfd)
{
    char sendline[kMaxLine], recvline[kMaxLine];

    fd_set  rset;
    FD_ZERO(&rset);

    int stdineof = 0;
    for ( ; ; )
    {
        if (stdineof == 0)
        {
            FD_SET(fileno(fp), &rset);
        }

        FD_SET(sockfd, &rset);

        int maxfdp1 = max(fileno(fp) /* return fd from strem */, sockfd) + 1;

        select(maxfdp1 /* nfds */, &rset /* readfds */, NULL /* writefds */,
                NULL /*exceptfds */, NULL /* timeout */);

        if (FD_ISSET(sockfd, &rset)) // socket is ready to read
        {
            if (readline(sockfd, recvline, kMaxLine) == 0)
            {
                if (stdineof == 1)
                {
                    return; /* exit */
                }
                else
                {
                    VPRINTF("str_cli: server terminated prematurely\n");
                    assert(false);
                }
            }

            fputs(recvline, stdout);
        }

        if (FD_ISSET(fileno(fp), &rset)) // fp is ready to read
        {
            if (fgets(sendline, kMaxLine, fp) == NULL)
            {
                stdineof = 1;
                shutdown(sockfd, SHUT_WR); /* send FIN */
                FD_CLR(fileno(fp), &rset);
                continue;
            }

            writen(sockfd, sendline, strlen(sendline));
        }

    }

}


int main()
{

    int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0 /* protocol */);

    struct sockaddr_un servaddr;
    bzero(&servaddr, sizeof(servaddr));

    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, kPath);

    VPRINTF("path: %s\n", kPath);

    connect(sockfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr));

    str_cli(stdin, sockfd);

    return 0;
}
