#ifdef CORE_CLIENT_C
#error "client.c included more than once"
#endif
#define CORE_CLIENT_C

void init_client(GwClient *client)
{
    memzero(client, sizeof(*client));

    thread_mutex_init(&client->mutex);

    kstr_hdr_init(&client->email, client->email_buffer, ARRAY_SIZE(client->email_buffer));
    kstr_hdr_init(&client->charname, client->charname_buffer, ARRAY_SIZE(client->charname_buffer));

    client->state = AwaitAuthServer;
    client->ingame = false;
    client->loading = true;
    client->connected = true;

    client->current_character_idx = SIZE_MAX;

    array_init(&client->characters);

    array_init(&client->player_hero.maps_unlocked);

    init_chat(&client->chat);

    client->next_transaction_id = 1;

    init_event_manager(&client->event_mgr);

    init_connection(&client->auth_srv, client);
    init_connection(&client->game_srv, client);
}

World* get_world(GwClient *client)
{
    assert(client != NULL);
    if (client->world.hash == 0) {
        return NULL;
    } else {
        return &client->world;
    }
}

World* get_world_or_abort(GwClient *client)
{
    assert(client != NULL);
    if (client->world.hash == 0) {
        abort();
    } else {
        return &client->world;
    }
}

uint32_t issue_next_transaction(GwClient *client, AsyncType type)
{
    AsyncRequest *request = array_push(&client->requests, 1);
    request->trans_id = ++client->next_transaction_id;
    request->type = type;
    return request->trans_id;
}

void compute_pswd_hash(struct kstr *email, struct kstr *pswd, char digest[20])
{
    uint16_t buffer[256];
    size_t i = 0;
    for (size_t j = 0; j < pswd->length; j++, i++)
        buffer[i] = htole16(pswd->buffer[j] & 0xffff);
    for (size_t j = 0; j < email->length; j++, i++)
        buffer[i] = htole16(email->buffer[j] & 0xffff);
    Sha1(buffer, i * 2, digest);
}

struct area_info {
    uint32_t campaign;
    uint32_t continent;
    Region region;
    RegionType region_type;
    uint32_t flags;
    uint32_t x;
    uint32_t y;
    uint32_t start_x;
    uint32_t start_y;
    uint32_t end_x;
    uint32_t end_y;
};

struct region_type_to_map_type {
    uint32_t map_type;
    RegionType region_type;
};

uint32_t find_map_type_from_map_id(uint32_t map_id)
{
    static const struct area_info area_info_table[] = {
        #include "data/area_info.data"
    };
    const size_t area_info_table_len = ARRAY_SIZE(area_info_table);

    static const struct region_type_to_map_type region_type_to_map_type_table[] = {
        #include "data/region_type_to_map_type.data"
    };
    const size_t region_type_to_map_type_table_len = ARRAY_SIZE(region_type_to_map_type_table);

    assert(map_id < area_info_table_len);
    const struct area_info *area_info = &area_info_table[map_id];

    for (size_t i = 0; i < region_type_to_map_type_table_len; ++i) {
        const struct region_type_to_map_type *entry = &region_type_to_map_type_table[i];
        if (entry->region_type == area_info->region_type)
            return entry->map_type;
    }

    LogError("Failed to find the map type from the map id %hu", map_id);
    abort();
}

Character* GetCharacter(GwClient *client, uint32_t char_id)
{
    if (client->characters.size <= char_id) {
        return NULL;
    } else {
        return &client->characters.data[char_id];
    }
}

void ContinueAccountLogin(GwClient *client, uint32_t error_code)
{
    assert(client->state == AwaitAccountConnection);

    if (error_code != 0) {
        LogError("AccountLogin failed");
        client->state = AwaitNothing;
        return;
    }

    uint32_t trans_id = issue_next_transaction(client, AsyncType_SendHardwareInfo);
    client->state = AwaitHardwareInfoReply;
    AuthSrv_AskServerResponse(&client->auth_srv, trans_id);
}

void ContinueSendHardwareInfo(GwClient *client, uint32_t error_code)
{
    LogDebug("Unused error code: %" PRIu32, error_code);

    assert(client->state == AwaitHardwareInfoReply);
    client->state = AwaitPlayCharacter;
    client->connected = true;
}

