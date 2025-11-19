cmake_minimum_required(VERSION 3.19)
project(juddperft LANGUAGES CXX)

# Find the threads package
find_package(Threads REQUIRED)

add_executable(juddperft
    diagnostics.h  engine.h  fen.h  hashtable.h  Juddperft.h  movegen.h  raiitimer.h  search.h  targetver.h  timemanage.h  winboard.h
    diagnostics.cpp  engine.cpp  fen.cpp  hashtable.cpp  Juddperft.cpp  movegen.cpp  search.cpp  timemanage.cpp  winboard.cpp
)

set_property(TARGET juddperft PROPERTY
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(juddperft PRIVATE Threads::Threads)
target_link_libraries(juddperft PRIVATE atomic)

include(GNUInstallDirs)

install(TARGETS juddperft
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
