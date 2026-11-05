killall -KILL qemu-system-x86_64
make all&&
qemu-system-x86_64 -drive file=os.img,format=raw,id=disk0 -s&