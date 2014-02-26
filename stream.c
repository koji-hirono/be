/*
 * Copyright (c) 2014 Koji Hirono <koji.hirono@gmail.com>
 *
 * Licensed under the X11/MIT License.
 * You may obtain a copy of the License at
 * http://www.opensource.org/licenses/mit-license.php
 */
#include <stdio.h>

#include <unistd.h>

#include "stream.h"

void
stream_init(Stream *stream, int fd, const char *s)
{
	stream->c = 0;

	if (s == NULL) {
		stream->type = STREAM_IO;
		stream->u.fd = fd;
	} else {
		stream->type = STREAM_STRING;
		stream->u.str.s = s;
		stream->u.str.p = s;
	}
}

int
stream_getc(Stream *stream)
{
	unsigned char c;

	if (stream->type == STREAM_IO) {
		if (read(stream->u.fd, &c, 1) != 1)
			stream->c = EOF;
		else
			stream->c = c;
	} else if (stream->type == STREAM_STRING) {
		if (*stream->u.str.p == '\0')
			stream->c = EOF;
		else
			stream->c = *stream->u.str.p++ & 0xff;
	} else {
		stream->c = EOF;
	}

	return stream->c;
}

int
stream_peekc(Stream *stream)
{
	return stream->c;
}
