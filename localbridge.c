#define USBIP_FROM "/sys/bus/usb/drivers/usbip-host/1-1/usbip_sockfd"
#define USBIP_FROM_BUSNUM 1 /* usbip-host/1-1/busnum */
#define USBIP_FROM_DEVNUM 2 /* usbip-host/1-2/devnum */
#define USBIP_FROM_DEVID ((USBIP_FROM_BUSNUM<<16)|USBIP_FROM_DEVNUM) /* See busnum:devnum */
#define USBIP_TO "/sys/devices/platform/vhci_hcd.0/attach"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


int
main(int ac, char** av){
    int r;
    size_t siz;
    int fds[2];
    int fd_from, fd_to;
    char buf[512];

    fd_from = open(USBIP_FROM, O_WRONLY);
    if(fd_from < 0){
        printf("from open err = %d\n",errno);
        return -1;
    }
    fd_to = open(USBIP_TO, O_WRONLY);
    if(fd_to < 0){
        printf("to open err = %d\n", errno);
        return -1;
    }

    r = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    if(r){
        printf("err = %d\n", errno);
        return -1;
    }

    printf("buflen = %ld\n", sizeof(buf));
    /* Inject socket to FROM */
    siz = snprintf(buf, sizeof(buf), "%d", fds[0]);
    printf("FROMFD = %s\n", buf);
    r = write(fd_from, buf, siz);
    if(r < 0){
        printf("from write err = %d\n", errno);
        return -1;
    }

    /* Inject socket to TO */
    siz = snprintf(buf, sizeof(buf), "%d %d %d %d", 
                   0 /* Port */,
                   fds[1] /* sockfd */,
                   USBIP_FROM_DEVID /* Devid */,
                   3 /* Speed */);
    printf("TOATTACH = %s\n", buf);
    r = write(fd_to, buf, siz);
    if(r < 0){
        printf("to write err = %d\n", errno);
        return -1;
    }

    printf("Done.\n");
    return 0;
}
