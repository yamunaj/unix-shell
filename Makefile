output := shell_yj

CFLAGS :=
#CFLAGS += -DDEBUG

shell : shell.o setup.o history.o
	gcc -o $(output) shell.o setup.o history.o

history.o : history.c var.h
	gcc -c history.c $(CFLAGS)

setup.o : setup.c history.h
	gcc -c setup.c $(CFLAGS)

shell.o : shell.c shell.h history.h
	gcc -c shell.c $(CFLAGS)

clean:
	rm *.o
	rm $(output)
