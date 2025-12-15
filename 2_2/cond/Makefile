BUILD_DIR := build

INCLUDE_DIR := "."
SRCS := $(notdir $(wildcard *.c))

LIBS := -pthread

override CFLAGS += -g -Wall -Wextra -Wpedantic

all: default delay

default: ${SRCS}
	mkdir -p ${BUILD_DIR}
	${CC} ${CFLAGS} -I${INCLUDE_DIR} ${SRCS} ${LIBS} -o ${BUILD_DIR}/$@

delay: ${SRCS}
	mkdir -p ${BUILD_DIR}
	${CC} ${CFLAGS} -D"DELAY=1" -I${INCLUDE_DIR} ${SRCS} ${LIBS} -o ${BUILD_DIR}/$@

.PHONY: clean

clean:
	rm -rf ${BUILD_DIR}
