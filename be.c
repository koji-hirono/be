/*
 * Fixed Binary Editor
 *
 * Copyright (c) 2014 Koji Hirono <koji.hirono@gmail.com>
 *
 * Licensed under the X11/MIT License.
 * You may obtain a copy of the License at
 * http://www.opensource.org/licenses/mit-license.php
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <unistd.h>
#include <fcntl.h>

#include "buf.h"
#include "stream.h"

typedef struct Region Region;
typedef struct Action Action;
typedef struct Env Env;
typedef struct Cmd Cmd;

enum {
	CMD_OK,
	CMD_ERROR,
	CMD_QUIT
};

struct Region {
	int n;
	uintmax_t addr[2];
};

struct Action {
	Region region;
	Buf b;
};

struct Env {
	Buf fname;
	int nact;
	Action *act;
	uintmax_t cur;
	uintmax_t end;
	int width;
	int silent;
};

struct Cmd {
	int ch;
	int (*exec)(Env *, Region *, Stream *);
};

void usage(const char *);
void env_init(Env *);
void region_init(Region *);
int parse(Env *, Stream *);
int parse_region(Env *, Region *, Stream *);
int parse_address(Env *, Region *, Stream *);
Cmd *cmd_find(int);
int cmd_change(Env *, Region *, Stream *);
int cmd_edit(Env *, Region *, Stream *);
int cmd_print(Env *, Region *, Stream *);
int cmd_quit(Env *, Region *, Stream *);
int cmd_write(Env *, Region *, Stream *);
int cmd_search(Env *, Region *, Stream *);
int cmd_print_addr(Env *, Region *, Stream *);
char *get_fname(Stream *, Buf *);
void hexdump(Env *, Region *, Stream *);
int input_data(Buf *, Stream *, int);
int get_width(uintmax_t);
Action *action_find(Env *, uintmax_t);
int action_at(Action *, uintmax_t);

Cmd cmdtbl[] = {
	{'c', cmd_change},
	{'e', cmd_edit},
	{'p', cmd_print},
	{'q', cmd_quit},
	{'w', cmd_write},
	{'/', cmd_search},
	{'=', cmd_print_addr},
	{'\0', NULL}
};

int
main(int argc, char **argv)
{
	const char *progname;
	const char *cmd;
	int silent;
	Stream s;
	Stream p;
	Env env;
	int opt;
	int r;

	progname = argv[0];
	cmd = NULL;
	silent = 0;
	while ((opt = getopt(argc, argv, "e:s")) != -1) {
		switch (opt) {
		case 'e':
			cmd = optarg;
			break;
		case 's':
			silent = 1;
			break;
		default:
			usage(progname);
			return EXIT_FAILURE;
		}
	}
	argc -= optind;
	argv += optind;

	env_init(&env);
	env.silent = silent;

	if (argc > 0) {
		stream_init(&p, -1, *argv);
		cmd_edit(&env, NULL, &p);
	}

	if (cmd)
		stream_init(&s, -1, cmd);
	else
		stream_init(&s, 0, NULL);

	do {
		if ((r = parse(&env, &s)) == CMD_ERROR)
			printf("?\n");
	} while (r != CMD_QUIT && stream_peekc(&s) != EOF);

	return EXIT_SUCCESS;
}

void
usage(const char *progname)
{
	printf("usage: %s [-e COMMANDS] [-s] [FILE]\n", progname);
}

void
env_init(Env *env)
{
	buf_init(&env->fname);
	env->nact = 0;
	env->act = NULL;
	env->cur = 0;
	env->end = 0;
	env->width = 0;
	env->silent = 0;
}

void
region_init(Region *region)
{
	region->n = 0;
	region->addr[0] = 0;
	region->addr[1] = 0;
}

int
parse(Env *env, Stream *s)
{
	Region region;
	Cmd *cmd;

	region_init(&region);
	if (parse_region(env, &region, s) != 0)
		return CMD_ERROR;

	if ((cmd = cmd_find(stream_peekc(s))) == NULL)
		return CMD_ERROR;

	return cmd->exec(env, &region, s);
}

int
parse_region(Env *env, Region *region, Stream *s)
{
	int c;

	if (parse_address(env, region, s) != 0)
		return -1;

	c = stream_peekc(s);
	if (c == ',') {
		if (parse_address(env, region, s) != 0)
			return -1;
		if (region->n == 1)
			return -1;
		if (region->n == 0) {
			region->addr[0] = 0;
			region->addr[1] = env->end;
			region->n = 2;
			return 0;
		}

		if (region->addr[1] < region->addr[0])
			return -1;
	} else if (c == ';') {
		region->addr[0] = env->cur;
		region->addr[1] = env->end;
		region->n = 2;
		stream_getc(s);
	}

	return 0;
}

int
parse_address(Env *env, Region *region, Stream *s)
{
	uintmax_t *addr;
	uintmax_t n;
	int sign;
	int match;
	int c;

	if (region->n >= 2)
		return -1;

	addr = region->addr + region->n;

	while (isspace(c = stream_getc(s)))
		;

	*addr = env->cur;
	match = 0;
	sign = 0;
	for (;;) {
		if (isdigit(c)) {
			n = 0;
			do {
				n = n * 10 + (c - '0');
				c = stream_getc(s);
			} while (isdigit(c));
			if (sign > 0) {
				*addr += n;
				if (*addr > env->end)
					return -1;
			} else if (sign < 0) {
				if (n > *addr)
					return -1;
				*addr -= n;
			} else {
				*addr = n;
				if (*addr > env->end)
					return -1;
			}
			sign = 0;
			match = 1;
		} else if (c == '$') {
			*addr = env->end;
			sign = 0;
			c = stream_getc(s);
			match = 1;
		} else if (c == '.') {
			*addr = env->cur;
			sign = 0;
			c = stream_getc(s);
			match = 1;
		} else if (c == '-') {
			sign--;
			c = stream_getc(s);
			match = 1;
		} else if (c == '+') {
			sign++;
			c = stream_getc(s);
			match = 1;
		} else {
			break;
		}
	}

	if (match) {
		if (sign < 0 && (uintmax_t)-sign > *addr)
			return -1;
		*addr += sign;
		if (*addr > env->end)
			return -1;
		region->n++;
	}

	return 0;
}

Cmd *
cmd_find(int ch)
{
	Cmd *cmd;

	for (cmd = cmdtbl; cmd->ch != '\0'; cmd++)
		if (cmd->ch == ch)
			return cmd;

	return NULL;
}

/* (.)c */
int
cmd_change(Env *env, Region *region, Stream *p)
{
	Action *act;

	if (region->n < 1)
		region->addr[0] = env->cur;

	if ((act = realloc(env->act, sizeof(Action) * (env->nact + 1))) == NULL)
		return CMD_ERROR;
	env->act = act;
	act = env->act + env->nact;

	act->region.addr[0] = region->addr[0];
	buf_init(&act->b);
	if (input_data(&act->b, p, '.') != 0)
		return CMD_ERROR;
	act->region.addr[1] = region->addr[0] + act->b.len - 1;
	act->region.n = 2;

	env->nact++;

	return CMD_OK;
}

