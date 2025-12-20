set(
  COMMON_SOURCES
  
  align.h
  array.c
  array.h
  assert.h
  basics.h
  paths.h
  endian.h
  hash.h
  list.h
  log.c
  log.h
  macro.h
  noreturn.h
  process.h
  rbtree.c
  rbtree.h
  socket.h
  thread.h
  time.h
  timer.c
  timer.h
  uuid.h
)

if (WIN32)
  list(
    APPEND COMMON_SOURCES
    win32/paths.c
    win32/process.c
    win32/thread.c
    win32/time.c
    win32/win32.h
  )
else()
  list(
    APPEND COMMON_SOURCES
    linux/paths.c
    linux/thread.c
    linux/time.c
  )
endif()

set_source_files_properties(
  ${COMMON_SOURCES}
  PROPERTIES HEADER_FILE_ONLY TRUE
)

add_library(
  common

  build_common.c
  ${COMMON_SOURCES}
)

if (MSVC)
  # We disable the following warnings:
  # - C4054: 'type cast': from function pointer 'FARPROC' to data pointer 'void *'
  # - C4201: nonstandard extension used: nameless struct/union
  # - C4204: nonstandard extension used: non-constant aggregate initializer
  # - C4221: nonstandard extension used: '<var>': cannot be initialized using address of automatic variable
  # - C4267: conversion from 'size_t' to 'int', possible loss of data
  # - C4459: declaration of '<var>' hides global declaration
  target_compile_options(common PRIVATE /WX /W4 /wd4054 /wd4201 /wd4204 /wd4221 /wd4267 /wd4459)
else ()
  target_compile_options(common PRIVATE -Wall -Werror -Wno-error=unused-function)
endif()

if (UNIX)
  target_link_libraries(common PRIVATE pthread m dl)
endif()

target_include_directories(
  common

  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
