#include <sys/socket.h>
#include <unistd.h> // unlink()
#include <cstring> // bzero()
#include <sys/un.h> // struct sockaddr_un
#include <cstdio>

#define VPRINTF(...) \
    do { \
        printf("%s() L%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)


/* Hello world of UNIX domain protocol */
int main()
{
    const char* path = "14_2_domain_socket";

    int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0 /* protocol */);
    unlink(path);

    struct sockaddr_un  addr1, addr2;

    bzero(&addr1, sizeof(addr1)); // fill with zero data
    addr1.sun_family = AF_LOCAL;
    strncpy(addr1.sun_path, path, sizeof(addr1.sun_path)-1); // Minus one ensures NULL terminate

    //bind(sockfd, (SA*)&addr1, SUN_LEN(&addr1));
    //bind(sockfd, reinterpret_cast<sockaddr*>(&addr1), SUN_LEN(&addr1));
    bind(sockfd, reinterpret_cast<sockaddr*>(&addr1), sizeof(struct sockaddr_un));

    socklen_t len = sizeof(addr2);

    getsockname(sockfd, reinterpret_cast<sockaddr*>(&addr2), &len);

    VPRINTF("bound name: %s\n", addr2.sun_path);
    VPRINTF("returned len: %d\n", len);

    return 0;
}
