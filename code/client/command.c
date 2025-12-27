#ifdef COMMAND_C_INC
#error "command.c included more than once"
#endif
#define COMMAND_C_INC

CommandOptions options;
int    g_Argc;
char **g_Argv;

void print_help(bool terminate)
{
    fprintf(terminate ? stderr : stdout,
            "Usage: [options] <script>\n"

            "    --version                  Print version and exist\n"
            "    -h, --help                 Print this help\n"
            "    -v, --verbose              Enable debug logs\n"
            "    -vv, --trace               Enable trace logs\n"
            "\n"
            "    --log-file <path>          Specify the path of the log file\n"
            "    --log-dir  <dir path>      Specify the path of the logs directory\n"
            "    --data-dir <dir path>      Specify the path of the data directory\n"
            "\n"
            "    --email     <string>       Sets the client's email\n"
            "    --password  <string>       Enable auto-loging. use with -email and -charname\n"
            "    --character <string>       Specify which character to play on\n"
            "    --authsrv                  Specify authserver IP to connect to\n"
            "    --2fa-secret <string>      Specify the OTP secret to automatically fill the 2fa value\n"
            "    --file-game-version <path> Provide a path to a file with the game version\n"
            "\n"
            "    --status    <number>       Sets the login online status (0 = offline, 1 = online, 2 = busy, 3 = away)\n"
            "    --mapid                    Specify the map id you want to start in\n"
            "    --maptype                  Specify the map type you want to start in\n"
            "\n"
        );

    if (terminate) exit(0);
}

void check_for_more_arguments(int argc, char **argv, int i, int nargs)
{
    if (argc <= i + nargs) {
        log_error("Not enough arguments after '%s'", argv[i]);
        print_help(true);
    }
}

