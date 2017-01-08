# Specify the version being used as well as the language
cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0015 NEW)

foreach(lang C CXX)
	set(CMAKE_${lang}_STANDARD_REQUIRED ON)
	set(CMAKE_${lang}_STANDARD 11)
	set(CMAKE_${lang}_EXTENSIONS OFF)
endforeach(lang)
add_definitions(-Wall) # This is a hack, but it should work consistently


project(LDParse)
set(LDPARSE_DEPS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deps")
list(APPEND CMAKE_MODULE_PATH "${LDPARSE_DEPS_DIRECTORY}")

find_package(Boost 1.59 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(LIBIGL)

option(LIBIGL_USE_STATIC_LIBRARY "Use LibIGL as static library" OFF)
option(LIBIGL_WITH_EMBREE "Use Embree" ON)

add_subdirectory("${LIBIGL_INCLUDE_DIR}/../shared/cmake" "libigl")
include_directories(${LIBIGL_INCLUDE_DIRS})
add_definitions(${LIBIGL_DEFINITIONS})


file(GLOB_RECURSE LDPARSE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/LDParse/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/LDParse/*.cpp)
add_library(LDParse STATIC ${LDPARSE_SOURCE})
#add_executable(parsetest ParseTest/main.cpp)
target_link_libraries(LDParse ${LIBIGL_LIBRARIES} ${LIBIGL_EXTRA_LIBRARIES})
#target_link_libraries(parsetest LDParse)