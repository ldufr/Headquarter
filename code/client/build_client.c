#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _CRT_NONSTDC_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#ifndef __STDC__
# define __STDC__ 1
#endif

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# pragma comment(lib, "Ws2_32.lib")
# pragma comment(lib, "Winmm.lib")
# include <common/win32/win32.h>

# include <timeapi.h>
# include <Shellapi.h> // CommandLineToArgvA
# include <Winsock2.h>
# include <Ws2tcpip.h>
#else // _WIN32
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <fcntl.h>
#endif

#include <mbedtls/sha1.h>
#include <mbedtls/bignum.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include <common/macro.h>
#include <common/basics.h>
#include <common/time.h>
#include <common/assert.h>
#include <common/paths.h>
#include <common/socket.h>
#include <common/thread.h>
#include <common/process.h>
#include <common/noreturn.h>

#include <common/log.h>
#include <common/list.h>
#include <common/uuid.h>
#include <common/hash.h>
#include <common/timer.h>
#include <common/endian.h>
#include <common/rbtree.h>

#include <portal/login.h>
#include <webgate/login.h>

#include <client/Headquarter.h>

#ifdef _WIN32
#pragma warning(disable: 4100 4189)
#pragma warning(disable: 4214) // nonstandard extension used: bit field types other than int
#endif

#include "arc4.h"
#include "command.h"
#include "error.h"
#include "kstr.h"

#include "opcodes.h"
#include "packets.h"
#include "network.h"
#include "filesrv.h"

// @Cleanup: Temporary
typedef struct GwClient GwClient;
typedef struct World World;

#include "bag.h"
#include "chat.h"
#include "data.h"
#include "item.h"
#include "agent.h"
#include "guild.h"
#include "party.h"
#include "quest.h"
#include "skill.h"
#include "trade.h"
#include "title.h"
#include "dialog.h"
#include "effect.h"
#include "friend.h"
#include "player.h"
#include "instance.h"
#include "merchant.h"
#include "character.h"
#include "inventory.h"

#include "world.h"
#include "event.h"
#include "client.h"

#include "auth.h"
#include "game.h"
#include "plugins.h"

#include "api.c"
#include "auth.c"
#include "game.c"
#include "plugins.c"

#include "chat.c"
#include "data.c"
#include "hero.c"
#include "item.c"
#include "agent.c"
#include "guild.c"
#include "party.c"
#include "quest.c"
#include "skill.c"
#include "trade.c"
#include "title.c"
#include "dialog.c"
#include "effect.c"
#include "friend.c"
#include "player.c"
#include "instance.c"
#include "merchant.c"

#include "world.c"
#include "event.c"
#include "client.c"

#include "packets.c"
#include "network.c"
#include "filesrv.c"

#include "arc4.c"
#include "kstr.c"
#include "vars.c"
#include "error.c"
#include "command.c"

#include "main.c"
