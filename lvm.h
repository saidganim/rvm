#ifndef LVM_H
#define LVM_H

#include <stdint.h>
#include <unistd.h>


// - PML4 entries refer to 512GB pages
// - PDP entries refer to 1GB pages
// - PD entries refer to 2MB pages
// - PT entries refer to 4KB pages

#define GB512 (1024 * 1024 * 1024 * 512)
#define GB1 (1024 * 1024 * 1024)
#define MB2 (1024 * 1024 * 2)
#define KB4 (1024 * 4)

#define X86_MMU_PG_P            0x0001          /* P    Valid                   */
#define X86_MMU_PG_RW           0x0002          /* R/W  Read/Write              */
#define X86_MMU_PG_U            0x0004          /* U/S  User/Supervisor         */
#define X86_MMU_PG_WT           0x0008          /* WT   Write-through           */
#define X86_MMU_PG_CD           0x0010          /* CD   Cache disable           */
#define X86_MMU_PG_A            0x0020          /* A    Accessed                */
#define X86_MMU_PG_D            0x0040          /* D    Dirty                   */
#define X86_MMU_PG_PS           0x0080          /* PS   Page size (0=4k,1=4M)   */
#define X86_MMU_PG_PTE_PAT      0x0080          /* PAT  PAT index for 4k pages  */
#define X86_MMU_PG_LARGE_PAT    0x1000          /* PAT  PAT index otherwise     */
#define X86_MMU_PG_G            0x0100          /* G    Global                  */
#define X86_DIRTY_ACCESS_MASK   0xf9f

#define PML4_SHIFT              39
#define PDP_SHIFT               30
#define PD_SHIFT                21
#define PT_SHIFT                12
#define ADDR_OFFSET             9
#define PDPT_ADDR_OFFSET        2
#define NO_OF_PT_ENTRIES        512

typedef uint64_t pt_entry_t;
typedef uint64_t vaddr_t;

// Virtual Memory Area
typedef struct vma {
	vaddr_t base;
	size_t size;
	char* data;
	struct vma* next;
} vma_t;


#define PHYMEM_SIZE (512 * 1024 * 1024) // physical memory is 512 MB by default

#define KPT (0x258000)
//2445312
// thread_list - 0xffffffff8020f87e

#define min(a,b) ({ \
		__typeof__(a) __a = a; \
		__typeof__(b) __b = b; \
		(__a) < (__b)? (__a) : (__b); \
})

#define max(a,b) ({ \
		__typeof__(a) __a = a; \
		__typeof__(b) __b = b; \
		(__a) < (__b)? (__b) : (__a); \
})


#endif