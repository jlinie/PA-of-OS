#include "../vm.c"

int main(int argc, char **argv) {
    initOS();
    fprintf(stdout, "Occupied memory after OS load:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);
    uint16_t ptr = allocMem(4096);
    fprintf(stdout, "Occupied memory after allocation:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);
    freeMem(ptr);
    fprintf(stdout, "Occupied memory after freeing:\n");
    fprintf_mem_nonzero(stdout, mem, UINT16_MAX);

    return 0;
}