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
#include <time.h>

#ifdef _WIN32
# include <common/win32/win32.h>
# include <Winsock2.h>
# include <Ws2tcpip.h>
# pragma comment(lib, "Ws2_32.lib")
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/tcp.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# define SOCKET int
# define closesocket close
# define INVALID_SOCKET -1
# define SOCKET_ERROR -1
#endif

#include <mbedtls/aes.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>

#include <common/array.h>
#include <common/basics.h>
#include <common/endian.h>
#include <common/macro.h>
#include <common/process.h>
#include <common/time.h>
#include <common/uuid.h>

#ifdef _WIN32
#pragma warning(disable: 4100 4189)
#endif

#include "stream.h"
#include "socket.h"

#include "login.h"
#include "prf.h"
#include "ssl.h"
#include "sts.h"
#include "totp.h"

#include "totp.c"
#include "login.c"
#include "prf.c"
#include "socket.c"
#include "ssl.c"
#include "stream.c"
#include "sts.c"