void PortalAccountConnect(GwClient *client, struct uuid *user_id, struct uuid *token, struct kstr *charname)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        uint32_t transaction_id;
        uint8_t  user_id[16];
        uint8_t  session_id[16];
        uint16_t charname[20];
        uint16_t type[32]; // "Portal", presumably "Steam"?
    } PortalAccountLogin;
#pragma pack(pop)

    // @Cleanup:
    // This check is a little bit weird, we would want to do something like
    // `if (client->screen == SCREEN_LOGIN_SCREEN)`, but when we are ingame it
    // we should be ingame rather than at the handshake.
    if (client->state != AwaitAccountConnect) {
        LogError("You must connect & send computer infos before calling PortalAccountConnect");
        return;
    }

    uint32_t trans_id = issue_next_transaction(client, AsyncType_AccountLogin);

    PortalAccountLogin packet = NewPacket(AUTH_CMSG_PORTAL_ACCOUNT_LOGIN);
    packet.transaction_id = trans_id;

    uuid_enc_le(packet.user_id, user_id);
    uuid_enc_le(packet.session_id, token);
    
    if (ARRAY_SIZE(packet.charname) < charname->length) {
        LogError("Charname is too long. Length: %zu, Max: %zu", charname->length, ARRAY_SIZE(packet.charname));
        return;
    }

    // @Cleanup: This seem to now be unsupported, it causes issues
    //
    // memcpy(packet.charname, charname->buffer, charname->length * 2);
    // packet.charname[charname->length] = 0;

    static char token_type[] = "Portal";
    STATIC_ASSERT(ARRAY_SIZE(token_type) <= ARRAY_SIZE(packet.type));
    for (size_t idx = 0; idx < ARRAY_SIZE(token_type); ++idx) {
        packet.type[idx * 2] = token_type[idx];
    }

    LogDebug("PortalAccountConnect: {trans_id: %lu}", trans_id);
    SendPacket(&client->auth_srv, sizeof(packet), &packet);
}

void AccountLogin(GwClient *client)
{
    assert(client->state == AwaitAccountConnect);
    struct kstr charname;
    kstr_init_from_kstr_hdr(&charname, &client->charname);
    PortalAccountConnect(client, &client->portal_user_id, &client->portal_token, &charname);
    client->state = AwaitAccountConnection;
    // client->state.connection_pending = true;
}

void AccountLogout(GwClient *client)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t unk0;
    } Logout;
#pragma pack(pop)

    Logout packet = NewPacket(AUTH_CMSG_DISCONNECT);
    packet.unk0 = 0;

    // NetConn_FlagRemove(client->auth_srv);
    // client->state.connected = false;

    SendPacket(&client->auth_srv, sizeof(packet), &packet);
}

void ContinuePlayCharacter(GwClient *client, uint32_t error_code)
{
    assert(client->state == AwaitChangeCharacter);

    if (error_code != 0) {
        LogInfo("ContinuePlayCharacter failed (Code=%03d)", error_code);
        return;
    }

    // @Cleanup:
    // We could teleport to the last map of the character, but for that
    // we would need to find the type from data. Can be done with `AreaInfo`.
    extern void begin_travel(GwClient *client);
    begin_travel(client);

    // Character *cc = client->current_character; // can use to find the map
    uint32_t trans_id = issue_next_transaction(client, AsyncType_PlayCharacter);
    LogDebug("AuthSrv_RequestInstance: {trans_id: %lu}", trans_id);

    uint32_t choosen_map_id;
    if (options.opt_map_id.set) {
        choosen_map_id = options.opt_map_id.map_id;
    } else {
        Character *ch = GetCharacter(client, client->current_character_idx);
        choosen_map_id = ch ? ch->settings.last_outpost : 0;
    }

    uint32_t map_type;
    if (options.opt_map_type.set) {
        map_type = options.opt_map_type.map_type;
    } else {
        map_type = find_map_type_from_map_id(choosen_map_id);
    }

    AuthSrv_RequestInstance(&client->auth_srv, trans_id,
        choosen_map_id, map_type, DistrictRegion_America, 0, DistrictLanguage_Default);
}

