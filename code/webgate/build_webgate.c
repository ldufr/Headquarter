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

#include <curl/curl.h>

#include <mbedtls/sha1.h>

#include <common/macro.h>
#include <common/array.h>
#include <common/time.h>
#include <common/uuid.h>

#include "login.h"
#include "totp.h"

#include "xml.c"
#include "base64.c"
#include "login.c"
#include "totp.c"
