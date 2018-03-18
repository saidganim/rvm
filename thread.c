#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "lvm.h"

char memory[2 * GB1];

extern const char *__progname;

static void printout_struct(const void* invar, const char* structname){
    char dbx[160];
    sprintf(dbx, "echo 'p (struct %s)*%p\n' > gdbcmds", structname, invar );
    system(dbx);
    sprintf(dbx, "echo 'where\ndetach' | gdb -batch --command=gdbcmds %s %d > struct.dump", __progname, getpid() );
    system(dbx);
    sprintf(dbx, "cat struct.dump");
    system(dbx);
    return;
}


int main(void){
	FILE* file = fopen("output", "rb");
	int magic = THREAD_MAGIC;
	size_t size = fread(memory, sizeof(char), GB1 + MB2 * 52, file);

	for(int i = 0; i < size; i += 4){
		if(!memcmp(memory + i, &magic, sizeof(int))){
			thread_t *thread = ((void*)memory + i);
			printout_struct(thread, "zx_thread");
		}
	}
	return 0;

}