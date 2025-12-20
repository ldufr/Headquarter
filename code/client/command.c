#ifdef COMMAND_C_INC
#error "command.c included more than once"
#endif
#define COMMAND_C_INC

CommandOptions options;
int    g_Argc;
char **g_Argv;

void print_help(bool terminate)
{
    printf( "Usage: [options] <script>\n"

            "    --version                  print version and exist\n"
            "    -c <config_file>           use config_file as configuration file\n"
            "    -l <log_file>              Specify the name of the log file to write to\n"
            "    -h, --help                 print this help\n"
            "\n"
            "    -account   <string>        Name of the account to use\n"
            "    -email     <string>        Sets the client's email\n"
            "    -password  <string>        Enable auto-loging. use with -email and -charname\n"
            "    -character <string>        Specify which character to play on\n"
            "    -status    <number>        Sets the login online status (0 = offline, 1 = online, 2 = busy, 3 = away)\n"
            "    -authsrv                   Specify authserver IP to connect to\n"
            "\n"
            "    -oldauth                   The client will use Portal connection\n"
            "\n"
            "    -seed                      Specify the seed for pseudo random generation\n"
            "    -v, --verbose              Enable debug logs\n"
            "    -vv, --trace               Enable trace logs\n"
            "    -file-game-version <path>  Provide a path to a file with the game version\n"
            "\n"
            "    -mapid                     Specify the map id you want to start in\n"
            "    -maptype                   Specify the map type you want to start in\n"
            "\n"
        );

    if (terminate) exit(0);
}

void check_for_more_arguments(int argc, char **argv, int i, int nargs)
{
    if (argc <= i + nargs) {
        printf("Not enough arguments after '%s'\n", argv[i]);
        print_help(true);
    }
}

void parse_command_args(int argc, char **argv)
{
    // @Remark: Currently, if the format is not valid, for instance -email with no
    // arguments following, we will print the help and exit. Maybe we just want
    // to returns with an error flag set.

    options.newauth = true;
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
        } else if (!strcmp(arg, "-authsrv")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.auth_srv = argv[++i];
        } else if (!strcmp(arg, "-status")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.online_status = atoi(argv[++i]);
            if (options.online_status < 0 || options.online_status > 3) {
                printf("Invalid -status\n");
                print_help(true);
            }
        } else if (!strcmp(arg, "-account")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.account, ARRAY_SIZE(options.account), argv[++i]);
        } else if (!strcmp(arg, "-email")) {
            check_for_more_arguments(argc, argv, i, 1);

            // @Remark: We need the email to be in lower cases, because
            // it is user to compute the static hash of the password.
            safe_strcpy(options.email, ARRAY_SIZE(options.email), argv[++i]);
            strlwc(options.email, ARRAY_SIZE(options.email));
        } else if (!strcmp(arg, "-password")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.password, ARRAY_SIZE(options.password), argv[++i]);
        } else if (!strcmp(arg, "-2fa-secret")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.secret_2fa, ARRAY_SIZE(options.secret_2fa), argv[++i]);
        } else if (!strcmp(arg, "-character")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.charname, ARRAY_SIZE(options.charname), argv[++i]);
        } else if (!strcmp(arg, "-oldauth")) {
            options.newauth = false;
        } else if (!strcmp(arg, "-mapid")) {
            check_for_more_arguments(argc, argv, i, 1);
            options.opt_map_id.set = true;
            options.opt_map_id.map_id = atoi(argv[++i]);
        } else if (!strcmp(arg, "-maptype")) {
            options.opt_map_type.set = true;
            options.opt_map_type.map_type = atoi(argv[++i]);
        } else if (!strcmp(arg, "-l")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.log_file_name, ARRAY_SIZE(options.log_file_name), argv[++i]);
        } else if (!strcmp(arg, "-file-game-version")) {
            check_for_more_arguments(argc, argv, i, 1);
            safe_strcpy(options.file_game_version, ARRAY_SIZE(options.file_game_version), argv[++i]);
        } else if (!strcmp(arg, "--")) {
            ++i;
            g_Argv = &argv[i];
            g_Argc = argc - i;
            break;
        } else {
            if (options.script) {
                printf("You shouldn't specify more than one script to run, '%s' already specified\n", options.script);
                print_help(true);
            }
            options.script = arg;
        }
    }

    if (!options.email[0]) {
        printf("You need to specify '-email'\n");
        print_help(true);
    }

    if (!options.password[0]) {
        printf("You need to specify '-password'\n");
        print_help(true);
    }

    if (!options.charname[0]) {
        printf("You need to specify '-charname'\n");
        print_help(true);
    }

    if (!options.script[0]) {
        printf("You need to specify the script\n");
        print_help(true);
    }

    // Assign a default log file name if one wasn't provided
    if (!options.log_file_name[0]) {
        char timestamp[64];
        time_t t = time(NULL);
        struct tm ts;
        // @Robustness: Deal with the error?
        time_localtime(&t, &ts);
		if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", &ts) <= 0) {
			abort();
		}

		if (snprintf(options.log_file_name, sizeof(options.log_file_name), "%s_%d.txt", timestamp, getpid()) == -1) {
			abort();
		}
    }

    if (!options.file_game_version[0]) {
        options.game_version = GUILD_WARS_VERSION;
    } else {
        FILE *file;
        if ((file = fopen(options.file_game_version, "rb")) == NULL) {
            fprintf(stderr, "Failed to open '%s', err: %d\n", options.file_game_version, errno);
            abort();
        }

        char buffer[16];
        size_t size;
        if (sizeof(buffer) < (size = fread(buffer, 1, sizeof(buffer), file))) {
            fprintf(stderr, "Couldn't read '%s', err: %d\n", options.file_game_version, errno);
            abort();
        }

        size_t length = size < sizeof(buffer) ? size : sizeof(buffer) - 1;
        buffer[length] = 0;

        int game_version = atoi(buffer);
        if (game_version <= 0) {
            fprintf(stderr, "Invalid game version '%s'\n", buffer);
            abort();
        }

        options.game_version = (uint32_t)game_version;
    }
}
