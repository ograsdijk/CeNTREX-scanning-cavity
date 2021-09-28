CFLAGS  = -g -std=gnu11 -Wall -Werror
# CFLAGS  = -g -std=gnu11 -Wall
CFLAGS += -I/opt/redpitaya/include
LDFLAGS = -L/opt/redpitaya/lib
LDLIBS =  -lpthread -lrp -lm

DEPS = communication.h peak_find.h
OBJS = main.o communication.o peak_find.o

# %.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)

# hellomake: $(OBJ)
# 	$(CC) -o $@ $^ $(CFLAGS)

all:
	gcc $(CFLAGS) -o main.out main.c communication.c peak_find.c daq.c pid.c $(LDLIBS) $(LDFLAGS)

clean:
	$(RM) *.o
	$(RM) $(OBJS)
	$(RM) *.out