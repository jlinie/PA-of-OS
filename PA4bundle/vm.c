#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "vm_dbg.h"

#define NOPS (16)

#define OPC(i) ((i)>>12)
#define DR(i) (((i)>>9)&0x7)
#define SR1(i) (((i)>>6)&0x7)
#define SR2(i) ((i)&0x7)
#define FIMM(i) ((i>>5)&01)
#define IMM(i) ((i)&0x1F)
#define SEXTIMM(i) sext(IMM(i),5)
#define FCND(i) (((i)>>9)&0x7)
#define POFF(i) sext((i)&0x3F, 6)
#define POFF9(i) sext((i)&0x1FF, 9)
#define POFF11(i) sext((i)&0x7FF, 11)
#define FL(i) (((i)>>11)&1)
#define BR(i) (((i)>>6)&0x7)
#define TRP(i) ((i)&0xFF)   

//New OS declarations
//  OS Bookkeeping constants 
#define OS_MEM_SIZE 4096    // OS Region size. At the same time, constant free-list starting header 

#define Cur_Proc_ID 0       // id of the current process
#define Proc_Count 1        // total number of processes, including ones that finished executing.
#define OS_STATUS 2         // Bit 0 shows whether the PCB list is full or not

//  Process list and PCB related constants
#define PCB_SIZE 6  // Number of fields in a PCB

#define PID_PCB 0   // holds the pid for a process
#define PC_PCB 1    // value of the program counter for the process
#define BSC_PCB 2   // base value of code section for the process
#define BDC_PCB 3   // bound value of code section for the process
#define BSH_PCB 4   // value of heap section for the process
#define BDH_PCB 5   // holds the bound value of heap section for the process

#define CODE_SIZE 4096
#define HEAP_INIT_SIZE 4096
//New OS declarations


bool running = true;

typedef void (*op_ex_f)(uint16_t i);
typedef void (*trp_ex_f)();

enum { trp_offset = 0x20 };
enum regist { R0 = 0, R1, R2, R3, R4, R5, R6, R7, RPC, RCND, RBSC, RBDC, RBSH, RBDH, RCNT };
enum flags { FP = 1 << 0, FZ = 1 << 1, FN = 1 << 2 };

extern uint16_t mem[UINT16_MAX] = {0};
uint16_t reg[RCNT] = {0};
uint16_t PC_START = 0x3000;

void initOS();
int createProc(char *fname, char* hname);
void loadProc(uint16_t pid);
static inline void tyld();
uint16_t allocMem(uint16_t size);
int freeMem(uint16_t ptr);
static inline uint16_t mr(uint16_t address);
static inline void mw(uint16_t address, uint16_t val);
static inline void tbrk();
static inline void thalt();
static inline void trap(uint16_t i);

