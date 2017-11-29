
BIN  := shell
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS := -g
LDLIBS :=

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDLIBS)

%.o : %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o $(BIN) tmp *.plist valgrind.out

valgrind: $(BIN)
	valgrind -q --leak-check=full --log-file=valgrind.out ./$(BIN)

.PHONY: clean test
