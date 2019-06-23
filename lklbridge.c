#define USBIP_FROM "/sys/bus/usb/drivers/usbip-host/1-1/usbip_sockfd"
#define USBIP_FROM_BUSNUM 1 /* usbip-host/1-1/busnum */
#define USBIP_FROM_DEVNUM 3 /* usbip-host/1-1/devnum */
#define USBIP_FROM_DEVID ((USBIP_FROM_BUSNUM<<16)|USBIP_FROM_DEVNUM) /* See busnum:devnum */

/* LKL: /sys => /sysfs */
#define USBIP_TO "/sysfs/devices/platform/vhci_hcd.0/attach"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <lkl.h>
#include <lkl_host.h>

#define FDBRIDGE_BUFSIZE (1024*1024)
typedef struct {
    int from;
    int to;
} fdbridge_param_t;

static void*
thr_lkl_to_host(void* arg){
    int r;
    void* buf;
    int from;
    int to;
    fdbridge_param_t* param = (fdbridge_param_t*)arg;
    buf = malloc(FDBRIDGE_BUFSIZE);
    from = param->from;
    to = param->to;

    for(;;){
        r = lkl_sys_read(from, buf, FDBRIDGE_BUFSIZE);
        //printf("LKL=>HOST: %d bytes\n", r);
        if(r<0){
            printf("ERR\n");
            return 0;
        }else{
            r = write(to, buf, r);
            if(r<0){
                printf("HOST WRITE ERR: %d\n",errno);
            }
        }
    }
}

static void*
thr_host_to_lkl(void* arg){
    int r;
    void* buf;
    int from;
    int to;
    fdbridge_param_t* param = (fdbridge_param_t*)arg;
    buf = malloc(FDBRIDGE_BUFSIZE);
    from = param->from;
    to = param->to;

    for(;;){
        r = read(from, buf, FDBRIDGE_BUFSIZE);
        //printf("HOST=>LKL: %d bytes\n", r);
        if(r<0){
            printf("ERR\n");
            return 0;
        }else{
            r = lkl_sys_write(to, buf, r);
            if(r<0){
                printf("LKL WRITE ERR: %d\n", r);
            }
        }
    }
}

int /* -errno */
lkl_socketpair_unix(int out[2]){
    long params[6];
    long r;

    out[0] = 1234;
    out[1] = 5678;

    params[0] = AF_UNIX; /* family(int) */
    params[1] = SOCK_STREAM; /* type(int) */
    params[2] = 0; /* protocol(int) */
    params[3] = (long)(&out[0]); /* usockvec(int*) */
    params[4] = 0;
    params[5] = 0;

    r = lkl_syscall(199 /* Socketpair*/ , params);

    printf("r = %ld\n",r);
    printf("out0 = %d\n",out[0]);
    printf("out1 = %d\n",out[1]);

    return r;
}

int
main(int ac, char** av){
    int r;
    size_t siz;
    int fds[2];
    int fd_from, fd_to;
    char buf[512];

    int lkl_fds[2];
    fdbridge_param_t pipe0;
    fdbridge_param_t pipe1;
    pthread_t thr_pipe0;
    pthread_t thr_pipe1;

    /* LKL: Start Kernel */
    lkl_start_kernel(&lkl_host_ops, "mem=16M");
    r = lkl_mount_fs("sysfs");
    printf("Mount sysfs = %d\n", r);
    r = lkl_socketpair_unix(lkl_fds);
    if(r){
        printf("LKL socketpair err = %d\n", r);
    }

    fd_from = open(USBIP_FROM, O_WRONLY);
    if(fd_from < 0){
        printf("from open err = %d\n",errno);
        return -1;
    }

    /* LKL: Use LKL call instead */
    fd_to = lkl_sys_open(USBIP_TO, LKL_O_WRONLY, 0);
    if(fd_to < 0){
        printf("LKL to open err = %d\n", errno);
        return -1;
    }

    r = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    if(r){
        printf("err = %d\n", errno);
        return -1;
    }

    /* LKL: Prepare pipe */
    pipe0.from = lkl_fds[0];
    pipe0.to = fds[1];
    pipe1.from = fds[1];
    pipe1.to = lkl_fds[0];
    r = pthread_create(&thr_pipe0, NULL, thr_lkl_to_host, &pipe0);
    if(r){
        printf("pipe0 err = %d\n",r);
        return -1;
    }
    r = pthread_create(&thr_pipe1, NULL, thr_host_to_lkl, &pipe1);
    if(r){
        printf("pipe1 err = %d\n",r);
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
                   lkl_fds[1] /* sockfd */,
                   USBIP_FROM_DEVID /* Devid */,
                   3 /* Speed */);
    printf("TOATTACH = %s\n", buf);
    r = lkl_sys_write(fd_to, buf, siz);
    if(r < 0){
        printf("LKL to write err = %d\n", r);
        return -1;
    }

    printf("Done.\n");
    for(;;){
        sleep(1);
    }
    return 0;
}
