cmake_minimum_required(VERSION 3.1)

project(windevblk_test)

file(GLOB_RECURSE SRCS *.def *.c *.h)

add_executable(windevblk_test ${SRCS})

target_include_directories(windevblk_test PUBLIC ../include)
set_target_properties(windevblk_test PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'")
target_link_libraries(windevblk_test libwindevblk)


