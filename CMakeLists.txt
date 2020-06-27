# Specify the version being used as well as the language
cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0015 NEW)

foreach(lang C CXX)
	set(CMAKE_${lang}_STANDARD_REQUIRED ON)
	set(CMAKE_${lang}_EXTENSIONS OFF)
endforeach(lang)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra) 


project(LDParse)
set(LDPARSE_DEPS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deps")
list(APPEND CMAKE_MODULE_PATH "${LDPARSE_DEPS_DIRECTORY}")

find_package(Boost 1.67 REQUIRED COMPONENTS context)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/deps/immer 
	${CMAKE_CURRENT_SOURCE_DIR}/deps/CxxBidirectionalCoroutines/include)


file(GLOB_RECURSE LDPARSE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/include/LDParse/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/LDParse/*.cpp)
add_library(LDParse SHARED ${LDPARSE_SOURCE})

add_subdirectory(face-unwrapper)
add_subdirectory(parsetest)
