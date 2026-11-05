#ifndef syscall
    #include "syscall.h"
#endif

struct FD {
    int val;
    char data;
};

#define FD_RO (char)0x01
#define FD_WO (char)0x10
#define FD_WR FD_RO+FD_WO

#define stdin struct FD {.val=3,.data=FD_RO}
#define stdout struct FD {.val=1,.data=FD_WO}
#define stderr struct FD {.val=2,.data=FD_WO}

int read(struct FD fd, unsigned int count, char *out) {
    if (fd.val&&FD_RO==0) {
        //file not readable error
        return -1;
    }
    unsigned int k=0;
    syscall(read_svc,(void *)[fd.val,count,out],(void *)[k]);
    return k
}

int write(struct FD fd, unsigned int count, unsigned int offset, char *out) {

}

struct FD open() {}
int close()

int fseek()