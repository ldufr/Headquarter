set(
  CLIENT_SOURCES
  
  agent.c
  agent.h
  api.c
  arc4.c
  arc4.h
  auth.c
  auth.h
  bag.h
  character.h
  chat.c
  chat.h
  client.c
  client.h
  command.c
  command.h
  data.c
  data.h
  dialog.c
  dialog.h
  effect.c
  effect.h
  error.c
  error.h
  event.c
  event.h
  friend.c
  friend.h
  filesrv.h
  filesrv.c
  game.c
  game.h
  guild.c
  guild.h
  hero.c
  instance.c
  instance.h
  inventory.h
  item.c
  item.h
  kstr.c
  kstr.h
  main.c
  merchant.c
  merchant.h
  network.c
  network.h
  opcodes.h
  packets.c
  packets.h
  party.c
  party.h
  player.c
  player.h
  plugins.c
  plugins.h
  portal_dll.c
  portal_dll.h
  quest.c
  quest.h
  skill.c
  skill.h
  title.c
  title.h
  trade.c
  trade.h
  world.c
  world.h
)

set_source_files_properties(
  ${CLIENT_SOURCES}
  PROPERTIES HEADER_FILE_ONLY TRUE
)

add_executable(
  client

  build_client.c
  ${CLIENT_SOURCES}
)

target_link_libraries(client PRIVATE common portal mbedcrypto)
target_compile_definitions(client PRIVATE HEADQUARTER_BUILD_EXPORTS)

target_include_directories(
  client

  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# This is necessary to allow other target to link against "client".
set_property(TARGET client PROPERTY ENABLE_EXPORTS 1)

if (MSVC)
  # We disable the following warnings:
  # - C4201: nonstandard extension used: nameless struct/union
  # - C4204: nonstandard extension used: non-constant aggregate initializer
  # - C4221: nonstandard extension used: '<var>': cannot be initialized using address of automatic variable
  # - C4267: conversion from 'size_t' to 'int', possible loss of data
  # - C4459: declaration of '<var>' hides global declaration
  target_compile_options(client PRIVATE /WX /W4 /wd4201 /wd4204 /wd4221 /wd4267 /wd4459)
endif()

