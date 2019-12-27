CC	= gcc
CFLAGS	= -Wall
LOGPARSE	= logParse
LOGPARSE_OBJ	= logParse.o
OUTPUT	= $(LOGPARSE)

.SUFFIXES: .c .o

all: $(OUTPUT)

$(LOGPARSE): $(LOGPARSE_OBJ)
	$(CC) $(CFLAGS) -g -o $@ $(LOGPARSE_OBJ)

.c.o:
	$(CC) $(CFLAGS) -g -c $<

.PHONY: clean
clean:
	/bin/rm -f $(OUTPUT) *.o

