/*
 * Copyright (c) 2014 Koji Hirono <koji.hirono@gmail.com>
 *
 * Licensed under the X11/MIT License.
 * You may obtain a copy of the License at
 * http://www.opensource.org/licenses/mit-license.php
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"

void
buf_init(Buf *b)
{
	b->size = 0;
	b->len = 0;
	b->c = NULL;
}

void
buf_free(Buf *b)
{
	free(b->c);
}

int
buf_expand(Buf *b, size_t need)
{
	size_t size;
	void *p;

	if (b->size == 0)
		b->size = 1;
	for (size = b->size; size < need; size *= 2)
		;

	if ((p = realloc(b->c, size)) == NULL)
		return -1;

	b->c = p;
	b->size = size;

	return 0;
}

int
buf_pushc(Buf *b, int c)
{
	if (b->len + 1 > b->size)
		if (buf_expand(b, b->len + 1) != 0)
			return -1;

	b->c[b->len++] = c;

	return 0;
}

int
buf_pushoct(Buf *b, void *oct, size_t len)
{
	if (b->len + len > b->size)
		if (buf_expand(b, b->len + len) != 0)
			return -1;

	memcpy(b->c + b->len, oct, len);
	b->len += len;

	return 0;
}

void
buf_dump(Buf *b, FILE *fp)
{
	size_t i;

	fprintf(fp, "size: %zu\n", b->size);
	fprintf(fp, "len:  %zu\n", b->len);
	for (i = 0; i < b->len; i++)
		fprintf(fp, " %02x", b->c[i]);
	fprintf(fp, "\n");
}