/* e file */
int
cmd_edit(Env *env, Region *region, Stream *p)
{
	struct stat st;

	if (env->fname.len != 0) {
		buf_free(&env->fname);
		buf_init(&env->fname);
	}

	if (get_fname(p, &env->fname) == NULL)
		return CMD_ERROR;

	if (stat((char *)env->fname.c, &st) != 0)
		return CMD_ERROR;

	env->end = st.st_size;
	env->width = get_width(env->end);

	if (!env->silent)
		printf("%ju\n", env->end);

	if (env->end > 0)
		env->end--;
	env->cur = env->end;

	return CMD_OK;
}

/* (0,$)/search-string */
int
cmd_search(Env *env, Region *region, Stream *p)
{
	unsigned char *t;
	unsigned char *e;
	Action *act;
	Stream s;
	Buf ptn;
	uintmax_t i;
	int fd;
	int c;

	if (env->fname.len == 0)
		return CMD_ERROR;

	if ((fd = open((char *)env->fname.c, O_RDONLY)) == -1)
		return CMD_ERROR;

	buf_init(&ptn);
	if (input_data(&ptn, p, '\n') != 0) {
		close(fd);
		return CMD_ERROR;
	}

	if (region->n < 1)
		region->addr[0] = 0;
	if (region->n < 2)
		region->addr[1] = env->end;
	region->n = 2;

	t = ptn.c;
	e = ptn.c + ptn.len;
	stream_init(&s, fd, NULL);
	for (i = region->addr[0]; i <= region->addr[1]; i++) {
		c = stream_getc(&s);
		if ((act = action_find(env, i)) != NULL)
			c = action_at(act, i);
		if (*t == c) {
			t++;
			if (t >= e) {
				printf("%0*jx\n", env->width, (intmax_t)i);
				break;
			}
		} else {
			t = ptn.c;
			if (*t == c)
				t++;
		}
	}

	buf_free(&ptn);
	close(fd);

	return CMD_OK;
}

/* (.,.)p */
int
cmd_print(Env *env, Region *region, Stream *p)
{
	Stream s;
	int fd;

	if (region->n < 1)
		region->addr[0] = env->cur;
	if (region->n < 2)
		region->addr[1] = region->addr[0];
	region->n = 2;

	if (env->fname.len == 0)
		return CMD_ERROR;

	if ((fd = open((char *)env->fname.c, O_RDONLY)) == -1)
		return CMD_ERROR;

	if (lseek(fd, region->addr[0], SEEK_SET) == (off_t)-1) {
		close(fd);
		return CMD_ERROR;
	}

	stream_init(&s, fd, NULL);

	hexdump(env, region, &s);

	close(fd);

	env->cur = region->addr[1];

	return CMD_OK;
}

