int syscall(int svc_num, void *in, void *out) {
    asm volatile(
        "syscall":
        :"b"fd.val
        :"c"count
        :"d"out
        :"=a"(k)
    );
}
