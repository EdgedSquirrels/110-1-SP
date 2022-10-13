all: server.c
	gcc server.c -D WRITE_SERVER -o write_server
	gcc server.c -D READ_SERVER -o read_server
clean: server.c
	rm -f write_server
	rm -f read_server