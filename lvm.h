#ifndef LVM_H
#define LVM_H

#include <stdint.h>
#include <unistd.h>

// - PML4 entries refer to 512GB pages
// - PDP entries refer to 1GB pages
// - PD entries refer to 2MB pages
// - PT entries refer to 4KB pages

#define GB512 ((uint64_t)1024 * (uint64_t)1024 * (uint64_t)1024 * (uint64_t)512)
#define GB1 ((uint64_t)1024 * (uint64_t)1024 * (uint64_t)1024)
#define MB2 ((uint64_t)1024 * (uint64_t)1024 * (uint64_t)2)
#define KB4 ((uint64_t)1024 * (uint64_t)4)

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

#define X86_MAX_EXTENDED_REGISTER_SIZE 1024

#define X86_FLAGS_MASK          (0x8000000000000ffful)

#define PML4_SHIFT              39
#define PDP_SHIFT               30
#define PD_SHIFT                21
#define PT_SHIFT                12
#define ADDR_OFFSET             9
#define PDPT_ADDR_OFFSET        2
#define NO_OF_PT_ENTRIES        512

#define THREAD_NAME_LENGTH 32

#define THREAD_LINEBUFFER_LENGTH 128

#define THREAD_MAGIC (0x74687264) // 'thrd'

// Number of kernel tls slots.
#define THREAD_MAX_TLS_ENTRY 2

typedef int bool;

typedef uint64_t pt_entry_t;
typedef uint64_t vaddr_t;
typedef uint64_t paddr_t;

// Virtual Memory Area
typedef struct vma {
	vaddr_t base;
	paddr_t pbase;
	size_t size;
	char* data;
	struct vma* next;
} vma_t;


#define PHYMEM_SIZE (512 * 1024 * 1024) // physical memory is 512 MB by default

#define KPT (0x258000)
//2445312
// thread_list - 0xffff8020f87e
// new thread_list - 0xffffffff8020f8c0

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

#define offsetof(type, member) (&((type*)(0x0))->member)
#define containerof(ptr, type, member) ((type*)((void*)(ptr)-(void*)offsetof(type, member)))

typedef struct list_node list_node_t;

struct list_node {
    list_node_t* prev;
    list_node_t* next;
};

typedef uint64_t zx_time_t;
typedef uint64_t zx_duration_t;

typedef int32_t zx_status_t;

typedef uint32_t cpu_mask_t;
typedef uint32_t cpu_num_t;


typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
} x86_syscall_general_regs_t;


struct arch_thread {
    vaddr_t sp;
// #if __has_feature(safe_stack)
//     vaddr_t unsafe_sp;
// #endif
    vaddr_t fs_base;
    vaddr_t gs_base;

    // Which entry of |suspended_general_regs| to use.
    // One of X86_GENERAL_REGS_*.
    uint32_t general_regs_source;

    // Debugger access to userspace general regs while suspended or stopped
    // in an exception.
    // The regs are saved on the stack and then a pointer is stored here.
    // NULL if not suspended or stopped in an exception.
    union {
        void *gregs;
        x86_syscall_general_regs_t *syscall;
        void* iframe; // x86_iframe_t* struct pointer
    } suspended_general_regs;

    /* buffer to save fpu and extended register (e.g., PT) state */
    vaddr_t *extended_register_state;
    uint8_t extended_register_buffer[X86_MAX_EXTENDED_REGISTER_SIZE + 64];

    /* if non-NULL, address to return to on page fault */
    void *page_fault_resume;
};


typedef struct wait_queue {
    int magic;
    int count;
    struct list_node heads;
} wait_queue_t;

enum thread_state {
    THREAD_INITIAL = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_SUSPENDED,
    THREAD_DEATH,
};


