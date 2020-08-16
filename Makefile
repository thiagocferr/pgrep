CC = gcc
C_FILES = pgrep.c

FLAGS = -std=c99
LIB = -lpthread

pgrep: $(C_FILES)
	$(CC) $(FLAGS) $(C_FILES) -o $@ $(LIB)
