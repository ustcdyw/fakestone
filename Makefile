
all:
	cc -o client client.c -pthread
	cc -o server server.c -pthread
clean:
	rm client
	rm server