typedef struct zx_thread {
    int magic;
    struct list_node thread_list_node;

    /* active bits */
    struct list_node queue_node;
    enum thread_state state;
    zx_time_t last_started_running;
    zx_duration_t remaining_time_slice;
    unsigned int flags;
    unsigned int signals;

    /* Total time in THREAD_RUNNING state.  If the thread is currently in
     * THREAD_RUNNING state, this excludes the time it has accrued since it
     * left the scheduler. */
    zx_duration_t runtime_ns;

    /* priority: in the range of [MIN_PRIORITY, MAX_PRIORITY], from low to high.
     * base_priority is set at creation time, and can be tuned with thread_set_priority().
     * priority_boost is a signed value that is moved around within a range by the scheduler.
     * inherited_priority is temporarily set to >0 when inheriting a priority from another
     * thread blocked on a locking primitive this thread holds. -1 means no inherit.
     * effective_priority is MAX(base_priority + priority boost, inherited_priority) and is
     * the working priority for run queue decisions.
     */
    int effec_priority;
    int base_priority;
    int priority_boost;
    int inherited_priority;

    /* current cpu the thread is either running on or in the ready queue, undefined otherwise */
    cpu_num_t curr_cpu;
    cpu_num_t last_cpu;      /* last cpu the thread ran on, INVALID_CPU if it's never run */
    cpu_mask_t cpu_affinity; /* mask of cpus that this thread can run on */

    /* if blocked, a pointer to the wait queue */
    void* blocking_wait_queue; // struct wait_queue*

    /* list of other wait queue heads if we're a head */
    struct list_node wait_queue_heads_node;

    /* return code if woken up abnormally from suspend, sleep, or block */
    zx_status_t blocked_status;

    /* are we allowed to be interrupted on the current thing we're blocked/sleeping on */
    bool interruptable;

    /* number of mutexes we currently hold */
    int mutexes_held;

    /* pointer to the kernel address space this thread is associated with */
    void* aspace; // struct vmm_aspace*

    /* pointer to user thread if one exists for this thread */
    void* user_thread;
    uint64_t user_tid;
    uint64_t user_pid;

    /* callback for user thread state changes */
    void* user_callback; // thread_user_callback_t - func pointer

    /* non-NULL if stopped in an exception */
    void* exception_context; // const struct arch_exception_context*

    /* architecture stuff */
    struct arch_thread arch;

    /* stack stuff */
    void* stack;
    size_t stack_size;
    vaddr_t stack_top;
// #if __has_feature(safe_stack)
//     void* unsafe_stack;
// #endif

    /* entry point */
    void* entry; // thread_start_routine - function ointer
    void* arg;

    /* Indicates whether a debugger is single stepping this thread. */
    bool single_step;

    /* return code */
    int retcode;
    struct wait_queue retcode_wait_queue;

    /* Preemption of the thread is disabled while this counter is non-zero.
     *
     * The preempt_disable field is modified by interrupt handlers, but it
     * is always restored to its original value before the interrupt
     * handler returns, so modifications are not visible to the interrupted
     * thread.  Despite that, "volatile" is still technically needed.
     * Otherwise the compiler is technically allowed to compile
     * "++thread->preempt_disable" into code that stores a junk value into
     * preempt_disable temporarily. */
    volatile uint32_t preempt_disable;
    /* This tracks whether a thread reschedule is pending.  This is
     * volatile because it can get set from true to false asynchronously by
     * an interrupt handler. */
    volatile bool preempt_pending;

    /* thread local storage, intialized to zero */
    void* tls[THREAD_MAX_TLS_ENTRY];

    /* callback for cleanup of tls slots */
    void* tls_callback[THREAD_MAX_TLS_ENTRY]; // thread_tls_callback_t - function pointer

    char name[THREAD_NAME_LENGTH];
// #if WITH_DEBUG_LINEBUFFER
//     /* buffering for debug/klog output */
//     int linebuffer_pos;
//     char linebuffer[THREAD_LINEBUFFER_LENGTH];
// #endif
} thread_t;





#endif