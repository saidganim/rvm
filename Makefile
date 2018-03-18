lvm: lvm.c lvm.h
	gcc -g3 -O3 lvm.c -o lvm
	gcc -g3 -O3 thread.c -o thread

clean:
	rm lvm
	rm thread
	rm struct.dump
	rm gdbcmds