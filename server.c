#include <stdio.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define CARDMAX 30

enum soldier_state {
	CARD_LIB = 0,
	HAND,
	BATTLEFILED,
};

/*
static void
print_soldier(struct soldier *s)
{
	if (s->state == BATTLEFILED)
		printf("AF:%d, HP:%d\n", s->attack_force, s->hp);
	else
		printf("AF:%d, HP:%d, POW:%d\n",
		       s->attack_force, s->hp, s->power);
}
*/

struct soldier {
	int hp;
	unsigned attack_force;
	unsigned can_attack;
	unsigned power;
	enum soldier_state state;
	// int (*attack)(int );
	// void (*print_stat)(struct soldier * );
};

struct spell {
	unsigned power;
};
enum card_type {
	SOLDIER,
	SPELL,
};
struct card {
	union {
		struct soldier soldier;
		struct spell spell;
	};
	enum card_type type;
};

struct card_lib {
	unsigned next_card;
	struct card cards[CARDMAX];
	struct card *(*get_next_card)(struct card_lib * );
};

struct card *
get_next_card(struct card_lib *cl)
{
	if (cl->next_card >= CARDMAX) {
		cl->next_card++;
		return NULL;
	}
	
	return &cl->cards[cl->next_card++];
}

struct hero {
	int hp;
	unsigned attack_force;
	unsigned can_attack;
	unsigned power;
	unsigned max_power;
	struct card_lib cl;
	int n_hand_cards;
	struct card *hand_cards[10];
	struct soldier *battle_soldier[7];
	void (*get_card)(struct hero * );
	/* args:
	 * hand field position,
	 * battle field position.
	 * */
	void (*push_soldier)(struct hero *, unsigned, unsigned);
	/* args:
	 * battle filed position,
	 * oppoent filed position
	 * */
	int (*attack)(struct hero *, unsigned, unsigned );
};

static int turn;
static int sock[2];

void
get_card(struct hero *h)
{
	struct card *c = h->cl.get_next_card(&h->cl);
	if (c == NULL)
		return;
	if (h->n_hand_cards >= 10)
		return;
	h->hand_cards[h->n_hand_cards++] = c;
}

/* args:
 * @hand_pos: the posistion in hand.
 * @battle_pos: Pos 0 stand for hero.
 * */
void
push_soldier(struct hero *h, unsigned hand_pos, unsigned battle_pos)
{
	struct soldier *s;
	if (hand_pos >= 10 || battle_pos >= 7)
		return;
	if (h->hand_cards[hand_pos]->type != SOLDIER)
		return;
	if (h->battle_soldier[battle_pos] != NULL)
		return;
	if (hand_pos >= h->n_hand_cards)
		return;

	s = &h->hand_cards[hand_pos]->soldier;
	if (h->power < s->power)
		return;
	h->battle_soldier[battle_pos] = &h->hand_cards[hand_pos]->soldier;
	h->hand_cards[hand_pos] = NULL;
	if (hand_pos < h->n_hand_cards-1) {
		int i;
		for (i = hand_pos; i < h->n_hand_cards-1; i++) {
			h->hand_cards[i] = h->hand_cards[i+1];
		}
		h->hand_cards[i] = NULL;
	}
	h->power -= s->power;
	h->n_hand_cards--;
}

static struct hero heros[2];
struct hero *
get_other_hero(struct hero *h)
{
	if (h == heros)
		return heros+1;
	return heros;
}

int
print_hero(struct hero *h, char *str)
{
	int n =
	sprintf(str, "Hero >>> " "HP:%d " "CanATK:%u " "ATK:%u " "POW:%u <<<\n",
	       h->hp, h->can_attack, h->attack_force, h->power);
	return n;
}

int
print_battlefield(struct hero *h1, struct hero *h2, char *str)
{
	int i, n;
	char *buf = str;
	n = sprintf(buf, "===========================================================\n");
	buf += n;
	n = sprintf(buf, "|  HP  |  ATK | CATK | ---> NO. <--- |  HP  |  ATK | CATK |\n");
	buf += n;
	for (i = 0; i < 7; i++) {
		n = sprintf(buf, "|  %3d |  %3u |  %2u  | --->  %1d  <--- |  %3d |  %3u |  %2u  |\n",
		       h1->battle_soldier[i] == NULL ? 0 : h1->battle_soldier[i]->hp,
		       h1->battle_soldier[i] == NULL ? 0 : h1->battle_soldier[i]->attack_force,
		       h1->battle_soldier[i] == NULL ? 0 : h1->battle_soldier[i]->can_attack,
		       i + 1,
		       h2->battle_soldier[i] == NULL ? 0 : h2->battle_soldier[i]->hp,
		       h2->battle_soldier[i] == NULL ? 0 : h2->battle_soldier[i]->attack_force,
		       h2->battle_soldier[i] == NULL ? 0 : h2->battle_soldier[i]->can_attack
		       );
		buf += n;
	}
	n = sprintf(buf, "===========================================================\n");
	buf += n;
	return buf-str;
}

