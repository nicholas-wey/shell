CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror -D_GNU_SOURCE -std=gnu99

PROMPT = -DPROMPT

CC = gcc
EXECS = 33sh 33noprompt
DEPENDENCIES = sh.c jobs.c

.PHONY: all clean

all: $(EXECS)

33sh: $(DEPENDENCIES)
	$(CC) $(CFLAGS) $(PROMPT) $^ -o $@

33noprompt: $(DEPENDENCIES)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(EXECS)
