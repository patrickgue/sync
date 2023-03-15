PROG=sync
SRCS=sync.c test.c
MAN=

CFLAGS+=-g -Wall -I./lib/opt/include
LDFLAGS+=-L./lib/opt/lib/ -lssh2

.include <bsd.prog.mk>
