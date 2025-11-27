cmake_minimum_required(VERSION 3.19)
project(juddperft LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mavx2 -DNDEBUG")

# Find the threads package
find_package(Threads REQUIRED)

add_executable(juddperft
	diagnostics.h
	engine.h
	fen.h
	hashtable.h
	juddperft.h
	movegen.h
	raiitimer.h
	search.h
	targetver.h
	timemanage.h
	utils.h
	winboard.h
	diagnostics.cpp
	engine.cpp
	fen.cpp
	hashtable.cpp
	juddperft.cpp
	movegen.cpp
	search.cpp
	timemanage.cpp
	utils.cpp
	winboard.cpp
)

set_property(TARGET juddperft PROPERTY
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

target_link_libraries(juddperft PRIVATE Threads::Threads)

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
	target_link_libraries(juddperft PRIVATE atomic)
endif()

include(GNUInstallDirs)

install(TARGETS juddperft
	BUNDLE  DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
