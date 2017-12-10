OBJS=libmain.o midiparser.o play_event.o
INSTALLLIB=/lib
LDFLAGS=-shared -fPIC
CFLAGS=-O -fPIC
#LDFLAGS=-Bdynamic
AROPTS=rc
AR=ar

all:	
	sh ./compile.sh $(INSTALLLIB) "$(CC)" "$(CFLAGS)" "$(MAKE)"

install: all
	cp libOSSlib.* $(INSTALLLIB)
	cp drums.o3 std.o3 /etc

static: libOSSlib.a
	cp libOSSlib.a $(INSTALLLIB)

libOSSlib.so:	$(OBJS)
	${CC} $(LDFLAGS) -o libOSSlib.so $(OBJS)

dep:

clean:
	rm -f *.o *.so x y z core *.a

libOSSlib.a:	$(OBJS)
	$(AR) $(AROPTS) libOSSlib.a $(OBJS)
	#-ranlib libOSSlib.a
