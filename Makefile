PROG=sync
SRCS=sync.c
MAN=

CFLAGS+=-g -Wall -I./lib/opt/include
LDFLAGS+=-L./lib/opt/lib/ -lssh2

.include <bsd.prog.mk>
