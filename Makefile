CC=gcc
CFLAGS=`curl-config --cflags`
LDFLAGS=`curl-config --libs`

TARGET=peeves-accept peeves-cron

peeves: $(TARGET)

peeves-accept: peeves-accept.c
	$(CC) -o $@ peeves-accept.c

peeves-cron: peeves-cron.c
	$(CC) $(CFLAGS) -o $@ peeves-cron.c $(LDFLAGS)

clean:
	rm $(TARGET)
