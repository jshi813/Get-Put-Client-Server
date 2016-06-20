all: server clientGet

server: server.c
	gcc server.c -o server

clientGet: clientGet.c
	gcc clientGet.c -o clientGet

clean:
	rm server clientGet
