cmake_minimum_required(VERSION 3.27)
project(tempest VERSION 0.0.0)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS program_options)

file(GLOB LIB_SOURCES src/core/*.hpp src/core/*.cpp)
add_library(tempest SHARED ${LIB_SOURCES})
file(GLOB CLI_SOURCES src/cli/*.cpp)
foreach(file ${CLI_SOURCES} )
	# Cut off the file extension and directory path
	get_filename_component(name ${file} NAME_WE)
	add_executable(${name} ${file})
	target_link_libraries(${name} tempest Boost::program_options)
endforeach(file ${CLI_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)
target_compile_options(tempest PRIVATE)