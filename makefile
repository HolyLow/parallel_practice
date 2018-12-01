CC = gcc
CFLAGS = -lpthread -lm
DEPS = fio.h
# COBJ = saxpy.o
# COBJ = concat.o
# COBJ = 2d5p.o
COBJ = nbody.o
TARGET = main

main: $(COBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c $< $(CFLAGS)


.PHONY: clean

clean:
	rm *.o $(TARGET)
