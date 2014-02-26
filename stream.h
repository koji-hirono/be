/*
 * Copyright (c) 2014 Koji Hirono <koji.hirono@gmail.com>
 *
 * Licensed under the X11/MIT License.
 * You may obtain a copy of the License at
 * http://www.opensource.org/licenses/mit-license.php
 */
#ifndef __STREAM_H__
#define __STREAM_H__

typedef struct Stream Stream;

enum {
	STREAM_IO,
	STREAM_STRING
};

struct Stream {
	int type;
	int c;
	union {
		int fd;
		struct {
			const char *s;
			const char *p;
		} str;
	} u;
};

extern void stream_init(Stream *, int, const char *);
extern int stream_getc(Stream *);
extern int stream_peekc(Stream *);

#endif /* !__STREAM_H__ */