int
print_handcard(struct hero *h, char *str)
{
	int i, n;
	char *buf = str;
	n = sprintf(buf, "====================================\n");
	buf += n;
	n = sprintf(buf, "|  NO. | TYPE |  HP  |  ATK | POWER|\n");
	buf += n;
	for (i = 0; i < h->n_hand_cards; i++) {
		n = sprintf(buf, "|  %2d  |   0  |  %3d |  %3u |  %2u  |\n",
		       i+1, h->hand_cards[i]->soldier.hp,
		       h->hand_cards[i]->soldier.attack_force,
		       h->hand_cards[i]->soldier.power);
		buf += n;
	}
	n = sprintf(buf, "====================================\n");
	buf += n;
	return buf-str;
}

char statbuf[5000];
char *
print_stat(struct hero *h, int *len)
{
	int n;
	char *buf = statbuf;
	struct hero *oh = get_other_hero(h);
	int t = (h - oh + 1)/2;

	buf[0] = 0;
	buf[1] = 5;
	buf[2] = '\n';
	buf += 3;
	n = print_hero(h, buf);
	buf += n;
	if ((turn & 0x1) == t) {
		buf--;
		buf[0] = ' ';
		buf[1] = '<';
		buf[2] = '-';
		buf[3] = '-';
		buf[4] = '-';
		buf[5] = '\n';
		buf += 6;
	}
	n = print_hero(oh, buf);
	buf += n;
	if ((turn & 0x1) != t) {
		buf--;
		buf[0] = ' ';
		buf[1] = '<';
		buf[2] = '-';
		buf[3] = '-';
		buf[4] = '-';
		buf[5] = '\n';
		buf += 6;
	}
	n = print_battlefield(h, oh, buf);
	buf += n;
	n = print_handcard(h, buf);
	*len = buf + n - statbuf;
	return statbuf;
}

void
send_game_result()
{
	int i;
	char buf[4];
	buf[0] = 0;
	for (i = 0; i < 2; i++) {
		if (heros[i].hp <= 0) {
			buf[1] = 8;
			write(sock[i], buf, 4);
		} else {
			buf[1] = 9;
			write(sock[i], buf, 4);
		}
	}
}
void
close_conn()
{
	close(sock[1]);
	close(sock[0]);
}

int
checkover()
{
	int i;
	for (i = 0; i < 2; i++) {
		if (heros[i].hp <= 0) {
			send_game_result();
			close_conn();
			return 1;
		}
	}
	return 0;
}

int
attack(struct hero *h, unsigned our_pos, unsigned opponent_pos)
{
	struct hero *other;
	struct soldier *other_s;
	if (our_pos > 7 || opponent_pos > 7)
		return -1;
	other = get_other_hero(h);
	if (our_pos == 0) {
		if (h->attack_force == 0 || h->can_attack == 0)
			return 0;
		if (opponent_pos == 0) // XXX
			other->hp -= h->attack_force;
		else {
			other_s = other->battle_soldier[opponent_pos-1];
			if (other_s == NULL)
				return 0;
			h->hp -= other_s->attack_force;
			other_s->hp -= h->attack_force;
			if (other_s->hp <= 0)
				other->battle_soldier[opponent_pos-1] = NULL;
		}
		h->can_attack --;
	} else {
		struct soldier *s = h->battle_soldier[our_pos-1];
		if (s == NULL || s->can_attack == 0 || s->attack_force == 0)
			return 0;
		if (opponent_pos == 0)
			other->hp -= s->attack_force;
		else {
			other_s = other->battle_soldier[opponent_pos-1];
			s->hp -= other_s->attack_force;
			other_s->hp -= s->attack_force;
			if (s->hp <= 0)
				h->battle_soldier[our_pos-1] = NULL;
			if (other_s->hp <= 0)
				other->battle_soldier[opponent_pos-1] = NULL;
		}
		s->can_attack --;
	}
	return checkover();
}

void
soldier_init(struct soldier *s, int hp, int atk, int pow)
{
	s->hp = hp;
	s->attack_force = atk;
	s->power = pow;
}
/*
 * XXX this should init from client.
 * */

void
card_lib_init(struct card_lib *cl)
{
	int i;
	cl->get_next_card = get_next_card;
	for (i = 0; i < CARDMAX; i++) {
		soldier_init(&cl->cards[i].soldier, i%8 + 1, i%8 + 1, i%8 + 1);
	}
}

void
init_hero(int n)
{
	struct hero *h = &heros[n];
	h->hp = 30;
	h->max_power = 0;
	card_lib_init(&h->cl);
	h->get_card = get_card;
	h->push_soldier = push_soldier;
	h->attack = attack;
}


void
get_stat(struct hero *h)
{
	int fd = sock[h - heros];
	int n;
	char *str = print_stat(h, &n);
//	printf("%s\n", str+2);
//	printf("write fd\n");
	write(fd, str, n);
}
/*
 * protocol:
 * 0: version
 * 1: cmd
 * 2: arg1
 * 3: arg2
 * */
