#include "../vm.c"

int main(int argc, char **argv) {
    initOS();
    uint16_t pid = createProc("programs/simple_code.obj", "programs/simple_heap.obj");
    loadProc(0);
    mw(0x1000, 42); // changing the value of an address to 42.
    fprintf(stdout, "%d\n",  mr(0x1000)); // reading it back.
    return 0;
}