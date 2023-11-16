cmake_minimum_required(VERSION 3.11)
project(bsml VERSION 0.0)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS program_options)

file(GLOB SOURCES src/*.cpp	src/**/*.cpp)
add_executable(bsml ${SOURCES})

target_link_libraries(bsml Boost::program_options)
if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)
target_compile_options(bsml PRIVATE)