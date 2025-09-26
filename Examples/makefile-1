CC	= gcc
CFLAGS  = -g3 -Wall
TARGET1 = memchild
TARGET2 = memmain 

OBJS1	= memchild.o
OBJS2	= memmain.o
all:	$(TARGET1) $(TARGET2)

$(TARGET1):	$(OBJS1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJS1)

$(TARGET2):	$(OBJS2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJS2)

memchild.o:	memchild.c
	$(CC) $(CFLAGS) -c memchild.c 

memmain.o:	memmain.c
	$(CC) $(CFLAGS) -c memmain.c


clean:
	/bin/rm -f *.o $(TARGET1) $(TARGET2)
