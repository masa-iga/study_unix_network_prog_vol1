#ifndef __UTIL__
#define __UTIL__

#include <cstdlib>
#include <unistd.h> // read()
#include <errno.h>
#include <cassert>
#include <cstdarg> // va_start()
#include <syslog.h> // LOG_ERR
#include <cstring> // strlen()
#include <cstdio> // printf()

#define VPRINTF(...) \
    do { \
        printf("%s() L%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)


void err_sys(const char *fmt, ...);
void err_quit(const char *fmt, ...);
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);

int daemon_proc;


ssize_t readn(int fd, void *vptr, size_t n)
{
    assert(false && "not implemented yet.");
    return 0;
}


ssize_t writen(int fd, const void* vptr, size_t n)
{
    size_t nleft = n;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(vptr);

    while (nleft > 0)
    {
        ssize_t nwritten;
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (errno == EINTR)
                nwritten = 0; // try write() again
            else
                return -1; // error
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return 0;
}


/* very slow since read() called as many as maxlen times */
ssize_t readline(int fd, void* vptr, size_t maxlen)
{

    ssize_t n;
    uint8_t* ptr = reinterpret_cast<uint8_t*>(vptr);

    for (n = 1; n < maxlen; n++)
    {
again:
        ssize_t rc;
        uint8_t c;

        const size_t count = 1;
        if ((rc = read(fd, &c, count)) == 1)
        {
            *ptr++ = c;

            if (c == '\n')
                break; // Store return carriage like fgets()
        }
        else if (rc == 0)
        {
            if (n == 1)
                return 0; // EoF, no data
            else
                break;
        }
        else
        {
            if (errno == EINTR)
                goto again;
            
            return -1; // error. read() sets errno
        }
    }

    *ptr = 0; // NULL terminate like fgets()

    return n;
}


/* Fatal error related to a system call.
   Print a message and terminate. */
void err_sys(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    err_doit(1, LOG_ERR, fmt, ap);

    va_end(ap);
    abort();
    exit(1);
}


/* Fatal error unrelated to a system call.
   Print a message and terminate. */
void err_quit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
}


/* Print a message and return to caller.
   Caller specifies "errnoflag" and "level". */
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    const int kMaxLine = 63;

    char buf[kMaxLine+1];

#ifdef HAVE_VSNPRINTF
    vsnprintf(buf, kMaxLine, fmt, ap);
#else
    vsprintf(buf, fmt, ap);
#endif

    int errno_save = errno;
    int n = strlen(buf);

    if (errnoflag)
        snprintf(buf+n, kMaxLine - n, ": %s", strerror(errno_save));

    strcat(buf, "\n");

    if (daemon_proc)
    {
        syslog(level, "%s", buf);
    }
    else
    {
        fflush(stdout); /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
}


#endif // __UTIL__
