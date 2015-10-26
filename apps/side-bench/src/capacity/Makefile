SRCS=pageset.c probe.c timestats.c capacity.c affinity.c trojan.c
PROJ=capacity
CFLAGS=-std=gnu99 -g -O2
#CFLAGS=-std=gnu99 -g 
LDLIBS=
LDFLAGS=

OBJS=$(SRCS:.c=.o)


all: $(PROJ)


capacity: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

pageset.o: pageset.h

probe.o: probe.h pageset.h sysinfo.h

timestats.o: timestats.h

capacity.o: timestats.h probe.h sysinfo.h

evictionset.o: evictionset.h pageset.h probe.h

trojan.c: trojan.h affinity.h

clean:
	rm -f $(PROJ) $(OBJS) core.[0-9]*