static inline uint16_t sext(uint16_t n, int b) { return ((n>>(b-1))&1) ? (n|(0xFFFF << b)) : n; }
static inline void uf(enum regist r) {
    if (reg[r]==0) reg[RCND] = FZ;
    else if (reg[r]>>15) reg[RCND] = FN;
    else reg[RCND] = FP;
}
static inline void add(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] + (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void and(uint16_t i)  { reg[DR(i)] = reg[SR1(i)] & (FIMM(i) ? SEXTIMM(i) : reg[SR2(i)]); uf(DR(i)); }
static inline void ldi(uint16_t i)  { reg[DR(i)] = mr(mr(reg[RPC]+POFF9(i))); uf(DR(i)); }
static inline void not(uint16_t i)  { reg[DR(i)]=~reg[SR1(i)]; uf(DR(i)); }
static inline void br(uint16_t i)   { if (reg[RCND] & FCND(i)) { reg[RPC] += POFF9(i); } }
static inline void jsr(uint16_t i)  { reg[R7] = reg[RPC]; reg[RPC] = (FL(i)) ? reg[RPC] + POFF11(i) : reg[BR(i)]; }
static inline void jmp(uint16_t i)  { reg[RPC] = reg[BR(i)]; }
static inline void ld(uint16_t i)   { reg[DR(i)] = mr(reg[RPC] + POFF9(i)); uf(DR(i)); }
static inline void ldr(uint16_t i)  { reg[DR(i)] = mr(reg[SR1(i)] + POFF(i)); uf(DR(i)); }
static inline void lea(uint16_t i)  { reg[DR(i)] =reg[RPC] + POFF9(i); uf(DR(i)); }
static inline void st(uint16_t i)   { mw(reg[RPC] + POFF9(i), reg[DR(i)]); }
static inline void sti(uint16_t i)  { mw(mr(reg[RPC] + POFF9(i)), reg[DR(i)]); }
static inline void str(uint16_t i)  { mw(reg[SR1(i)] + POFF(i), reg[DR(i)]); }
static inline void rti(uint16_t i) {} // unused
static inline void res(uint16_t i) {} // unused
static inline void tgetc() { reg[R0] = getchar(); }
static inline void tout() { fprintf(stdout, "%c", (char)reg[R0]); }
static inline void tputs() {
    uint16_t *p = mem + reg[R0];
    while(*p) {
        fprintf(stdout, "%c", (char)*p);
        p++;
    }
}
static inline void tin() { reg[R0] = getchar(); fprintf(stdout, "%c", reg[R0]); }
static inline void tputsp() { /* Not Implemented */ }

static inline void tinu16() { fscanf(stdin, "%hu", &reg[R0]); }
static inline void toutu16() { fprintf(stdout, "%hu\n", reg[R0]); }


trp_ex_f trp_ex[10] = { tgetc, tout, tputs, tin, tputsp, thalt, tinu16, toutu16, tyld, tbrk };
static inline void trap(uint16_t i) { trp_ex[TRP(i)-trp_offset](); }
op_ex_f op_ex[NOPS] = { /*0*/ br, add, ld, st, jsr, and, ldr, str, rti, not, ldi, sti, jmp, res, lea, trap };


void ld_img(char *fname, uint16_t offset, uint16_t size) {
    FILE *in = fopen(fname, "rb");
    if (NULL==in) {
        fprintf(stderr, "Cannot open file %s.\n", fname);
        exit(1);    
    }
    uint16_t *p = mem + offset;
    fread(p, sizeof(uint16_t), (size), in);
    fclose(in);
}

void run(char* code, char* heap) {

    while(running) {
        uint16_t i = mr(reg[RPC]++);
        op_ex[OPC(i)](i);
    }
}


// YOUR CODE STARTS HERE

void initOS() {
    mw(0, 0xffff);
    mw(1, 0);
    mw(2, 0x0000);

    mw(4096, 61438);
    mw(4097, 0);
}

// process functions to implement
int createProc(char *fname, char* hname) {
    uint16_t os_status = mr(OS_STATUS);
    if (os_status & 1) {
        printf("The OS memory region is full. Cannot create a new PCB.\n");
        return 0;
    }
    
    uint16_t code_base = allocMem(CODE_SIZE);
    if (code_base == 0) {
        printf("Cannot create code segment.\n");
        return 0;
    }
    
    uint16_t heap_base = allocMem(HEAP_INIT_SIZE);
    if (heap_base == 0) {
        printf("Cannot create heap segment.\n");
        return 0;
    }
    
    uint16_t proc_count = mr(Proc_Count);
    uint16_t new_pid = proc_count;
    uint16_t pcb_addr = 12 + (new_pid * PCB_SIZE); // Calculate the address of the new PCB

    mw(pcb_addr + PID_PCB, new_pid);      // Set PID
    mw(pcb_addr + PC_PCB, 0x3000);        // Set PC
    mw(pcb_addr + BSC_PCB, code_base);    // Set base of code segment
    mw(pcb_addr + BDC_PCB, CODE_SIZE);    // Set bound of code segment
    mw(pcb_addr + BSH_PCB, heap_base);    // Set base of heap segment
    mw(pcb_addr + BDH_PCB, HEAP_INIT_SIZE); // Set bound of heap segment

    mw(Proc_Count, proc_count + 1);
    if ((proc_count + 1) * PCB_SIZE >= OS_MEM_SIZE - 12) {
        mw(OS_STATUS, os_status | 1); // Set the least significant bit to 1 indicating the OS region is full
    }

    ld_img(fname, code_base, CODE_SIZE);
    ld_img(hname, heap_base, HEAP_INIT_SIZE);

    return 1;
}

void loadProc(uint16_t pid) {
    uint16_t pcb_addr = 12 + (pid * PCB_SIZE);
    uint16_t pc = mr(pcb_addr + PC_PCB);
    uint16_t bsc = mr(pcb_addr + BSC_PCB);
    uint16_t bdc = mr(pcb_addr + BDC_PCB);
    uint16_t bsh = mr(pcb_addr + BSH_PCB);
    uint16_t bdh = mr(pcb_addr + BDH_PCB);

    mw(Cur_Proc_ID, pid);
    mw(8, pc);     // Assuming register 8 is the PC register
    mw(9, bsc);    // Assuming register 9 is the base register for code
    mw(10, bdc);   // Assuming register 10 is the bound register for code
    mw(11, bsh);   // Assuming register 11 is the base register for heap
    mw(12, bdh);   // Assuming register 12 is the bound register for heap
}

uint16_t allocMem(uint16_t size) {
    uint16_t required_size = size + 2; // Size including header
    uint16_t current = 4096; // Start from the first free chunk
    uint16_t prev = 0;

    while (current != 0) {
        uint16_t chunk_size = mr(current); // Size of the current free chunk
        uint16_t next_free = mr(current + 1); // Next free chunk address

        if (chunk_size >= required_size) {
            uint16_t new_chunk_size = chunk_size - required_size;
            mw(current, new_chunk_size); // Update the size of the free chunk

            uint16_t alloc_start = current + new_chunk_size + 2;
            mw(alloc_start, size);  // Set size in header
            mw(alloc_start + 1, 42); // Set magic number in header

            if (new_chunk_size == 0) {
                // If the entire chunk is used, update the previous chunk's next pointer
                if (prev != 0) {
                    mw(prev + 1, next_free);
                }
            }

            return alloc_start + 2; // Return the beginning address of the allocated chunk
        }

        prev = current;
        current = next_free; // Move to the next free chunk
    }

    return 0; // No suitable free chunk found
}


int freeMem(uint16_t ptr) {
    if (ptr < OS_MEM_SIZE) {
        return 0; // Pointer is in the OS region
    }

    uint16_t header_addr = ptr - 2;
    if (mr(header_addr + 1) != 42) {
        return 0; // Pointer is not the beginning of an allocated region
    }

    uint16_t size = mr(header_addr); // Size of the chunk to be freed
    uint16_t current = 4096; // Start from the first free chunk at address 4096
    uint16_t prev = 0;

    while (current != 0 && current < header_addr) {
        prev = current;
        current = mr(current + 1); // Move to the next free chunk
    }

    if (prev != 0) {
        mw(prev + 1, header_addr); // Link the previous free chunk to the new free chunk
    }

    mw(header_addr, size); // Set the size in the header of the freed chunk
    mw(header_addr + 1, current); // Link the new free chunk to the next free chunk

    // Coalesce with the next chunk if it is free
    if (current != 0 && current == header_addr + size + 2) {
       
        size += mr(current) + 2; // Include the size of the next chunk and its header
        mw(header_addr, size); // Update the size in the header
        mw(header_addr + 1, mr(current + 1)); // Link to the next-next free chunk
    }

    // Coalesce with the previous chunk if it is free
    if (prev != 0 && prev + mr(prev) + 2 == header_addr) {
        uint16_t prev_size = mr(prev);
        size += prev_size + 2; // Include the size of the previous chunk and its header
        mw(prev, size); // Update the size in the header of the previous chunk
        mw(prev + 1, mr(header_addr + 1)); // Link to the next free chunk
    } else {
        // Update header to indicate this is a free chunk if it wasn't coalesced
        mw(header_addr, size);
        mw(header_addr + 1, current);
    }

    // Clear the header of the freed chunk for cleanliness
    mw(header_addr, 0);
    //mw(header_addr + 1, 0);
  

    return 1;
}


static inline void thalt() {
    running = false; 
}

static inline uint16_t mr(uint16_t address) {
    return mem[address];  
}

static inline void mw(uint16_t address, uint16_t val) {
    mem[address] = val;
}

static inline void tyld() {
    uint16_t oldProcID = mr(Cur_Proc_ID);
    uint16_t procCount = mr(Proc_Count);
    uint16_t newProcID = oldProcID;

    do {
        newProcID = (newProcID + 1) % procCount;
    } while (mr(12 + newProcID * PCB_SIZE + PID_PCB) == 0xffff);

    if (newProcID != oldProcID) {
        printf("We are switching from process %d to %d.\n", oldProcID, newProcID);
    } else {
        printf("No other process to switch to, continuing with process %d.\n", oldProcID);
    }

    loadProc(newProcID);
}

static inline void tbrk() {
    uint16_t curProcID = mr(Cur_Proc_ID);
    uint16_t pcb_addr = 12 + curProcID * PCB_SIZE;
    uint16_t heap_base = mr(pcb_addr + BSH_PCB);
    uint16_t old_heap_size = mr(pcb_addr + BDH_PCB);
    uint16_t new_heap_size = reg[0];  // Assume reg[0] holds the new size value

    printf("Heap increase requested by process %d.\n", curProcID);

    if (new_heap_size > old_heap_size) {
        uint16_t next_chunk = heap_base + old_heap_size + 2;
        uint16_t next_chunk_size = mr(next_chunk);

        if (next_chunk_size == 42) {
            printf("Cannot allocate more space for the heap of pid %d since we bumped into an allocated region.\n", curProcID);
            return;
        }

        if (new_heap_size > old_heap_size + next_chunk_size) {
            printf("Cannot allocate more space for the heap of pid %d since total free space size here is not enough.\n", curProcID);
            return;
        }

        mw(pcb_addr + BDH_PCB, new_heap_size);

        if (new_heap_size == old_heap_size + next_chunk_size) {
            mw(heap_base + old_heap_size, next_chunk_size + 2);
            mw(heap_base + old_heap_size + 1, mr(next_chunk + 1));
        }
    } else {
        mw(pcb_addr + BDH_PCB, new_heap_size);
        uint16_t new_free_chunk_addr = heap_base + new_heap_size;
        mw(new_free_chunk_addr, old_heap_size - new_heap_size - 2);
        mw(new_free_chunk_addr + 1, mr(4096 + 1));  // Link to the first free chunk
        mw(4096 + 1, new_free_chunk_addr);  // Update the first free chunk link
    }
}


// YOUR CODE ENDS HERE