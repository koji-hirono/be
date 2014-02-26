PROG=be
SRCS=\
	be.c \
	stream.c \
	buf.c

OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.dep)

CC=gcc
CFLAGS=-Wall -W -Wno-unused-parameter -Werror

# for Linux
CPPFLAGS=-D_LARGEFILE_SOURCE
CPPFLAGS+=-D_FILE_OFFSET_BITS=64

all: $(PROG)

clean:
	-rm -f $(PROG) $(OBJS) $(DEPS) *~

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJS): %.o : %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(DEPS): %.dep : %.c
	@echo "===> Update" $@
	@$(CC) $(CPPFLAGS) -MM $< -o $@

ifneq ($(MAKECMDGLOALS), clean)
-include $(DEPS)
endif
