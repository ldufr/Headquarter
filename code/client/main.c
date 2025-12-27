#ifdef CORE_MAIN_C
#error "main.c included more than once"
#endif
#define CORE_MAIN_C

bool quit;
uint32_t fps;
GwClient *client;

void sighandler(int signum)
{
    (void)signum;
    quit = true;
}

void main_loop(void)
{
    uint32_t frame_count = 0;
    struct timespec t0, t1;
    struct timespec last_frame_time;

    while (!quit) {
        frame_count += 1;
        get_wall_clock_time(&t0);

        long frame_delta_nsec = time_diff_nsec(&t0, &last_frame_time);
        msec_t frame_delta = frame_delta_nsec / 1000000;

        //
        // Update the client asynchronous operations such as:
        // - Connecting to an GuildWars account.
        // - Connecting to a game server or transferring game server.
        //
        if (client->state == AwaitAccountConnect) {
            AccountLogin(client);
        }

        // @Enhancement:
        // We gotta think a bit more about theses condition
        if (client->state == AwaitPlayCharacter) {
            assert(NetConn_IsShutdown(&client->game_srv));

            DECLARE_KSTR(charname_kstr, 20);
            if (!kstr_read_ascii(&charname_kstr, options.charname, 20)) {
                LogError("Failed to read '%s' in a kstr", options.charname);
                return;
            }

            PlayCharacter(client, &charname_kstr, options.online_status);
        }

        if (client->state == AwaitGameServerDisconnect &&
            NetConn_IsShutdown(&client->game_srv))
        {
            client->ingame = false;
            client->loading = true;
            client->state = AwaitGameServerTransfer;
        }

        if (client->state == AwaitGameServerTransfer &&
            NetConn_IsShutdown(&client->game_srv)) {

            TransferGameServer(client);
        }

        //
        // Update the network connection.
        // It will send the data out and check if new data was received.
        //
        NetConn_Update(&client->auth_srv);
        NetConn_Update(&client->game_srv);

        //
        // Update worlds
        // @TODO: Rename to "simulate world"?
        //
        client_frame_update(client, frame_delta);

        //
        // trigger all timers event
        //
        process_timers();

        {
            long frame_target_nsec = 1000000000 / fps;

            get_wall_clock_time(&t1);
            long diff_nsec = time_diff_nsec(&t1, &t0);
            long sleep_time_nsec = frame_target_nsec - diff_nsec;
            if (sleep_time_nsec > 0)
                time_sleep_ns(sleep_time_nsec);
        }

        last_frame_time = t0;
    }
}

int main(int argc, char **argv)
{
    parse_command_args(argc - 1, argv + 1);

    if (options.print_version) {
        printf("Headquarter %d.%d\n", HEADQUARTER_VERSION_MAJOR, HEADQUARTER_VERSION_MINOR);
        return 0;
    }

    if (argc < 2 || options.print_help) {
        print_help(true);
    }

    // @Enhancement: add arguments "-seed=23232"
    unsigned int seed = (unsigned int)time(NULL);
    srand(seed);

    signal(SIGINT, sighandler);
    log_init(options.log_file_name);
    time_init();
    init_timers();

    log_set_level(LOG_INFO);

    if (options.verbose)
        log_set_level(LOG_DEBUG);
    if (options.trace)
        log_set_level(LOG_TRACE);

#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    fps = 60;

    Network_Init();

    LogInfo("Initialization complete, running with client version %u\n", options.game_version);

    client = malloc(sizeof(*client));
    init_client(client);

    AuthSrv_RegisterCallbacks(&client->auth_srv);
    GameSrv_RegisterCallbacks(&client->game_srv);

    if (!init_auth_connection(client, options.auth_srv)) {
        LogInfo("Auth connection try failed.");
        exit(1);
    }

    {
        kstr_hdr_read_ascii(&client->email, options.email, ARRAY_SIZE(options.email));
        kstr_hdr_read_ascii(&client->charname, options.charname, ARRAY_SIZE(options.charname));

        DECLARE_KSTR(password, 100);
        kstr_read_ascii(&password, options.password, ARRAY_SIZE(options.password));

        if (options.newauth) {
            const char *secret;
            if (options.secret_2fa[0] == 0) {
                secret = NULL;
            } else {
                secret = options.secret_2fa;
            }

            struct portal_login_result result;
            int ret = portal_login(&result, options.email, options.password, secret);
            if (ret != 0) {
                fprintf(stderr, "Failed to connect to portal\n");
                return 1;
            }

            client->portal_token = result.token;
            client->portal_user_id = result.user_id;
        } else {
            struct kstr email;
            kstr_init_from_kstr_hdr(&email, &client->email);
            compute_pswd_hash(&email, &password, client->password);
        }

    #if defined(HEADQUARTER_CONSOLE)
        SetConsoleTitleA(options.charname);
    #endif
    }

    if (!plugin_load(options.script)) {
        LogError("Couldn't load the plugin '%s'", options.script);
        return 1;
    }
    LogInfo("Plugin loaded, %s\n", options.script);

    main_loop();

    while (!list_empty(&plugins)) {
        Plugin *it = plugin_first();
        plugin_unload(it);
    }

    Network_Shutdown();
    printf("Quit cleanly !!\n");

    return 0;
}
