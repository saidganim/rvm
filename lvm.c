#include <stdio.h>
#include <stdlib.h>
#include "lvm.h"
#include <string.h>
#include <inttypes.h>

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
	pg_high = (pt_entry_t*)(physmem + KPT);
	//pg_high[0] = 2465827;
	// printf("my_number: %#018" PRIx64 "\n",  GB512 * 511 + GB1 * 511 + MB2*511 + KB4*511);
	// walking through high page table (Recursion is evil)
	for(pt_entry_t *pt = pg_high; pt < pg_high + NO_OF_PT_ENTRIES; ++pt){
		if(!(*pt & X86_MMU_PG_P))
			continue; // current range is not presented

		if(X86_MMU_PG_PS & *pt){ // Huge page (512GB)
		*pt = *pt & ~0xFFF;
		current_vma->data = malloc(sizeof(char) * GB512);
		memset(current_vma->data, 0x0, GB512);
		current_vma->base = (uint64_t)(pt - pg_high) * GB512;
		current_vma->size = GB512;
		current_vma->next = malloc(sizeof(vma_t));
		memset(current_vma->next, 0x0, sizeof(vma_t));
		memcpy(current_vma->data, physmem + *pt, GB512); // Copy one huge page(512GB) - absolutely useless for our 512MB memory :)
		current_vma= current_vma->next;
		current_vma->next = NULL;
		current_vma->data = NULL;
		continue;
		}
		*pt = *pt & ~0xFFF;

		for(pt_entry_t *pt2 = (void*)physmem + *pt; pt2 < (pt_entry_t*)(physmem + *pt) + NO_OF_PT_ENTRIES; ++pt2){
			// The second level table
			if(!(*pt2 & X86_MMU_PG_P))
				continue; // current range is not presented
			if(X86_MMU_PG_PS & *pt2){ // Huge page (1GB)
			*pt2 = *pt2 & ~0xFFF;
			current_vma->data = malloc(sizeof(char) * GB1);
			memset(current_vma->data, 0x0, GB1);
			current_vma->base = (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1;
			current_vma->size = GB1;
			current_vma->next = malloc(sizeof(vma_t));
			memset(current_vma->next, 0x0, sizeof(vma_t));
			memcpy(current_vma->data, physmem  + *pt2, GB1); // Copy one huge page(1GB) - also absolutely useless for our 512MB memory :)
			current_vma= current_vma->next;
			current_vma->next = NULL;
			current_vma->data = NULL;
			continue;
			}
			*pt2 = *pt2 & ~0xFFF;

			for(pt_entry_t *pt3 = (pt_entry_t*)(physmem + *pt2); pt3 < (pt_entry_t*)(physmem + *pt2) + NO_OF_PT_ENTRIES; ++pt3){
				// The third level of the page table
				
				if(!(*pt3 & X86_MMU_PG_P))
					continue; // current range is not presented in memory

				if(((*pt3 & ~X86_FLAGS_MASK) >= PHYMEM_SIZE)){
					vaddr_t vaddr = (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2;
						// printf("WEIRD CASE out of 3rd level physmem 0x%llx > 0x%llx\n", *pt3, PHYMEM_SIZE);
						// printf("WEIRD CASE out of 3rd level physmem 0x%llx\n", (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2);
						// if(vaddr >= (uint64_t)0xffff80200000 && vaddr < (uint64_t)0xffff80300000 )
						// 	printf("WTFFFFF vaddr: 0x%llx ;;; physaddr: 0x%llx\n", vaddr, (*pt3));
						continue;
				}
				
				if(X86_MMU_PG_PS & *pt3){ // Huge page (2MB)
					*pt3 = *pt3 & ~X86_FLAGS_MASK;

					current_vma->data = malloc(sizeof(char) * MB2);
					if(!current_vma->data){
						return 1;
					}
					memset(current_vma->data, 0x0, MB2);
					current_vma->base = (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2;
					current_vma->pbase = (uint64_t)(physmem) + *pt3;
					current_vma->size = MB2;
					current_vma->next = malloc(sizeof(vma_t));
					memset(current_vma->next, 0x0, sizeof(vma_t));
					memcpy(current_vma->data, physmem  + *pt3, current_vma->size); // Copy one huge page(2MB)
					current_vma= current_vma->next;
					current_vma->next = NULL;
					current_vma->data = NULL;
					current_vma->size = 0x0;

					continue;
				}
				*pt3 = *pt3 & ~0xFFF;

				for(pt_entry_t *pt4 = (void*)physmem + *pt3; pt4 < (pt_entry_t*)(physmem + *pt3) + NO_OF_PT_ENTRIES; ++pt4){
					// Finally the last level :)
					if(!(*pt4 & X86_MMU_PG_P))
						continue; // current range is not presented in memory
					if(((*pt4 & ~X86_FLAGS_MASK) >= PHYMEM_SIZE)){
						vaddr_t vaddr = (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2 + (pt4 - (pt_entry_t*)(physmem + *pt3)) * KB4;
						// printf("WEIRD CASE out of 3rd level physmem 0x%llx > 0x%llx\n", *pt3, PHYMEM_SIZE);
						// printf("WEIRD CASE out of 3rd level physmem 0x%llx\n", (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2);
						// if(vaddr >= (uint64_t)0xffff80200000 && vaddr < (uint64_t)0xffff80300000 )
						// 	printf("WTFFFFF vaddr: 0x%llx ;;; physaddr: 0x%llx\n", vaddr, (*pt4));
						// printf("WEIRD CASE out of 4th level physmem 0x%llx > 0x%llx\n", *pt4, PHYMEM_SIZE);
						continue;
					}
					*pt4 = *pt4 & ~X86_FLAGS_MASK;

					// printf("Copying something\n");
					current_vma->data = malloc(sizeof(char) * KB4);
					if(!current_vma->data){
						return 1;
					}
					memset(current_vma->data, 0x0, KB4);
					current_vma->base = (uint64_t)(pt - pg_high) * GB512 + (pt2 - (pt_entry_t*)(physmem + *pt)) * GB1 + (pt3 - (pt_entry_t*)(physmem + *pt2)) * MB2 + (pt4 - (pt_entry_t*)(physmem + *pt3)) * KB4;
					current_vma->pbase = (uint64_t)(physmem) + *pt4;
					current_vma->size = KB4;
					current_vma->next = malloc(sizeof(vma_t));
					memset(current_vma->next, 0x0, sizeof(vma_t));
					memcpy(current_vma->data, (void*)(physmem) + *pt4, KB4); // Copy one small page (4KB)
					current_vma = current_vma->next;
					current_vma->next = NULL;
					current_vma->data = NULL;
					current_vma->size = 0x0;
					continue;
				}
			}
		}
	}

	char string_buffer[256];
	while(first_vma && first_vma->data){
		// if(first_vma->base == 0xffff8020f000){
		// 	struct thread* thread_init = containerof(first_vma->data + 0x87e, thread_t, thread_list_node);
		// 	printf("USER TID %d\n", thread_init->user_tid);
		// }
		printf("my_number: %#018" PRIx64 "\n", first_vma->base);
		sprintf(string_buffer, "\n===VMA { BASE:0x%llx, PHYS_BASE=0x%llx SIZE:%llx} DATA:\n", first_vma->base, first_vma->pbase, first_vma->size);
		// for(size_t cursor = 0; cursor < first_vma->size; cursor += 512){
			// if(!memcmp(first_vma->data + cursor, empty_buffer, min(512, first_vma->size - cursor)) )
				// continue;
			write(STDOUT_FILENO, string_buffer, 83);
	 		write(STDOUT_FILENO, first_vma->data, first_vma->size);
	 	// }
	 	// printf("\n");
	 	first_vma = first_vma->next;
	}
	return 0;
}
