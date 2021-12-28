namec = $(wildcard *.c)
nameo = $(patsubst %.c, %.o ,$(src))

ALL: cgiserver

cgiserver: cgiserver.o
	g++ cgiserver.o -o cgiserver
cgiserver.o: cgiserver.cpp
	g++ -c cgiserver.cpp -o cgiserver.o

clean:
	-rm -rf	$(nameo) 	

