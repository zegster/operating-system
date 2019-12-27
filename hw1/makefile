DT	= dt
DT_OBJ	= dt.o
OUTPUT	= $(DT)

.SUFFIXES: .c .o

all: $(OUTPUT)

$(DT): $(DT_OBJ)
	gcc -Wall -g -o $@ $(DT_OBJ)

.c.o:
	gcc -Wall -g -c $<

.PHONY: clean
clean:
	/bin/rm -f $(OUTPUT) *.o

