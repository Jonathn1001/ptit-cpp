CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
SRC := $(wildcard src/*.cpp)
APP := apex
TEST_SRC := $(wildcard test/*.cpp) $(filter-out src/main.cpp,$(SRC))
TEST_BIN := apex_tests
WEB_DIR := web
WEB_PORT ?= 8080

.PHONY: all test clean run web web-serve

all: $(APP)

$(APP): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(APP)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -Itest $(TEST_SRC) -o $(TEST_BIN)

run: $(APP)
	./$(APP)

web:
	@echo "Static web dashboard is in $(WEB_DIR)/"
	@echo "Open $(WEB_DIR)/index.html or run: make web-serve"

web-serve:
	cd $(WEB_DIR) && python3 -m http.server $(WEB_PORT)

clean:
	rm -f $(APP) $(TEST_BIN) src/*.o test/*.o
