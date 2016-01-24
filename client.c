#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static const char prompt[] = ">>> ";

int
parse_cmd(char *buf, int len) {
	char cmd;
	if (len < 4)
		return -1;
	cmd = buf[1];
	if (cmd == 1) {
		printf("Our turn now!\n%s", prompt);
		fflush(stdout);
	}
	else if (cmd == 5) {
		printf("%s\n%s", buf+2, prompt);
		fflush(stdout);
	} else if (cmd == 9) {
		printf("Win!\n");
	} else if (cmd == 8) {
		printf("Lose!\n");
	}
	return 0;
}
int
parse_line(char *line, int len, char *buf)
{
	int blen = 0;
	char *token = strtok(line, " \t\n");
	if (token == NULL)
		return 0;
	buf[blen++] = 0; // first byte be 0;
	if (strncmp(token, "u", 1) == 0) {
		int t;
		buf[blen++] = 1;
		while (1) {
			token = strtok(NULL, " \t\n");
			if (token == NULL)
				break;
			t = atoi(token);
			buf[blen++] = (char)t;
		}
	} else if (strncmp(token, "a", 1) == 0) {
		int t;
		buf[blen++] = 2;
		while (1) {
			token = strtok(NULL, " \t\n");
			if (token == NULL)
				break;
			t = atoi(token);
			buf[blen++] = (char)t;
		}
	} else if (strncmp(token, "e", 1) == 0) {
		buf[blen++] = 3;
	} else if (strncmp(token, "s", 1) == 0) {
		buf[blen++] = 4;
	} else {
		return 0;
	}
	return blen >= 4 ? blen : 4;
}

#define PORT 8888
int
connect_server(int argc, char *argv[])
{
	int fd;
	struct sockaddr_in sin;
	int i;
	socklen_t addrlen;
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(PORT);
	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("connect");
		return -1;
	}
	return fd;
}
void
print_buf(char *buf, int len)
{
	int i;
	printf("sendbuf:\n");
	for (i = 0; i < len; i++)
		printf("%d ", (int)buf[i]);
	printf("\n");
}
void *
recv_routine(void *arg)
{
	int ret;
	int fd = (int)arg;
	char buf[5000];
	while (1) {
		ret = read(fd, buf, sizeof(buf));
		buf[ret + 1] = '\0';
		parse_cmd(buf, ret);
	}
}
int
main(int argc, char *argv[])
{
	int fd;
	char *line = NULL;
	size_t len = 0;
	pthread_t pt;
	char buf[1000];
	int ret;

	if ((fd = connect_server(argc, argv)) < 0)
		return -1;
	pthread_create(&pt, NULL, recv_routine, (void *)(unsigned long long)fd);
	while (1) {
		printf("%s", prompt);
		fflush(stdout);
		ret = getline(&line, &len, stdin);
		ret = parse_line(line, ret, buf);
		if (ret > 0) {
			write(fd, buf, ret);
		}
	}
	free(line);
}
