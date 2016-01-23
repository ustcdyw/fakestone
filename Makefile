
all:
	cc -o client client.c
	cc -o server server.c
clean:
	rm client
	rm server