void parse_command_args(int argc, char **argv)
{
    // @Remark: Currently, if the format is not valid, for instance -email with no
    // arguments following, we will print the help and exit. Maybe we just want
    // to returns with an error flag set.

    options.online_status = 1;

    const char *ptr;
    if ((ptr = getenv("HEADQUARTER_PASSWORD")) != NULL) {
        safe_strcpy(options.password, ARRAY_SIZE(options.password), ptr);
    }

    if ((ptr = getenv("HEADQUARTER_2FA_SECRET")) != NULL) {
        safe_strcpy(options.secret_2fa, ARRAY_SIZE(options.secret_2fa), ptr);
    }

    for (int i = 0; i < argc; i++) {
        const char *arg = argv[i];

        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
            options.print_help = true;
        } else if (!strcmp(arg, "--version")) {
            options.print_version = true;
        } else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
            options.verbose = true;
        } else if (!strcmp(arg, "-vv") || !strcmp(arg, "--trace")) {
            options.trace = true;
        } else if (!strcmp(arg, "-authsrv") || !strcmp(arg, "--authsrv")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.auth_srv = argv[++i];
        } else if (!strcmp(arg, "-status") || !strcmp(arg, "--status")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.online_status = atoi(argv[++i]);
            if (options.online_status < 0 || 3 < options.online_status) {
                log_error("Invalid --status %d, must be between 0 and 3", options.online_status);
                print_help(true);
            }
        } else if (!strcmp(arg, "-email") || !strcmp(arg, "--email")) {
            check_for_more_arguments(argc, argv, i, 1);

            // @Remark: We need the email to be in lower cases, because
            // it is user to compute the static hash of the password.
            safe_strcpy(options.email, ARRAY_SIZE(options.email), argv[++i]);
            strlwc(options.email, ARRAY_SIZE(options.email));
        } else if (!strcmp(arg, "-password") || !strcmp(arg, "--password")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.password, ARRAY_SIZE(options.password), argv[++i]);
        } else if (!strcmp(arg, "-2fa-secret") || !strcmp(arg, "--2fa-secret")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.secret_2fa, ARRAY_SIZE(options.secret_2fa), argv[++i]);
        } else if (!strcmp(arg, "-character") || !strcmp(arg, "--character")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.charname, ARRAY_SIZE(options.charname), argv[++i]);
        } else if (!strcmp(arg, "-mapid") || !strcmp(arg, "--mapid")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.opt_map_id.set = true;
            options.opt_map_id.map_id = atoi(argv[++i]);
        } else if (!strcmp(arg, "-maptype") || !strcmp(arg, "--maptype")) {
            options.opt_map_type.set = true;
            options.opt_map_type.map_type = atoi(argv[++i]);
        } else if (!strcmp(arg, "--log-file")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.log_file, ARRAY_SIZE(options.log_file), argv[++i]);
        } else if (!strcmp(arg, "--log-dir")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.log_dir, ARRAY_SIZE(options.log_dir), argv[++i]);
        } else if (!strcmp(arg, "-file-game-version") || !strcmp(arg, "--file-game-version")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.file_game_version, ARRAY_SIZE(options.file_game_version), argv[++i]);
        } else if (!strcmp(arg, "--data-dir")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.data_dir, ARRAY_SIZE(options.data_dir), argv[++i]);
        } else if (!strcmp(arg, "--")) {
            ++i;
            g_Argv = &argv[i];
            g_Argc = argc - i;
            break;
        } else if (arg[0] == '-') {
            log_error("Unknown flag '%s'", arg);
            print_help(true);
        } else {
            if (options.script) {
                log_error("You shouldn't specify more than one script to run, '%s' already specified", options.script);
                print_help(true);
            }
            options.script = arg;
        }
    }

    if (!options.email[0]) {
        log_error("You need to specify '--email'");
        print_help(true);
    }

    if (!options.password[0]) {
        log_error("You need to specify '--password'");
        print_help(true);
    }

    if (!options.charname[0]) {
        log_error("You need to specify '--charname'");
        print_help(true);
    }

    if (!options.script[0]) {
        log_error("You need to specify the script");
        print_help(true);
    }

    if (options.log_dir[0]) {
        if (options.log_file[0]) {
            log_warn("Ignoring '--log-dir' (value: '%s'), because '--log-file' is also specified", options.log_dir);
        } else {
            size_t dir_len = strlen(options.log_dir);

            // Strip the last path seperator, we will add it manually.
            if (options.log_dir[dir_len - 1] == '/' || options.log_dir[dir_len - 1] == '\\') {
                options.log_dir[dir_len - 1] = 0;
            }

            struct tm ts;
            if (!time_localtime(time(NULL), &ts)) {
                abort();
            }

            int res = snprintf(
                options.log_file,
                sizeof(options.log_file),
                "%s/%04d-%02d-%02d_%02d-%02d-%02d_%d.txt",
                options.log_dir,
                ts.tm_year + 1900,
                ts.tm_mon + 1,
                ts.tm_mday,
                ts.tm_hour,
                ts.tm_min,
                ts.tm_sec,
                getpid());

            if (res < 0) {
                log_error("Couldn't create the path of the log file");
                abort();
            }
        }
    }

    if (!options.file_game_version[0]) {
        options.game_version = GUILD_WARS_VERSION;
    } else {
        FILE *file;
        if ((file = fopen(options.file_game_version, "rb")) == NULL) {
            log_error("Failed to open '%s', err: %d", options.file_game_version, errno);
            abort();
        }

        char buffer[16];
        size_t size;
        if (sizeof(buffer) < (size = fread(buffer, 1, sizeof(buffer), file))) {
            log_error("Couldn't read '%s', err: %d", options.file_game_version, errno);
            abort();
        }

        size_t length = size < sizeof(buffer) ? size : sizeof(buffer) - 1;
        buffer[length] = 0;

        int game_version = atoi(buffer);
        if (game_version <= 0) {
            log_error("Invalid game version '%s'", buffer);
            abort();
        }

        options.game_version = (uint32_t)game_version;
    }
}
