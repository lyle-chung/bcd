
INSTALL=install

PWD :=${shell /bin/pwd}


### library
INCLUDES = -I${TOOLCHAIN_BASE}/include -I../../linux/include/ 


#### for BCD server
BCD = bcd
BCD_SRCS = ${BCD}.c bcd_linked.c flash.c crc32.c syslog.c 
BCD_OBJS = ${BCD_SRCS:.c=.o}

### for apps
CLIENT = bcc
CLIENT_SRCS = ${CLIENT}.c 
CLIENT_OBJS = ${CLIENT_SRCS:.c=.o}

### for API sample
API = api_sample
API_SRCS = ${API}.c 
API_OBJS = ${API_SRCS:.c=.o}

### for LIBRARY
LIBMSG = libmessage
LIBDIR = ../lib
BCD_LDFLAGS = -lpthread 
LDFLAGS = -L. -L${LIBDIR} -lmessage

### for ALL
ALL = ${LIBMSG} ${BCD} ${CLIENT} ${API}
ALL_SRCS = ${BCD_SRCS} ${CLIENT_SRCS} ${API_SRCS}
ALL_OBJS = ${ALL_SRCS:.c=.o}


#################################################################
#################################################################

all : ${ALL}

${BCD}:	${BCD_OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ $^ ${LDFLAGS} ${BCD_LDFLAGS}

${LIBMSG}:
	mkdir -p $(LIBDIR)
	${CC} ${CFLAGS} -c message.c
	${CC} ${CFLAGS} -c crc32.c
	${CC} ${CFLAGS} -c linked.c
	${AR} r ${LIBDIR}/${LIBMSG}.a message.o crc32.o linked.o

${CLIENT}:	${CLIENT_OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ $^ ${LDFLAGS} 

${API}:	${API_OBJS}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ $^ ${LDFLAGS} 

.c.o:   
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@ 

clean :
	rm -rf ${ALL} ${ALL_OBJS} core ${LIBMSG}.a message.o linked.o

install: all
	#strip
	@echo "striping.."
	@for i in ${BCD}; do \
		${STRIP} $$i; \
	done
	@for i in ${CLIENT}; do \
		${STRIP} $$i; \
	done
	#copy
	@echo "copying...."
	@for i in ${BCD}; do \
		cp $$i ${TARGETDIR}/org; \
	done
	@for i in ${CLIENT}; do \
		cp $$i ${TARGETDIR}/org; \
	done

