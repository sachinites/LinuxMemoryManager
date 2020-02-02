CC=gcc
CFLAGS=-g
TARGET:testapp.exe libmm.a
OUTFILES=testapp.exe libmm.a
EXTERNAL_LIBS=
OBJS=gluethread/glthread.o mm.o

testapp.exe:testapp.o ${OBJS}
	${CC} ${CFLAGS} testapp.o ${OBJS} -o testapp.exe ${EXTERNAL_LIBS}
testapp.o:testapp.c
	${CC} ${CFLAGS} -c testapp.c -o testapp.o
gluethread/glthread.o:gluethread/glthread.c
	${CC} ${CFLAGS} -c -I gluethread gluethread/glthread.c -o gluethread/glthread.o
mm.o:mm.c
	${CC} ${CFLAGS} -c mm.c -o mm.o
libmm.a:${OBJ}
	ar rs libmm.a ${OBJ}
clean:
	rm -f testapp.o
	rm -f ${OUTFILES}
	rm -f ${OBJS}
