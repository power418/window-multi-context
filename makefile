CC := clang++
SRC := main.cpp Terminal.cpp AnsiParser.cpp TerminalState.cpp PTY.cpp Grid.cpp Color.cpp UTF8Decoder.cpp stb_image.cpp
EXEC := main.out

LIBS := -lX11 -lGL -lXft -lXrender -lfontconfig -lfreetype
XFT_CFLAGS := $(shell pkg-config --cflags xft 2>/dev/null)

CXXFLAGS := -std=c++17 -Wall
MODE ?= release
BUILD_DIR := build/$(MODE)
OBJ := $(SRC:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJ:.o=.d)

ifeq ($(MODE),debug)
    CXXFLAGS += -g -O0 -DDEBUG -Wextra -Wpedantic
else
    CXXFLAGS += -DNDEBUG -O3 -flto
    LDFLAGS := -flto
endif

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CC) $(CXXFLAGS) $(XFT_CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

run: $(EXEC)
	./$(EXEC)

run-debug:
	$(MAKE) MODE=debug run

run-release:
	$(MAKE) MODE=release run

clean:
	rm -rf build

clean-all: clean
	rm -f $(EXEC)

-include $(DEPS)

.PHONY: all debug release run run-debug run-release clean clean-all
