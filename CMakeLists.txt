cmake_minimum_required(VERSION 3.16)
project(cpp_api)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)

add_executable(cpp_api
    src/main.cpp
)

target_link_libraries(cpp_api
    PRIVATE
    ${Boost_LIBRARIES}
    mysqlclient
    Threads::Threads
    nlohmann_json::nlohmann_json
)