void ContinueChangeCharacter(GwClient *client, uint32_t error_code)
{
    assert(client->state == AwaitChangeCharacter && client->pending_character_idx != 0);

    if (error_code != 0) {
        LogInfo("ContinueChangeCharacter failed (Code=%03d)", error_code);
        return;
    }
    client->current_character_idx = client->pending_character_idx;
    client->pending_character_idx = 0;
    client->state = AwaitPlayCharacter;
    PlayCharacter(client, NULL, 0);
}

bool ChangeCharacter(GwClient* client, struct kstr* name)
{
    Character *cc = GetCharacter(client, client->current_character_idx);
    if (!(cc && name && kstr_hdr_compare_kstr(&cc->name, name) != 0))
        return false;
    for (size_t idx = 0; idx < client->characters.size; ++idx) {
        Character *ch = &client->characters.data[idx];
        if (kstr_hdr_compare_kstr(&ch->name, name) == 0) {
            client->state = AwaitChangeCharacter;
            client->pending_character_idx = idx;
            uint32_t trans_id = issue_next_transaction(client, AsyncType_ChangeCharacter);

            LogDebug("AuthSrv_ChangeCharacter {trans_id: %lu}", trans_id);
            AuthSrv_ChangeCharacter(&client->auth_srv, trans_id, name);
            return true;
        }
    }

    LogError("Failed to find character matching required name");
    return false;
}

void PlayCharacter(GwClient *client, struct kstr *name, PlayerStatus status)
{
    assert(client->state == AwaitPlayCharacter);

    if (client->player_status != status) {
        AuthSrv_SetPlayerStatus(&client->auth_srv, status);
        client->player_status = status;
    }
    if (ChangeCharacter(client, name))
        return;
    client->state = AwaitChangeCharacter;
    ContinuePlayCharacter(client, 0);
}

void GameSrv_Disconnect(GwClient *client)
{
    assert(client && client->game_srv.secured);

    client->state = AwaitGameServerDisconnect;

    Packet packet = NewPacket(GAME_CMSG_DISCONNECT);
    SendPacket(&client->game_srv, sizeof(packet), &packet);
    NetConn_Shutdown(&client->game_srv);
}

void GameSrv_LogoutToCharselect(GwClient *client)
{
    if (client->player_status != PlayerStatus_Offline) {
        AuthSrv_SetPlayerStatus(&client->auth_srv, PlayerStatus_Offline);
        client->player_status = PlayerStatus_Offline;
    }

    // @Cleanup:
    // We have to ensure that whenever we lose the connection or connect to a game server
    // we reset all the necessary states.
    GameSrv_Disconnect(client);
}

void GameSrv_SkipCinematic(GwClient *client)
{
    Packet packet = NewPacket(GAME_CMSG_SKIP_CINEMATIC);
    SendPacket(&client->auth_srv, sizeof(packet), &packet);
}

void GameSrv_ChangeGold(GwClient *client, int gold_character, int gold_storage)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t gold_character;
        int32_t gold_storage;
    } ChangeGold;
#pragma pack(pop)

    assert(client && client->game_srv.secured);

    ChangeGold packet = NewPacket(GAME_CMSG_ITEM_CHANGE_GOLD);
    packet.gold_character = gold_character;
    packet.gold_storage = gold_storage;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_HeartBeat(Connection *conn);
void AuthSrv_HeartBeat(Connection *conn, msec_t tick);

void client_frame_update(GwClient *client, msec_t diff)
{
    thread_mutex_lock(&client->mutex);
    Connection *auth = &client->auth_srv;
    Connection *game = &client->game_srv;
    msec_t frame_time = time_get_ms();

    if (auth && ((frame_time - auth->last_tick_time) >= 120*1000)) {
        AuthSrv_HeartBeat(auth, frame_time);
        auth->last_tick_time = frame_time;
    }

    if (client->ingame && ((frame_time - game->last_tick_time) >= 5*1000)) {
        GameSrv_HeartBeat(game);
        game->last_tick_time = frame_time;
    }

    if (client->world.hash)
        world_update(&client->world, diff);
    thread_mutex_unlock(&client->mutex);
}
