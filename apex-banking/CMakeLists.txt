cmake_minimum_required(VERSION 3.16)
project(apex_banking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

set(CORE_SOURCES
    src/Account.cpp
    src/Bank.cpp
    src/BankWebAccess.cpp
    src/Checking.cpp
    src/Currency.cpp
    src/Money.cpp
    src/Savings.cpp
    src/Transaction.cpp
)

add_executable(apex
    ${CORE_SOURCES}
    src/main.cpp
)

add_executable(apex_web
    ${CORE_SOURCES}
    src/web_main.cpp
)

if (WIN32)
    target_link_libraries(apex_web PRIVATE ws2_32)
endif()
