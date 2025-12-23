#ifdef DATA_H_INC
#error "data.c included more than once"
#endif
#define DATA_H_INC

typedef struct GameData {
    struct
    {
        uint32_t build;
        uint32_t file_id;
    } version;
    struct
    {
        char root[64];
        char server_public[180];
        char prime[180];
    } network;
} GameData;

extern GameData g_GameData;

int data_init(const char *dir);