int
cmd(char *buf, int n, struct hero *h, int turn)
{
	char cmd;
	unsigned arg1;
	unsigned arg2;
	int ret = 0;

	if (n < 4)
		return -1;
	cmd = buf[1];
	arg1 = (unsigned)buf[2];
	arg2 = (unsigned)buf[3];
	if (cmd > 10 || cmd == 0)
		return -1;
	switch (cmd) {
	/* push soldier */
	case 1:
		if (turn == 0) // not our turn.
			return 0;
		h->push_soldier(h, arg1 - 1, arg2 - 1);
		break;
	/* attack */
	case 2:
		if (turn == 0)
			return 0;
		ret = h->attack(h, arg1, arg2);
		break;
	/* turn over */
	case 3:
		if (turn == 0)
			return 0;
		ret = 2;
		break;
	/* print */
	case 4:
		get_stat(h);
		break;
	default:
		break;
	}
	return ret;
}

void
notify_client_turn()
{
	int fd = sock[turn & 0x1];
	char buf[10] = {0, 1,};
	printf("Now it's round %d\n", turn);
	printf("It's %d 's turn\n", turn & 0x1);
	/*
	int i;
	printf("fd: %d, %d\n", turn&0x1, fd);
	for (i =0; i<4; i++)
		printf("%d ", buf[i]);
	*/
	write(fd, buf, 4);
}

void
setblock(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1) {
		perror("fcntl");
		exit(1);
	}
}
void
setnonblock(int fd)
{
	int flags = fcntl(fd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1) {
		perror("fcntl");
		exit(1);
	}
}

void
begin()
{
	int n, ret;
	char buf[1000];
	struct hero *h;
	struct fd_set rfds;
	int max_fd;
	int i;
	h = &heros[0];
	for (i = 0; i < 3; i++)
		h->get_card(h);
	h = &heros[1];
	for (i = 0; i < 4; i++)
		h->get_card(h);
	h = &heros[turn & 0x1];
	h->get_card(h);
	h->max_power ++;
	h->power = h->max_power;
	notify_client_turn();
	printf("--begin--\n");
	max_fd = sock[0] > sock[1] ? sock[0] : sock[1];
	while (1) {
again:
		FD_ZERO(&rfds);
		FD_SET(sock[0], &rfds);
		FD_SET(sock[1], &rfds);
		//get_stat(h);
		//n = read(sock[turn & 0x1], buf, sizeof(buf));
		
		ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EAGAIN)
				goto again;
			else goto again;
		} else {
			if (FD_ISSET(sock[0], &rfds)) {
				n = read(sock[0], buf, sizeof(buf));
				ret = cmd(buf, n, &heros[0], 1-(0^(turn&0x1)));
				if (ret == 1) { /*game over*/
					break;
				} else if (ret == 2) {
					turn++;
					h = &heros[turn & 0x1];
					h->get_card(h);
					if (h->max_power < 10)
						h->max_power++;
					h->power = h->max_power;
					for (i = 0; i < 7; i++) {
						if (h->battle_soldier[i] != NULL)
							h->battle_soldier[i]->can_attack = 1;
					}
					notify_client_turn();
					goto again;
				}
			}
			if (FD_ISSET(sock[1], &rfds)) {
				n = read(sock[1], buf, sizeof(buf));
				ret = cmd(buf, n, &heros[1], 1-(1^(turn&0x1)));
				if (ret == 1) { /*game over*/
					break;
				} else if (ret == 2) {
					turn++;
					h = &heros[turn & 0x1];
					h->get_card(h);
					if (h->max_power < 10)
						h->max_power++;
					h->power = h->max_power;
					for (i = 0; i < 7; i++) {
						if (h->battle_soldier[i] != NULL)
							h->battle_soldier[i]->can_attack = 1;
					}
					notify_client_turn();
					goto again;
				}
			}
		}
	}
}

#define PORT 8888
int
main(int argc, char *argv[])
{
	int servfd, newfd;
	struct sockaddr_in sin;
	int i;
	socklen_t addrlen;
	servfd = socket(PF_INET, SOCK_STREAM, 0);
	if (servfd < 0) {
		perror("socket");
		exit(1);
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);
	if (bind(servfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("socket");
		exit(1);
	}
	if (listen(servfd, 2) == -1) {
		perror("listen");
		exit(1);
	}
	while (1) {
		addrlen = sizeof(sin);
		newfd = accept(servfd, (struct sockaddr *)&sin, &addrlen);
		if (newfd < 0) {
			perror("accept");
		}
		for (i = 0; i < 2; i++) {
			if (sock[i] == 0) {
				sock[i] = newfd;
				break;
			}
		}
		for (i = 0; i < 2; i++) {
			if (sock[i] == 0)
				break;
		}
		if (i == 2)
			break;
	}
	//setnonblock(sock[0]);
	//setnonblock(sock[1]);
	init_hero(0);
	init_hero(1);
	begin();
	return 0;
}
