cmake_minimum_required(VERSION 3.27)
set(CMAKE_CXX_STANDARD 20)
set(project "tempest")
set(version "0.0.1")
project(${project} VERSION ${version})
include(FetchContent)
add_compile_definitions(PROJECT_NAME="${project}" VERSION="${version}")

set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")

find_package(Boost REQUIRED COMPONENTS program_options)

file(GLOB LIB_SOURCES src/core/*.hpp src/core/*.cpp)
add_library(${project} SHARED ${LIB_SOURCES})
target_compile_options(${project} PRIVATE)
set_target_properties(${project} PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install(TARGETS ${project} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

file(GLOB CLI_SOURCES src/cli/*.cpp)
foreach(file ${CLI_SOURCES})
	get_filename_component(name ${file} NAME_WE)
	add_executable(${name} ${file})
	target_link_libraries(${name} ${project} Boost::program_options)

	if(${name} STREQUAL "main")
		set_target_properties(${name} PROPERTIES OUTPUT_NAME "${project}")
	else()
		set_target_properties(${name} PROPERTIES OUTPUT_NAME "${project}-${name}")
	endif()

	install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endforeach(file ${CLI_SOURCES})

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

file(GLOB HEADERS src/core/*.hpp)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${project})