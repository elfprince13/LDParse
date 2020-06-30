project(face-unwrapper)

add_executable(face-unwrapper face-unwrapper.cc)
target_link_libraries(face-unwrapper LDParse Boost::context Boost::filesystem fmt::fmt)
