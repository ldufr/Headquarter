set(
  PORTAL_SOURCES

  login.c
  login.h
  prf.c
  prf.h
  socket.c
  socket.h
  ssl.c
  ssl.h
  stream.c
  stream.h
  sts.c
  sts.h
  totp.c
  totp.h
)

set_source_files_properties(
  ${PORTAL_SOURCES}
  PROPERTIES HEADER_FILE_ONLY TRUE
)

add_library(
  portal

  build_portal.c
  ${PORTAL_SOURCES}
)

target_link_libraries(portal PRIVATE common mbedcrypto)
target_include_directories(
  portal

  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if (MSVC)
  # We disable the following warnings:
  # - C4115: named type definition in parentheses
  # - C4201: nonstandard extension used : nameless struct/union
  # - C4204: nonstandard extension used: non-constant aggregate initializer
  # - C4267: conversion from 'size_t' to 'int', possible loss of data
  target_compile_options(portal PRIVATE /W4 /WX /wd4115 /wd4201 /wd4204 /wd4267)
else()
  target_compile_options(portal PRIVATE -Wall -Werror -Wno-error=unused-function)
endif()

