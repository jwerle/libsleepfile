SRC += ram.c sleepfile.c
DEPS += $(wildcard deps/*/*.c)
OBJECT = $(SRC:.c=.o) $(DEPS:.c=.o)

CFLAGS += -I deps/
CFLAGS += -I .
CFLAGS += -l m
CFLAGS += -g

libsleepfile.a: $(OBJECT)
	$(AR) crs $@ $^

%.o: %.c
	$(CC) $^ $(CFLAGS) -c -o $@

test: libsleepfile.a
test: test.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	$(RM) -f *.a *.o test
