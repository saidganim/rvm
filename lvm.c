#include <stdio.h>
#include <stdlib.h>
#include "lvm.h"
#include <string.h>


pt_entry_t *pg_high;

char *physmem; // physical memory will be stored here
vma_t *first_vma;
vma_t *current_vma;

int main(void){
	void* empty_buffer = malloc(sizeof(char) * 512);
	physmem = malloc(PHYMEM_SIZE);
	current_vma = first_vma = (vma_t*)malloc(sizeof(vma_t));
	memset(first_vma, 0x0, sizeof(vma_t));
	FILE* file = fopen("physmem", "rb");
	for(int cursor = 0; cursor < PHYMEM_SIZE; cursor += 512){
		// printf("BUFFER READ %d\n", cursor);
		fread(physmem + cursor, sizeof(char), min(512, PHYMEM_SIZE - cursor), file);
	}
	fclose(file);
	// memset(virmem, PHYMEM_SIZE, 0x0);
	// printf("FIRST COPY , physadd: %x\n", physmem);
	// first_vma->data = malloc(KB4);
	// memcpy(first_vma->data, (void*)physmem + 2097152, KB4);
	pg_high = physmem + KPT;
	//pg_high[0] = 2465827;
	// walking through high page table (Recursion is evil)
	for(pt_entry_t *pt = pg_high; pt < pg_high + NO_OF_PT_ENTRIES; ++pt){
		if(!(*pt & X86_MMU_PG_P))
			continue; // current range is not presented
			printf("THE FIRST LEVEL\n");
		printf("I AM HERE??? %d \n", pt - pg_high);

		if(X86_MMU_PG_PS & *pt){ // Huge page (512GB)
			// printf("Copying something 512GB\n");
		*pt = *pt & ~0xFFF;

		printf("Higher table entry %d \n", *pt);

			current_vma->data = malloc(sizeof(char) * GB512);
			memset(current_vma->data, 0x0, GB512);
			current_vma->base = ((uint64_t)pt - (uint64_t)pg_high) * GB512;
			current_vma->size = GB512;
			current_vma->next = malloc(sizeof(vma_t));
			memset(current_vma->next, 0x0, sizeof(vma_t));
			memcpy(current_vma->data, physmem + *pt, GB512); // Copy one huge page(512GB) - absolutely useless for our 512MB memory :)
			current_vma= current_vma->next;
			
			continue;
		}
		*pt = *pt & ~0xFFF;

		for(pt_entry_t *pt2 = (void*)physmem + *pt; pt2 < (void*)physmem + *pt + NO_OF_PT_ENTRIES; ++pt2){
			// The second level table
			printf("THE SECOND LEVEL\n");
			if(!(*pt2 & X86_MMU_PG_P))
				continue; // current range is not presented
			printf("WOOOW\n");

			if(X86_MMU_PG_PS & *pt2){ // Huge page (1GB)
				// printf("Copying something 1GB\n");
			*pt2 = *pt2 & ~0xFFF;

			printf("Middle table entry %d \n", (uint64_t)(pt2 - (pt_entry_t*)((void*)physmem + *pt)));

				current_vma->data = malloc(sizeof(char) * GB1);
				memset(current_vma->data, 0x0, GB1);
				current_vma->base = ((uint64_t)pt - (uint64_t)pg_high) * GB512 + ((uint64_t)pt2 - (uint64_t)physmem - *pt) * GB1;
				current_vma->size = GB1;
				current_vma->next = malloc(sizeof(vma_t));
				memset(current_vma->next, 0x0, sizeof(vma_t));
				memcpy(current_vma->data, physmem  + *pt2, GB1); // Copy one huge page(1GB) - also absolutely useless for our 512MB memory :)
				current_vma= current_vma->next;
				
				continue;
			}
			*pt2 = *pt2 & ~0xFFF;

			for(pt_entry_t *pt3 = physmem + *pt2; pt3 < (void*)physmem + *pt2 + NO_OF_PT_ENTRIES; ++pt3){
				// The third level of the page table
				
				if(!(*pt3 & X86_MMU_PG_P))
					continue; // current range is not presented in memory
				
			printf("THE THIRD LEVEL\n");
				if(X86_MMU_PG_PS & *pt3){ // Huge page (2MB)
					// printf("Copying something 2MB\n");
					*pt3 = *pt3 & ~0xFFF;
				printf("Directory table entry %d \n", (uint64_t)(pt3 - (pt_entry_t*)(physmem + *pt2)));

					current_vma->data = malloc(sizeof(char) * MB2);
					memset(current_vma->data, 0x0, MB2);
					current_vma->base = ((uint64_t)pt - (uint64_t)pg_high) * GB512 + ((uint64_t)pt2 - (uint64_t)physmem - *pt) * GB1 + ((uint64_t)pt3 - (uint64_t)physmem -*pt2) * MB2;
					current_vma->size = MB2;
					current_vma->next = malloc(sizeof(vma_t));
					memset(current_vma->next, 0x0, sizeof(vma_t));
					
					memcpy(current_vma->data, physmem  + *pt3, MB2); // Copy one huge page(2MB)
					current_vma= current_vma->next;
					continue;
				}
				*pt3 = *pt3 & ~0xFFF;

				for(pt_entry_t *pt4 = (void*)physmem + *pt3; pt4 < physmem + *pt3 + NO_OF_PT_ENTRIES; ++pt4){
					printf("The fourth level 0x%x\n", *pt3);
					// Finally the last level :)
					if(!(*pt4 & X86_MMU_PG_P))
						continue; // current range is not presented in memory
					*pt4 = *pt4 & ~0xFFF;

					printf("Table entry %d , physadd: %x, MAX is %x \n", (uint64_t)(pt4 - (pt_entry_t*)((void*)physmem + *pt3)), *pt4, PHYMEM_SIZE);
					// printf("Copying something\n");
					current_vma->data = malloc(sizeof(char) * KB4);
					memset(current_vma->data, 0x0, KB4);
					current_vma->base = ((uint64_t)pt - (uint64_t)pg_high) * GB512 + ((uint64_t)pt2 - (uint64_t)physmem - *pt) * GB1 + ((uint64_t)pt3 - (uint64_t)physmem -*pt2) * MB2 + ((uint64_t)pt4 - (uint64_t)physmem - *pt3) * KB4;
					current_vma->size = KB4;
					current_vma->next = malloc(sizeof(vma_t));
					memset(current_vma->next, 0x0, sizeof(vma_t));
					memcpy(current_vma->data, (void*)(physmem) + *pt4, KB4); // Copy one small page (4KB)
					current_vma = current_vma->next;
					continue;
				}
				// goto label;
			// } goto label;
		}
	} 
	// END OF WALKING PGDIRS 
	// printf("dumping\n");
	// FILE* output = fopen("virtmem_dump", "w");
	// for(int cursor = 0; cursor < PHYMEM_SIZE; cursor += 512){
	// 	// printf("BUFFER WRITTEN %d, size == %d\n", cursor, min(512, PHYMEM_SIZE - cursor));
	// 	fwrite(virmem + cursor, sizeof(char), min(512, PHYMEM_SIZE - cursor), output);
	// }
	// fclose(output);
	label:

	while(first_vma && first_vma->data){
		printf("\n================================VMA { BASE:0x%x, SIZE:%d} DATA:=====================================\n", first_vma->base, first_vma->size);
		for(int cursor = 0; cursor < PHYMEM_SIZE; cursor += 512){
			if(!memcmp(first_vma->data + cursor, empty_buffer, min(512, PHYMEM_SIZE - cursor)) )
				continue;
	 		write(STDOUT_FILENO, first_vma->data + cursor, min(512, PHYMEM_SIZE - cursor));
	 		printf("\n");
	 	}
	 	first_vma = first_vma->next;
	}
	return 0;
}
