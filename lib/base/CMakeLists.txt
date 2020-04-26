file(GLOB_RECURSE COMMON_SOURCE *.c *.cpp *.h *.hpp)
add_library(vks STATIC ${COMMON_SOURCE})
target_link_libraries(vks gli)
