#ifdef DATA_C_INC
#error "data.c included more than once"
#endif
#define DATA_C_INC

GameData g_GameData;

int data_read_file(GameData *data, FILE *file)
{
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t key_start_idx;
        size_t key_end_idx;
        size_t val_start_idx;
        size_t val_end_idx;

        for (key_start_idx = 0; key_start_idx < sizeof(line); ++key_start_idx) {
            char ch = line[key_start_idx];
            if (ch == 0 || (ch != ' ' && ch != '\t')) {
                break;
            }
        }

        for (key_end_idx = key_start_idx; key_end_idx < sizeof(line); ++key_end_idx) {
            char ch = line[key_end_idx];
            if (ch == 0 || ch == ' ' || ch == '\t' || ch == '=') {
                break;
            }
        }

        for (val_start_idx = key_end_idx; val_start_idx < sizeof(line); ++val_start_idx) {
            char ch = line[val_start_idx];
            if (ch == 0 || (ch != ' ' && ch != '\t' && ch != '=')) {
                break;
            }
        }

        for (val_end_idx = val_start_idx; val_end_idx < sizeof(line); ++val_end_idx) {
            char ch = line[val_end_idx];
            if (ch == 0 || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                break;
            }
        }

        line[key_end_idx] = 0;
        const char *key = &line[key_start_idx];
        line[val_end_idx] = 0;
        const char *val = &line[val_start_idx];

        if (strcmp(key, "root") == 0) {
            if (safe_strcpy(data->network.root, sizeof(data->network.root), val) == 0) {
                log_error(
                    "Parsing error, value for key 'root' is too long. (Length %zu, Maximum: %zu)",
                    val_end_idx - val_start_idx,
                    sizeof(data->network.root)
                );
                return 1;
            }
        } else if (strcmp(key, "server_public") == 0) {
            if (safe_strcpy(data->network.server_public, sizeof(data->network.server_public), val) == 0) {
                log_error(
                    "Parsing error, value for key 'server_public' is too long. (Length %zu, Maximum: %zu)",
                    val_end_idx - val_start_idx,
                    sizeof(data->network.server_public)
                );
                return 1;
            }
        } else if (strcmp(key, "prime") == 0) {
            if (safe_strcpy(data->network.prime, sizeof(data->network.prime), val) == 0) {
                log_error(
                    "Parsing error, value for key 'prime' is too long. (Length %zu, Maximum: %zu)",
                    val_end_idx - val_start_idx,
                    sizeof(data->network.prime)
                );
                return 1;
            }
        } else if (strcmp(key, "build") == 0) {
            data->version.build = atoi(val);
        } else if (strcmp(key, "file_id") == 0) {
            data->version.file_id = atoi(val);
        } else {
            log_warn("Unsupported key '%s'", key);
            continue;
        }
    }
    return 0;
}

int data_init(const char *dir)
{
    int err;
    char buffer[COMMON_PATH_MAX];

    FILE *file = NULL;
    if (dir != NULL) {
        err = snprintf(buffer, sizeof(buffer), "%s/gw_%d.pub.txt", dir, options.game_version);
        if (err < 0) {
            log_error("Invalid string format");
            abort();
        } else if (sizeof(buffer) <= err) {
            log_error("Buffer not large enough to hold the game data path. Have %zu bytes, %d needed", sizeof(buffer), err);
            return 1;
        }

        if ((file = fopen(dir, "r")) == NULL) {
            log_error("Couldn't open file '%s'", buffer);
            return 1;
        }
    } else {
        char exe_dir[COMMON_PATH_MAX];
        size_t exe_dir_len;
        if ((err = get_executable_dir(exe_dir, sizeof(exe_dir), &exe_dir_len)) != 0) {
            return err;
        }
        for (;;) {
            err = snprintf(buffer, sizeof(buffer), "%s%cdata%cgw_%d.pub.txt", exe_dir, PATH_SEP, PATH_SEP, options.game_version);
            if (err < 0) {
                log_error("Invalid string format");
                abort();
            } else if (sizeof(buffer) <= err) {
                log_error("Buffer not large enough to hold the game data path. Have %zu bytes, %d needed", sizeof(buffer), err);
                return 1;
            }

            file = fopen(buffer, "r");
            if (file != NULL)
                break;

            char *pos = strrchr(exe_dir, PATH_SEP);
            if (pos == NULL) {
                break;
            }
            *pos = 0;
        }

        if (file == NULL) {
            log_error("Couldn't find the version file");
        }
    }

    return data_read_file(&g_GameData, file);
}
