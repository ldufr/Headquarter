#ifdef COMMAND_H_INC
#error "command.h included more than once"
#endif
#define COMMAND_H_INC

typedef struct CommandOptions {
    bool        print_help;
    bool        print_version;
    bool        verbose;
    bool        trace;
    bool        newauth;

    const char *script;
    const char *auth_srv;
    const char *conf_file;

    char        email[64];
    char        password[100];
    char        charname[20];
    char        secret_2fa[64];
    char        log_file_name[256];

    uint32_t    game_version;
    char        file_game_version[256];

    bool        launch_service;
    int         service_option; // windows => {'r', 'i', 'u'} or linux => {'r', 's'}

    int         online_status;
    int32_t     seed;

    struct {
        bool set;
        int  map_id;
    } opt_map_id;
    struct {
        bool set;
        int  map_type;
    } opt_map_type;
} CommandOptions;

extern CommandOptions options;
extern int    g_Argc;
extern char **g_Argv;

void print_help(bool terminate);
void parse_command_args(int argc, char **argv);
