#ifdef CORE_PACKETS_H
#error "packets.h included more than once"
#endif
#define CORE_PACKETS_H

typedef enum Type {
    TYPE_MSG_HEADER     = 1,
    TYPE_AGENT_ID       = 2,
    TYPE_FLOAT          = 3,
    TYPE_VECT2          = 4,
    TYPE_VECT3          = 5,
    TYPE_BYTE           = 6,
    TYPE_WORD           = 7,
    TYPE_DWORD          = 8,
    TYPE_BLOB           = 9,
    TYPE_STRING_16      = 10,
    TYPE_ARRAY_8        = 11,
    TYPE_ARRAY_16       = 12,
    TYPE_ARRAY_32       = 13,
    TYPE_NESTED_STRUCT  = 14,
} Type;

_Static_assert(sizeof(Type) == 4, "sizeof(Type) == 4");

typedef struct MsgField {
    Type        type;
    uint32_t    param;
    uint32_t    size; // optional, but really usefull for stuff like struct
    char       *name;
} MsgField;

typedef struct MsgFormat {
    uint32_t    header;
    uint32_t    count;
    MsgField   *fields;
    uint32_t    unpack_size;
} MsgFormat;

extern MsgFormat AUTH_CLIENT_FORMATS[AUTH_CMSG_COUNT];
extern MsgFormat AUTH_SERVER_FORMATS[AUTH_SMSG_COUNT];
extern MsgFormat GAME_CLIENT_FORMATS[GAME_CMSG_COUNT];
extern MsgFormat GAME_SERVER_FORMATS[GAME_SMSG_COUNT];
