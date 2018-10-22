CC = gcc
CFLAGS = -lpthread
DEPS = fio.h
COBJ = saxpy.o
TARGET = main

main: $(COBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c $< $(CFLAGS)


.PHONY: clean

clean:
	rm *.o $(TARGET)