/* q */
int
cmd_quit(Env *env, Region *region, Stream *p)
{
	return CMD_QUIT;
}

/* w */
int
cmd_write(Env *env, Region *region, Stream *p)
{
	uintmax_t total;
	Action *act;
	int fd;
	int i;

	if (env->fname.len == 0)
		return CMD_ERROR;

	if ((fd = open((char *)env->fname.c, O_RDWR)) == -1)
		return CMD_ERROR;

	total = 0;
	for (i = 0; i < env->nact; i++) {
		act = env->act + i;
		if (lseek(fd, act->region.addr[0], SEEK_SET) == (off_t)-1) {
			close(fd);
			return CMD_ERROR;
		}
		if (write(fd, act->b.c, act->b.len) != (int)act->b.len) {
			close(fd);
			return CMD_ERROR;
		}
		total += act->b.len;
	}

	close(fd);

	for (i = 0; i < env->nact; i++) {
		act = env->act + i;
		buf_free(&act->b);
	}
	free(env->act);
	env->act = NULL;
	env->nact = 0;

	if (!env->silent)
		printf("%ju\n", total);

	return CMD_OK;
}

/* ($)= */
int
cmd_print_addr(Env *env, Region *region, Stream *p)
{
	if (region->n < 1)
		region->addr[0] = env->end;

	printf("%0*jx\n", env->width, region->addr[0]);

	return CMD_OK;
}

char *
get_fname(Stream *s, Buf *b)
{
	int over;
	int c;

	while (isblank(c = stream_getc(s)))
		;

	over = 0;
	do {
		if (!over && buf_pushc(b, c) != 0)
			over = 1;
		c = stream_getc(s);
	} while (isgraph(c));

	if (over || b->len == 0)
		return NULL;

	buf_pushc(b, '\0');

	return (char *)b->c;
}

void
hexdump(Env *env, Region *region, Stream *s)
{
	Action *act;
	char t[17];
	char *p;
	uintmax_t i;
	uintmax_t end;
	uintmax_t size;
	int c;
	int over;

	size = region->addr[1] + 1;
	end = (size + 0xf) & ~0xf;
	over = 0;
	p = t;
	printf("%-*.*s  0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f\n",
	       env->width, env->width, "Address");
	for (i = region->addr[0] & ~0xf; i < end; i++) {
		if ((i & 0xf) == 0)
			printf("%0*jx  ", env->width, i);

		if (!over && i >= region->addr[0] && i < size) {
			if ((c = stream_getc(s)) == EOF)
				over = 1;
			if ((act = action_find(env, i)) != NULL)
				c = action_at(act, i);
		}

		if (over || i < region->addr[0] || i >= size) {
			printf("   ");
			*p++ = ' ';
		} else {
			printf("%02x ", c);
			*p++ = isprint(c) ? c : '.';
		}

		if ((i & 0x7) == 0x7)
			printf(" ");

		if ((i & 0xf) == 0xf) {
			*p = '\0';
			printf("|%s|\n", t);
			p = t;
		}
	}

	printf("%0*jx\n", env->width, size);
}

int
xtoi(int c)
{
	if (isdigit(c))
		return c - '0';
	if (islower(c))
		return 10 + c - 'a';
	if (isupper(c))
		return 10 + c - 'A';

	return 0;
}

int
input_data(Buf *b, Stream *s, int t)
{
	int c, n, x;

	x = 0;
	n = 0;
	while ((c = stream_getc(s)) != t && c != EOF) {
		if (isspace(c))
			continue;

		if (!isxdigit(c)) {
			buf_free(b);
			return -1;
		}

		x |= xtoi(c);

		if (n == 0)
			x <<= 4;
		else {
			if (buf_pushc(b, x) != 0) {
				buf_free(b);
				return -1;
			}
			x = 0;
		}
		n = (n + 1) & 1;
	}

	return 0;
}

Action *
action_find(Env *env, uintmax_t addr)
{
	Action *act;
	int i;

	for (i = env->nact - 1; i >= 0; i--) {
		act = env->act + i;
		if (addr >= act->region.addr[0] && addr <= act->region.addr[1])
			return act;
	}

	return NULL;
}

int
action_at(Action *act, uintmax_t addr)
{
	uintmax_t offset;

	if ((offset = addr - act->region.addr[0]) >= (uintmax_t)act->b.len)
		return -1;

	return act->b.c[offset] & 0xff;
}

int
get_width(uintmax_t size)
{
	int width;

	width = 8;
	for (size >>= 32; size != 0; size >>= 8)
		width += 2;

	return width;
}
