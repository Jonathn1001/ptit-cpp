CXX       := g++
CXXFLAGS  := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
SRC       := $(wildcard src/*.cpp)
OBJ       := $(SRC:.cpp=.o)
APP       := apex

TEST_SRC  := $(wildcard test/*.cpp) $(filter-out src/main.cpp,$(SRC))
TEST_BIN  := apex_tests

.PHONY: all test clean run

all: $(APP)

$(APP): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(APP)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -Itest $(TEST_SRC) -o $(TEST_BIN)

run: $(APP)
	./$(APP)

clean:
	rm -f $(APP) $(TEST_BIN) src/*.o test/*.o
