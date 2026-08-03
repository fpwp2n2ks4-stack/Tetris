CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wformat=2 -Wuninitialized -Wimplicit-fallthrough -O2 -Isrc
LDLIBS   := -lncurses

BUILD    := build

CORE_SRCS := \
	src/game/Piece.cpp \
	src/game/Game.cpp \
	src/game/Randomizer.cpp \
	src/storage/ScoreStore.cpp
UI_SRCS   := src/ui/NcursesUi.cpp
MAIN_SRC  := src/main.cpp
TEST_SRC  := tests/test_main.cpp

CORE_OBJS := $(patsubst src/%.cpp,$(BUILD)/%.o,$(CORE_SRCS))
UI_OBJS   := $(patsubst src/%.cpp,$(BUILD)/%.o,$(UI_SRCS))

all: tetris

tetris: $(CORE_OBJS) $(UI_OBJS) $(BUILD)/main.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

tests: $(CORE_OBJS) $(BUILD)/test_main.o
	$(CXX) $(CXXFLAGS) -o tetris_tests $^ $(LDLIBS)

$(BUILD)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/test_main.o: $(TEST_SRC)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD) tetris tetris_tests tetris.o

.PHONY: all tests clean
