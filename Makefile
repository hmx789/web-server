TARGETS=webserver thread_example

CFLAGS=-Wall -g -O0

all: $(TARGETS)

homework5: webserver.c
	gcc $(CFLAGS) -o webserver webserver.c -lpthread

thread_example: thread_example.c
	gcc $(CFLAGS) -o thread_example thread_example.c -lpthread

clean:
	rm -f $(TARGETS)

