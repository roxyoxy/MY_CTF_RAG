cmake_minimum_required(VERSION 3.16)

project(MY_CTF_RAG_GUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release: the online-installed Qt ships no debug DLLs
# (Qt6Cored.dll & co), and an empty build type would map the imported
# targets to their Debug variants and fail to run.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(MY_CTF_RAG_GUI
    qt_main.cpp
    mainwindow.cpp
    mainwindow.h
    ../src/loader.cpp
    ../src/chunker.cpp
    ../src/tokenizer.cpp
    ../src/atom_scan.cpp
    ../src/indexer.cpp
    ../src/persist.cpp
    ../src/corpus_diff.cpp
)

target_include_directories(MY_CTF_RAG_GUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(MY_CTF_RAG_GUI PRIVATE Qt6::Widgets)

if(MSVC)
    # /utf-8 keeps the Chinese UI literals in mainwindow.cpp readable
    # to the compiler (the file is UTF-8 without BOM).
    target_compile_options(MY_CTF_RAG_GUI PRIVATE /W4 /permissive- /utf-8)
    target_compile_definitions(MY_CTF_RAG_GUI PRIVATE UNICODE _UNICODE NOMINMAX)
else()
    target_compile_options(MY_CTF_RAG_GUI PRIVATE -Wall -Wextra -Wpedantic)
endif()
