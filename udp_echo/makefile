#
# Makefile for tcp_echo
#
MYLIBDIR=../mynet
MYLIB=-lmynet
CFLAGS=-I${MYLIBDIR} -L${MYLIBDIR}

all: udp_echo_server udp_echo_client

udp_echo_server: udp_echo_server.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

udp_echo_client: udp_echo_client.o
	${CC} ${CFLAGS} -o $@ $^ ${MYLIB}

clean:
	${RM} *.o udp_echo_server udp_echo_client *~