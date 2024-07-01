
C = gcc 
CFLAGS = -std=c11 -Wall

TARGET1 = vm
TARGET2 = programs/simple
TARGET3 = programs/brk
TARGET4 = programs/yld
TARGET5 = tests

SOURCE1 = main.c
SOURCE2 = programs/simple.c
SOURCE3 = programs/brk.c
SOURCE4 = programs/yld.c

OBJ1 = programs/simple_code.obj
OBJ2 = programs/brk_code.obj
OBJ3 = programs/yld_code.obj
OBJ4 = programs/simple_heap.obj
OBJ5 = programs/brk_heap.obj
OBJ6 = programs/yld_heap.obj

TEST1 = tests/initos-test
TEST2 = tests/mem-test
TEST3 = tests/coalesce-test
TEST4 = tests/proc-test
TEST5 = tests/mw-mr-test
TEST6 = tests/mw-mr-test2

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) tests

$(TARGET1): $(SOURCE1)
	$(C) $(SOURCE1) -o $(TARGET1) $(CFLAGS)

$(TARGET2): $(SOURCE2)
	$(C) $(SOURCE2) -o $(TARGET2) $(CFLAGS)
	$(TARGET2)
	rm $(TARGET2)

$(TARGET3): $(SOURCE3)
	$(C) $(SOURCE3) -o $(TARGET3) $(CFLAGS)
	$(TARGET3)
	rm $(TARGET3)

$(TARGET4): $(SOURCE4)
	$(C) $(SOURCE4) -o $(TARGET4) $(CFLAGS)
	$(TARGET4)
	rm $(TARGET4)

	$(C) $(TEST1).c -o $(TEST1) $(CFLAGS)
	$(C) $(TEST2).c -o $(TEST2) $(CFLAGS)
	$(C) $(TEST3).c -o $(TEST3) $(CFLAGS)
	$(C) $(TEST4).c -o $(TEST4) $(CFLAGS)
	$(C) $(TEST5).c -o $(TEST5) $(CFLAGS)
	$(C) $(TEST6).c -o $(TEST6) $(CFLAGS)

# $(TARGET4): $(TARGET2)
# 	$(TARGET2) 
.PHONY: clean
clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(TEST1) $(TEST2) $(TEST3) $(TEST4) $(TEST5) $(TEST6)