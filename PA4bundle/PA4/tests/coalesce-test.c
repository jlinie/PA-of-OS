#include "../vm.c"

int main(int argc, char **argv) {
    initOS();
    uint16_t ptr1 = allocMem(64);
    uint16_t ptr2 = allocMem(64);
    uint16_t ptr3 = allocMem(64);
    uint16_t ptr4 = allocMem(64);
    fprintf(stdout, "Occupied memory after allocation:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);
    freeMem(ptr2);
    freeMem(ptr3);
    fprintf(stdout, "Occupied memory after freeing:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);

    return 0;
}