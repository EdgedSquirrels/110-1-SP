default: server.c
	gcc server.c -D WRITE_SERVER -fsanitize=undefined -o write_server
	gcc server.c -D READ_SERVER -fsanitize=undefined -o read_server