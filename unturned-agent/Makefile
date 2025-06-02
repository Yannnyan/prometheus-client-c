.PHONY: clean

VERSION = $(shell cat ../VERSION)


GCC = /usr/bin/gcc
FLAGS = -g 
DEPS = client.h log.h
OBJ = client.o log.o main.o
LIBS = /usr/lib/libprom.so /usr/lib/libpromhttp.so 

PROM_DEB_PACKAGE = ../prom/libprom-dev-${VERSION}-Linux.deb
PROM_TGZ_PACKAGE = ../prom/libprom-dev-${VERSION}-Linux.tar.gz

PROMHTTP_DEB_PACKAGE = ../promhttp/libpromhttp-dev-${VERSION}-Linux.deb
PROMHTTP_TGZ_PACKAGE = ../promhttp/libpromhttp-dev-${VERSION}-Linux.tar.gz

all: unturned_agent

unturned_agent: ${OBJ} ${LIBS}
	gcc -g -o $@ ${FLAGS} -O1 -pthread ${OBJ} -lprom -lpromhttp -lmicrohttpd

#${PROM_DEB_PACKAGE}:
#	cd .. && ./auto build && ./auto package
#
#${PROMHTTP_DEB_PACKAGE}:
#	cd .. && ./auto build && ./auto package
#
#/usr/lib/libprom.so: ${PROM_DEB_PACKAGE}
#	dpkg -i --force-architecture --force-overwrite ${PROM_DEB_PACKAGE}
#
#/usr/lib/libpromhttp.so: ${PROMHTTP_DEB_PACKAGE}
#	dpkg -i  --force-architecture --force-overwrite ${PROMHTTP_DEB_PACKAGE}

%.o: %.c ${PROM_DEB_PACKAGE} ${PROMHTTP_DEB_PACKAGE}  ${LIBS} ${DEPS} 
	${GCC} ${FLAGS} -c $< -o $@

clean:
	-rm unturned_prom_client ${OBJ}
#	-rm ${OBJ} example ${PROM_DEB_PACKAGE} ${PROM_TGZ_PACKAGE} ${PROMHTTP_DEB_PACKAGE} ${PROMHTTP_TGZ_PACKAGE}
#	-dpkg -r libprom-dev
#	-dpkg -r libpromhttp-dev
