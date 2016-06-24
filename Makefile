all: server get put
	chmod +x client.sh server.sh

server: server.c
	gcc server.c -o server

get: get.c
	gcc get.c -o get

put: put.c
	gcc put.c -o put

clean:
	rm server get put
