set(
  WEBGATE_SOURCES

  base64.c
  login.c
  login.h
  xml.c
)

set_source_files_properties(
  ${WEBGATE_SOURCES}
  PROPERTIES HEADER_FILE_ONLY TRUE
)

find_package(CURL)
message("Found: ${CURL_FOUND}")
message("version: ${CURL_VERSION}")
message("dirs: ${CURL_INCLUDE_DIRS}")

add_library(
  webgate

  build_webgate.c
  ${WEBGATE_SOURCES}
)

target_link_libraries(webgate PRIVATE common mbedcrypto CURL::libcurl)
target_include_directories(
  webgate

  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if (MSVC)
  # We disable the following warnings:
  # - C4115: named type definition in parentheses
  # - C4201: nonstandard extension used : nameless struct/union
  # - C4204: nonstandard extension used: non-constant aggregate initializer
  # - C4267: conversion from 'size_t' to 'int', possible loss of data
  target_compile_options(webgate PRIVATE /W4 /WX /wd4115 /wd4201 /wd4204 /wd4267)
else()
  target_compile_options(webgate PRIVATE -Wall -Werror -Wno-error=unused-function)
endif()
