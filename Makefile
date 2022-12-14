CC      = gcc
CFLAGS  = -g
TARGET1 = oss
OBJS1   = master.o
TARGET2 = user
OBJS2   = user.o

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1) -pthread

$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2) -pthread

master.o: master.c
	$(CC) $(CFLAGS) -c master.c

user.o: user.c
	$(CC) $(CFLAGS) -c user.c


clean:
	/bin/rm -f *.o $(TARGET)

