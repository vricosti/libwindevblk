cmake_minimum_required(VERSION 3.1)

#Configuration du projet
project(libwindevblk)

#set(EXECUTABLE_OUTPUT_PATH bin${CMAKE_BUILD_TYPE})

# sources
file(GLOB_RECURSE lib_src *.def *.c *.h ../include/*.h)


add_library(libwindevblk SHARED
  ${lib_src}
)

# Add include path
target_include_directories(libwindevblk PUBLIC
    ../include
	${CMAKE_SOURCE_DIR}/posix4msvc/include
	"C:/Program Files (x86)/Windows Kits/10/Include/10.0.10586.0/km"
)

target_link_libraries (libwindevblk setupapi.lib)

