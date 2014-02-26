/*
 * Copyright (c) 2014 Koji Hirono <koji.hirono@gmail.com>
 *
 * Licensed under the X11/MIT License.
 * You may obtain a copy of the License at
 * http://www.opensource.org/licenses/mit-license.php
 */
#ifndef __BUF_H__
#define __BUF_H__

#include <stdio.h>

typedef struct Buf Buf;

struct Buf {
	size_t size;
	size_t len;
	unsigned char *c;
};

extern void buf_init(Buf *);
extern void buf_free(Buf *);
extern int buf_expand(Buf *, size_t);
extern int buf_pushc(Buf *, int);
extern int buf_pushoct(Buf *, void *, size_t);
extern void buf_dump(Buf *, FILE *);

#endif /* !__BUF_H__ */
