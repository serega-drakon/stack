CC = gcc
CPPFLAGS = -DTEST -MMD -MP

OBJ = stack.o main.o
SRC = stack.o main.o 

all: $(OBJ)
	$(CC) $(OBJ) -o main 
	
.PHONY: clean
clean:
	rm -rf *.d *.o *.gcda *.gcno *.info main test 

-include *.d