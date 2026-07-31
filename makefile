CC := clang++
SRC := main.cpp Terminal.cpp AnsiParser.cpp TerminalState.cpp PTY.cpp Grid.cpp Color.cpp UTF8Decoder.cpp
EXEC := main.out

LIBS := -lX11 -lGL -lXft -lXrender -lfontconfig -lfreetype
XFT_CFLAGS := $(shell pkg-config --cflags xft 2>/dev/null)

MODE ?= release

ifeq ($(MODE),debug)
    FLAGS := -std=c++17 -g -DDEBUG -O0 -Wall -Wextra
else
    FLAGS := -std=c++17 -DNDEBUG -O2 -Wall
endif

all:
	$(CC) $(SRC) $(FLAGS) $(XFT_CFLAGS) $(LIBS) -o $(EXEC)

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

run:
	./$(EXEC)

.PHONY: clean debug release run
clean:
	rm -f $(EXEC)
