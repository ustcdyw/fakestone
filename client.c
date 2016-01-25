#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static const char prompt[] = ">>> ";
static char *cardlib;

void print_cmd_help();

int
send_cardlib(int fd)
{
	char buf[2000];
	int n;
	int cardfd = open(cardlib, O_RDONLY);
	if (cardfd < 0)
		return -1;
	n = read(cardfd, buf, sizeof(buf));
	if (n <= 0)
		return -1;
	n = write(fd, buf, n);
	if (n <= 0)
		return -1;
	return 0;
}

int
parse_cmd(int fd, char *buf, int len) {
	char cmd;
	int ret = 0;
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
	} else if (cmd == 6) {
		if (send_cardlib(fd) < 0) {
			ret = -2;
		}
	} else if (cmd == 9) {
		printf("Win!\n");
	} else if (cmd == 8) {
		printf("Lose!\n");
	}
	return ret;
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
	} else if (strncmp(token, "h", 1) == 0) {
		print_cmd_help();
	} else if (strncmp(token, "q", 1) == 0) {
		return -2;
	} else {
		return 0;
	}
	return blen >= 4 ? blen : 4;
}

void
print_help(char *s)
{
	printf("Usage: "
	       "%s [-s server-addr] [-p server-port]\n", s);
}

void
print_cmd_help()
{
	printf("\nCMD [ARG1] [ARG2] ... (help string)\n"
	       "s    (check the statement)\n"
	       "u num-in-hand num-in-battlefield (push a soldier into battlefield)\n"
	       "a our-num-in-battlefiled opponent-num-in-battlefiled. (attack, 0 for hero)\n"
	       "e    (end our turn)\n");
}

#define PORT 8888
int
connect_server(int argc, char *argv[])
{
	int fd;
	struct sockaddr_in sin;
	int i;
	int r;
	char *server_addr = "127.0.0.1";
	unsigned port = PORT;
	socklen_t addrlen;
	while ((r = getopt(argc, argv, "s:p:c:h")) != -1) {
		switch (r) {
		case 's':
			server_addr = optarg;
			break;
		case 'p':
			port = (unsigned)atoi(optarg);
			break;
		case 'c':
			cardlib = optarg;
			break;
		case 'h':
			print_help(argv[0]);
			break;
		}
	}
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		exit(1);
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(server_addr);
	sin.sin_port = htons(port);
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
	int fd = (int)(unsigned long long)arg;
	char buf[5000];
	while (1) {
		ret = read(fd, buf, sizeof(buf));
		buf[ret + 1] = '\0';
		if (parse_cmd(fd, buf, ret) == -2)
			break;
	}
	close(fd);
	printf("Recv routine failed\n");
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
	char cardpwd[1000];

	char *c;
	int i;
	getcwd(cardpwd, sizeof(cardpwd));
	c = &cardpwd[strlen(cardpwd)];
	*c = '/';
	c++;
	strcpy(c, argv[0]);
	for (i = strlen(cardpwd); cardpwd[i] != '/'; i--);
	strcpy(&cardpwd[i+1], "card");
	cardlib = cardpwd;

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
		} else if (ret == -2) {
			break;
		}
	}
	free(line);
}
