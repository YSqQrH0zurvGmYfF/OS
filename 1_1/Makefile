TARGET = thread-example
SRCS = thread.c

CFLAGS+=-g -Wall -Wextra -pedantic -Wno-format
LIBS=-pthread
INCLUDE_DIR="."

all: ${TARGET}

${TARGET}: ${SRCS}
	${CC} ${CFLAGS} -I${INCLUDE_DIR} ${SRCS} ${LIBS} -o ${TARGET}

clean:
	rm -f ${BUILD_DIR}
