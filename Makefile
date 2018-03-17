lvm: lvm.c lvm.h
	gcc -O3 lvm.c -o lvm

clean:
	rm lvm
	rm virtmem_dump