CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude

ifeq ($(OS),Windows_NT)
WEB_LDLIBS := -lws2_32
else
WEB_LDLIBS :=
endif

CORE_SRC := $(filter-out src/main.cpp src/web_main.cpp src/BankWebAccess.cpp src/gui_main.cpp src/MainWindow.cpp,$(wildcard src/*.cpp))
APP := apex
WEB_APP := apex_web
TEST_SRC := $(wildcard test/*.cpp) $(CORE_SRC) src/BankWebAccess.cpp
TEST_BIN := apex_tests

.PHONY: all web test clean run run-web

all: $(APP)

$(APP): $(CORE_SRC) src/main.cpp src/BankWebAccess.cpp
	$(CXX) $(CXXFLAGS) $(CORE_SRC) src/main.cpp src/BankWebAccess.cpp -o $(APP)

web: $(WEB_APP)

$(WEB_APP): $(CORE_SRC) src/web_main.cpp src/BankWebAccess.cpp
	$(CXX) $(CXXFLAGS) $(CORE_SRC) src/web_main.cpp src/BankWebAccess.cpp -o $(WEB_APP) $(WEB_LDLIBS)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -Itest $(TEST_SRC) -o $(TEST_BIN)

run: $(APP)
	./$(APP)

run-web: $(WEB_APP)
	./$(WEB_APP)

clean:
	rm -f $(APP) $(WEB_APP) $(TEST_BIN) src/*.o test/*.o
