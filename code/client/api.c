#ifdef CORE_API_C
#error "api.c included more than once"
#endif
#define CORE_API_C

Player* GetPlayerPtr(World *world)
{
    if (world->players.size <= world->player_id)
        return NULL;
    Player *player = world->players.data[world->player_id];
    if (player->player_id == 0) {
        return NULL;
    }
    return player;
}

Party* GetPlayerPartyPtr(World *world)
{
    Player *player;
    if ((player = GetPlayerPtr(world)) == NULL)
        return NULL;
    if (!player->party_id)
        return NULL;
    Party *party;
    if ((party = get_party_safe(world, player->party_id)) == NULL) {
        return NULL;
    }
    return party;
}

Guild* GetPlayerGuildPtr(World *world)
{
    Player *player;
    if ((player = GetPlayerPtr(world)) == NULL)
        return NULL;
    if (!player->guild_id)
        return NULL;
    Guild *guild;
    if ((guild = get_guild_safe(world, player->guild_id)) == NULL) {
        return NULL;
    }
    return guild;
}

HQAPI void LogError(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_verror(fmt, args);
    va_end(args);
}

HQAPI void LogDebug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vdebug(fmt, args);
    va_end(args);
}

HQAPI void LogInfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vinfo(fmt, args);
    va_end(args);
}

HQAPI void LogWarn(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vwarn(fmt, args);
    va_end(args);
}

HQAPI void LogTrace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vtrace(fmt, args);
    va_end(args);
}

HQAPI char** GetCommandLineArgs(int *argc)
{
    *argc = g_Argc;
    return g_Argv;
}

HQAPI void FreePluginAndExitThread(PluginObject *plugin, int retval)
{
    assert(client != NULL && plugin != NULL);
    thread_mutex_lock(&client->mutex);
    Plugin *it;
    plugin_foreach(it) {
        if (it->module == plugin->module) {
            plugin_unload(it);
            break;
        }
    }
    thread_mutex_unlock(&client->mutex);
    thread_exit(retval);
}

HQAPI size_t GetPlugins(ApiPlugin *buffer, size_t length)
{
    assert(client != NULL);
    Plugin *it;
    ApiPlugin *plugin;
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    plugin_foreach(it) {
        if (count >= length)
            goto leave;
        plugin = &buffer[count];
        plugin->handle = it->module;
        safe_strcpy(plugin->name, sizeof(plugin->name), it->path);
        count++;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI bool RegisterEvent(EventType event, CallbackEntry *entry)
{
    assert(client != NULL);
    if (event < 0 || EventType_Count <= event)
        return false;
    if (!entry || entry->registered)
        return false;
    thread_mutex_lock(&client->mutex);
    events_add_entry(&client->event_mgr, event, entry);
    thread_mutex_unlock(&client->mutex);
    return true;
}

HQAPI bool UnRegisterEvent(CallbackEntry *entry)
{
    assert(client != NULL);
    if (!entry)
        return false;
    if (!entry->registered)
        return true;
    if (!(entry->node.prev && entry->node.next))
        return false;
    thread_mutex_lock(&client->mutex);
    events_rem_entry(&client->event_mgr, entry);
    thread_mutex_unlock(&client->mutex);
    return true;
}

HQAPI void LogoutToCharselect()
{
}

HQAPI msec_t GetPing(void)
{
    assert(client != NULL);
    msec_t ping = 0;
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    ping = client->game_srv.latency;
leave:
    thread_mutex_unlock(&client->mutex);
    return ping;
}

HQAPI msec_t GetPvPTimer(void)
{
    assert(client != NULL);
    msec_t pvp_timer = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (!world->pvp_timer_start)
        goto leave;
    msec_t diff = world->world_time - world->pvp_timer_start;
    pvp_timer = (world->pvp_timer_duration - diff) % world->pvp_timer_duration;
leave:
    thread_mutex_unlock(&client->mutex);
    return pvp_timer;
}

HQAPI msec_t GetWorldTime(void)
{
    assert(client != NULL);
    msec_t world_time = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    world_time = world->world_time;
leave:
    thread_mutex_unlock(&client->mutex);
    return world_time;
}

HQAPI bool GetIsIngame(void)
{
    assert(client != NULL);
    return client->ingame && !client->loading;
}

HQAPI bool GetIsConnected(void)
{
    assert(client != NULL);
    return client->connected && !NetConn_IsShutdown(&client->auth_srv);// && !NetConn_IsShutdown(&client->game_srv);
}

HQAPI int GetMapId(void)
{
    assert(client != NULL);

    int map_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    map_id = world->map_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return map_id;
} 

HQAPI size_t GetMapsUnlocked(uint32_t *buffer, size_t length)
{
    assert(client != NULL);

    size_t written = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (buffer) {
        if (client->player_hero.maps_unlocked.size > length)
            goto leave;
        memcpy(buffer, client->player_hero.maps_unlocked.data, client->player_hero.maps_unlocked.size * sizeof(*client->player_hero.maps_unlocked.data));
        written = client->player_hero.maps_unlocked.size;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return written;
}

HQAPI bool IsMapUnlocked(uint32_t map_id)
{
    assert(client != NULL);
    bool ret = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    uint32_t real_index = map_id / 32;
    if (real_index >= array_size(&client->player_hero.maps_unlocked))
        goto leave;
    uint32_t flag = 1 << (map_id % 32);
    uint32_t val = array_at(&client->player_hero.maps_unlocked, real_index);
    ret = (val & flag) != 0;
leave:
    thread_mutex_unlock(&client->mutex);
    return ret;
}

HQAPI District GetDistrict(void)
{
    assert(client != NULL);

    District district = DISTRICT_CURRENT;
    switch (GetDistrictRegion()) {
    case DistrictRegion_International:
        return DISTRICT_INTERNATIONAL;
    case DistrictRegion_America:
        return DISTRICT_AMERICAN;
    case DistrictRegion_Korea:
        return DISTRICT_ASIA_KOREAN;
    case DistrictRegion_Europe:
        switch (GetDistrictLanguage()) {
        case DistrictLanguage_English:
            return DISTRICT_EUROPE_ENGLISH;
        case DistrictLanguage_French:
            return DISTRICT_EUROPE_FRENCH;
        case DistrictLanguage_German:
            return DISTRICT_EUROPE_GERMAN;
        case DistrictLanguage_Italian:
            return DISTRICT_EUROPE_ITALIAN;
        case DistrictLanguage_Spanish:
            return DISTRICT_EUROPE_SPANISH;
        case DistrictLanguage_Polish:
            return DISTRICT_EUROPE_POLISH;
        case DistrictLanguage_Russian:
            return DISTRICT_EUROPE_RUSSIAN;
        }
        break;
    case DistrictRegion_China:
        return DISTRICT_ASIA_CHINESE;
    case DistrictRegion_Japanese:
        return DISTRICT_ASIA_JAPANESE;
    }

    return district;
}

HQAPI DistrictLanguage GetDistrictLanguage(void)
{
    assert(client != NULL);
    DistrictLanguage district_language = DistrictLanguage_Default;
    thread_mutex_lock(&client->mutex);
    World* world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    district_language = world->language;
leave:
    thread_mutex_unlock(&client->mutex);
    return district_language;
}

HQAPI DistrictRegion GetDistrictRegion(void)
{
    assert(client != NULL);
    DistrictRegion district_region = DistrictRegion_America;
    thread_mutex_lock(&client->mutex);
    district_region = client->region;
    thread_mutex_unlock(&client->mutex);
    return district_region;
}

HQAPI int GetDistrictNumber(void)
{
    assert(client != NULL);
    int district_number = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    district_number = world->district;
leave:
    thread_mutex_unlock(&client->mutex);
    return district_number;
}

HQAPI void Travel(uint32_t map_id, District district, uint16_t district_number)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    // @Cleanup: Check if we want to use region & language internally
    // DistrictRegion region;
    // DistrictLanguage language;
    // extract_district(world, district, &region, &language);
    assert(map_id <= (uint32_t)UINT16_MAX);
    GameSrv_Travel(client, cast(uint16_t)map_id, district, district_number);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TravelHall(uint32_t guild_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Guild *guild = get_guild_safe(world, guild_id);
    if (!guild) goto leave;
    GameSrv_TravelGH(client, &guild->guild_uuid);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void LeaveHall(void)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    // @Enhancement: Is there a way to check if we are in a guild hall.
    GameSrv_LeaveGH(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void RedirectMap(uint32_t map_id, District district, uint32_t district_number)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->connected)
        goto leave;
    uint32_t map_type = find_map_type_from_map_id(map_id);

    DistrictRegion region;
    DistrictLanguage language;
    extract_district(client, district, &region, &language);

    uint32_t trans_id = issue_next_transaction(client, AsyncType_None);
    AuthSrv_RequestInstance(&client->auth_srv, trans_id, map_id,
        map_type, 0, 0, 0);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI bool GetInCinematic(void)
{
    assert(client != NULL);
    bool in_cinematic = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    in_cinematic = world->in_cinematic;
leave:
    thread_mutex_unlock(&client->mutex);
    return in_cinematic;
}

HQAPI void SkipCinematic(void)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL || !world->in_cinematic)
        goto leave;
    GameSrv_SkipCinematic(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI size_t GetCharacterName(char *buffer, size_t length)
{
    assert(client != NULL);

    size_t written = 0;
    thread_mutex_lock(&client->mutex);
    if (client->characters.size <= client->current_character_idx)
        goto leave;
    Character *cc = &client->characters.data[client->current_character_idx];
    if (!cc) goto leave;
    if (kstr_hdr_write_ascii(&cc->name, buffer, length)) {
        written = cc->name.length;
    } else {
        written = length;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return written;
}
HQAPI int GetAccountUuid(char* buffer, size_t length)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    int written = uuid_snprint(buffer, length, &client->uuid);
    thread_mutex_unlock(&client->mutex);
    return written;
}

HQAPI size_t GetPlayers(ApiPlayer *buffer, size_t length)
{
    assert(client != NULL);
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (buffer == NULL) {
        count = world->player_count;
        goto leave;
    }
    ArrayPlayer players = world->players;
    for (size_t i = 0; (i < players.size) && (count < length); i++) {
        if (!players.data[i]) continue;
        ApiPlayer *dest = &buffer[count++];
        api_make_player(dest, players.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI size_t GetPlayerName(uint32_t player_id, uint16_t *buffer, size_t length)
{
    assert(client != NULL);
    if (!length && buffer)
        return 0;
    size_t written = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    ArrayPlayer players = world->players;
    if (!array_inside(&players, player_id))
        goto leave;
    Player *player = array_at(&players, player_id);
    if (!player) goto leave;
    if (!buffer) {
        written = player->name.length;
        goto leave;
    }
    if (kstr_hdr_write(&player->name, buffer, length)) {
        written = player->name.length;
    } else {
        written = 0;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return written;
}

HQAPI bool GetPlayerIsPartyLeader(uint32_t player_id)
{
    assert(client != NULL);
    bool is_leader = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL) {
        goto leave;
    }
    ArrayPartyPlayer players = party->players;
    if (!players.size)
        goto leave;
    is_leader = (players.data[0].player_id == player_id);
leave:
    thread_mutex_unlock(&client->mutex);
    return is_leader;
}

HQAPI size_t GetPlayersOfParty(uint32_t party_id, uint32_t *buffer, size_t length)
{
    assert(client != NULL);
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party = get_party_safe(world, party_id);
    if (!party)
        goto leave;
    ArrayPartyPlayer players = party->players;
    if (!buffer) {
        count = players.size;
        goto leave;
    }
    for (size_t i = 0; (i < players.size) && (i < length); i++) {
        buffer[count++] = players.data[i].player_id;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI AgentId GetMyAgentId(void)
{
    thread_mutex_lock(&client->mutex);
    AgentId ret = 0;
    World *world;
    if ((world = get_world(client)) != NULL)
        ret = world->player_agent_id;
    thread_mutex_unlock(&client->mutex);
    return ret;
}

HQAPI uint32_t GetMyGuildId(void)
{
    assert(client != NULL);
    uint32_t guild_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Player *player;
    if ((player = GetPlayerPtr(world)) == NULL)
        goto leave;
    guild_id = player->guild_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return guild_id;
}

HQAPI uint32_t GetMyPlayerId(void)
{
    assert(client != NULL);
    uint32_t player_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Player *player;
    if ((player = GetPlayerPtr(world)) == NULL)
        goto leave;
    player_id = world->player_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return player_id;
}

HQAPI bool GetGuildInfo(ApiGuild *api_guild, uint32_t guild_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Guild* guild = get_guild_safe(world, guild_id);
    if (!guild) goto leave;
    api_make_guild(api_guild, guild);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI size_t GetGuildMembers(ApiGuildMember *buffer, size_t length)
{
    assert(client != NULL);
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Guild *guild;
    if ((guild = GetPlayerGuildPtr(world)) == NULL)
        goto leave;
    ArrayGuildMember members = guild->members;
    if (buffer == NULL) {
        count = members.size;
        goto leave;
    }
    for (size_t i = 0; (i < members.size) && (count < length); i++) {
        ApiGuildMember* dest = &buffer[count++];
        api_make_guild_member(dest, &members.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI uint32_t GetGuildOfAgent(uint32_t agent_id)
{
    assert(client != NULL);
    uint32_t guild_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    Player *player = get_player_safe(world, agent->player_id);
    if (!(player && player->guild_id))
        goto leave;
    Guild *guild;
    if ((guild = get_guild_safe(world, player->guild_id)) == NULL) {
        goto leave;
    }
    guild_id = guild->guild_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return guild_id;
}

HQAPI uint32_t GetGuildFaction(uint32_t guild_id, FactionType *allegiance)
{
    assert(client != NULL);
    int32_t points = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Guild *guild = get_guild_safe(world, guild_id);
    if (!guild) goto leave;
    points = guild->faction_pts;
    if (allegiance) {
        *allegiance = guild->allegiance;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return points;
}

HQAPI bool GetAgent(ApiAgent *api_agent, AgentId agent_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    api_make_agent(api_agent, agent);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI Vec2f GetAgentPos(AgentId agent_id)
{
    assert(client != NULL);

    Vec2f pos = {INFINITY, INFINITY};
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    pos = agent->position;
leave:
    thread_mutex_unlock(&client->mutex);
    return pos;
}

HQAPI bool GetAgentOfItem(ApiAgent *api_agent, uint32_t item_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item  *item = get_item_safe(world, item_id);
    if (!(item && item->agent_id))
        goto leave;
    Agent *agent = get_agent_safe(world, item->agent_id);
    if (!agent) goto leave;
    api_make_agent(api_agent, agent);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI bool GetAgentOfPlayer(ApiAgent *api_agent, uint32_t player_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (!player_id) {
        player_id = world->player_agent_id;
    }
    Player *player = get_player_safe(world, player_id);
    if (!player) goto leave;
    Agent *agent = get_agent_safe(world, player->agent_id);
    if (!agent) goto leave;
    api_make_agent(api_agent, agent);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI bool GetAgentIsMoving(AgentId agent_id)
{
    assert(client != NULL);
    bool moving = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    moving = agent->moving || agent->maybe_moving;
leave:
    thread_mutex_unlock(&client->mutex);
    return moving;
}

HQAPI AgentEffect GetAgentEffects(AgentId agent_id)
{
    assert(client != NULL);
    AgentEffect effect = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    effect = agent->effects;
leave:
    thread_mutex_unlock(&client->mutex);
    return effect;
}

HQAPI size_t GetAgents(ApiAgent *buffer, size_t length)
{
    assert(client != NULL);
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    ArrayAgent agents = world->agents;
    if (buffer == NULL) {
        count = agents.size;
        goto leave;
    }
    for (size_t i = 0; (i < agents.size) && (count < length); i++) {
        if (!agents.data[i]) continue;
        ApiAgent *dest = &buffer[count++];
        api_make_agent(dest, agents.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI uint32_t GetNpcIdOfAgent(AgentId agent_id)
{
    assert(client != NULL);
    uint32_t npc_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!(agent && agent->npc_id))
        goto leave;
    npc_id = agent->npc_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return npc_id;
}

HQAPI bool GetItem(ApiItem *api_item, uint32_t item_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (!array_inside(&world->items, item_id))
        goto leave;
    Item *item = array_at(&world->items, item_id);
    if (!item) goto leave;
    api_make_item(api_item, item);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI bool GetItemOfAgent(ApiItem *api_item, AgentId agent_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!(agent && agent->type == AgentType_Item))
        goto leave;
    Item *item = get_item_safe(world, agent->item_id);
    if (!item) goto leave;
    api_make_item(api_item, item);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI BagEnum GetItemLocation(uint32_t item_id, unsigned int *slot)
{
    assert(client != NULL);
    BagEnum bag = BagEnum_Invalid;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item *item = get_item_safe(world, item_id);
    if (!(item && item->bag))
        goto leave;
    bag = item->bag->model;
    if (slot) *slot = item->slot;
leave:
    thread_mutex_unlock(&client->mutex);
    return bag;
}

HQAPI size_t GetItemModStruct(uint32_t item_id, uint32_t* buffer, size_t length)
{
    assert(client != NULL);
    size_t written = 0;
    if (!length && buffer)
        goto leave;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item* item = get_item_safe(world, item_id);
    if (!item)
        goto leave;
    if (!buffer) {
        written = item->mod_struct.size;
        goto leave;
    }
    if (length < item->mod_struct.size)
        goto leave;
    for (written = 0; written < item->mod_struct.size; ++written) {
        buffer[written] = array_at(&item->mod_struct, written);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return written;
}

HQAPI size_t GetItemName(uint32_t item_id, uint16_t* buffer, size_t length)
{
    assert(client != NULL);
    if (!length && buffer)
        return 0;
    size_t written = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item* item = get_item_safe(world, item_id);
    if (!item)
        goto leave;
    if (!buffer) {
        written = item->name.length;
        goto leave;
    }
    if (kstr_hdr_write(&item->name, buffer, length)) {
        written = item->name.length;
    } else {
        written = 0;
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return written;
}

HQAPI size_t GetBagCapacity(BagEnum bag)
{
    assert(client != NULL);
    size_t capacity = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Bag *bag_ptr = world->inventory.bags[bag];
    if (!bag_ptr) goto leave;
    capacity = bag_ptr->items.size;
leave:
    thread_mutex_unlock(&client->mutex);
    return capacity;
}

HQAPI size_t GetBagItems(BagEnum bag, ApiItem *buffer, size_t length)
{
    assert(client != NULL);

    size_t count = 0;

    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Bag *bag_ptr = world->inventory.bags[bag];
    if (!bag_ptr) goto leave;
    ArrayItem items = bag_ptr->items;
    if (buffer == NULL) {
        count = items.size;
        goto leave;
    }
    for (size_t i = 0; (i < items.size) && (count < length); i++) {
        if (!items.data[i]) continue;
        ApiItem *item = &buffer[count++];
        api_make_item(item, items.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI bool RequestItemQuote(uint32_t item_id)
{
    assert(client != NULL);
    bool success = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    size_t merchant_item_size = array_size(&world->merchant_items);
    if (!merchant_item_size)
        goto leave;
    if (!world->merchant_agent_id)
        goto leave;
    Agent* agent = get_agent_safe(world, world->player_agent_id);
    if (!agent) goto leave;
    Vec2f player_pos = GetAgentPos(GetMyAgentId());
    agent = get_agent_safe(world, world->merchant_agent_id);
    if (!agent) goto leave;
    Vec2f agent_pos = GetAgentPos(GetMyAgentId());
    float dx = agent_pos.x - player_pos.x;
    float dy = agent_pos.y - player_pos.y;
    float dist = sqrtf((dx * dx) + (dy * dy));
    if (dist > 160.f)
        goto leave; // Too far from merchant
    Item** arr_item = NULL;
    Item* item = NULL;
    array_foreach(arr_item, &world->merchant_items) {
        if (*arr_item && (*arr_item)->item_id == item_id) {
            item = *arr_item;
            break;
        }
    }
    if (!item)
        goto leave;
    TransactionType type = TransactionType_TraderSell;
    QuoteInfo give;
    give.item_count = 0;
    give.gold = 0;
    QuoteInfo recv;
    recv.item_count = 0;
    recv.gold = 0;
    if (!item->bag) {
        // Buy quote
        type = TransactionType_TraderBuy;
        recv.item_count = 1;
        recv.item_ids[0] = item_id;
    }
    else {
        // Sell quote
        type = TransactionType_TraderSell;
        give.item_count = 1;
        give.item_ids[0] = item_id;
    }
    GameSrv_RequestQuote(client, type, &give, &recv, false);
    success = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return success;
}

HQAPI size_t GetMerchantItems(ApiItem *buffer, size_t length)
{
    assert(client != NULL);

    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    ArrayItem items = world->merchant_items;
    if (buffer == NULL) {
        count = items.size;
        goto leave;
    }
    for (size_t i = 0; (i < items.size) && (count < length); i++) {
        if (!items.data[i]) continue;
        ApiItem *item = &buffer[count++];
        api_make_item(item, items.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI bool GetQuest(ApiQuest *quest, uint32_t quest_id)
{
    assert(client != NULL);
    bool found = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Quest *quest_ptr = find_quest_by_id(&world->quests, quest_id);
    if (!quest_ptr)
        goto leave;
    api_make_quest(quest, quest_ptr);
    found = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return found;
}

HQAPI size_t GetQuests(ApiQuest *buffer, size_t length)
{
    assert(client != NULL);

    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    ArrayQuest quests = world->quests;
    if (buffer == NULL) {
        count = quests.size;
        goto leave;
    }
    for (size_t i = 0; (i < quests.size) && (count < length); i++) {
        ApiQuest *quest = &buffer[count++];
        api_make_quest(quest, &quests.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI size_t GetFriends(ApiFriend* buffer, size_t length)
{
    assert(client != NULL);

    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    FriendArray friends = client->friends;
    if (buffer == NULL) {
        count = friends.size;
        goto leave;
    }
    for (size_t i = 0; (i < friends.size) && (count < length); i++) {
        ApiFriend* friend = &buffer[count++];
        api_make_friend(friend, &friends.data[i]);
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI bool GetFriendByUuid(ApiFriend *friend, const uint8_t *uuid)
{
    assert(client != NULL);
    bool found = false;
    struct uuid u;
    uuid_dec_le(uuid, &u);

    thread_mutex_lock(&client->mutex);
    Friend* friend_ptr = get_friend(&u, NULL);
    if (!friend_ptr)
        goto leave;
    api_make_friend(friend, friend_ptr);
    found = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return found;
}

HQAPI bool GetFriend(ApiFriend* friend, const uint16_t* name)
{
    assert(client != NULL);
    bool found = false;
    thread_mutex_lock(&client->mutex);
    Friend* friend_ptr = get_friend(NULL, (uint16_t * )name);
    if (!friend_ptr)
        goto leave;
    api_make_friend(friend, friend_ptr);
    found = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return found;
}

HQAPI void AddFriend(const uint16_t* name)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    AuthSrv_AddFriend(&client->auth_srv, name);
    thread_mutex_unlock(&client->mutex);
}

HQAPI FactionPoint GetLuxonPoints(void)
{
    assert(client != NULL);

    FactionPoint point = {0};
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    point = client->player_hero.luxon;
leave:
    thread_mutex_unlock(&client->mutex);
    return point;
}

HQAPI FactionPoint GetKurzickPoints(void)
{
    assert(client != NULL);

    FactionPoint point = {0};
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    point = client->player_hero.kurzick;
leave:
    thread_mutex_unlock(&client->mutex);
    return point;
}

HQAPI FactionPoint GetImperialPoints(void)
{
    assert(client != NULL);

    FactionPoint point = {0};
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    point = client->player_hero.imperial;
leave:
    thread_mutex_unlock(&client->mutex);
    return point;
}

HQAPI FactionPoint GetBalthazarPoints(void)
{
    assert(client != NULL);

    FactionPoint point = {0};
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    point = client->player_hero.balthazar;
leave:
    thread_mutex_unlock(&client->mutex);
    return point;
}

HQAPI void DonateFaction(FactionType faction)
{
    assert(client != NULL);
    const uint32_t max_guild_faction = 214748364; // (2^31 / 10)

    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Guild *guild;
    if ((guild = GetPlayerGuildPtr(world)) == NULL) {
        goto leave;
    }
    if (max_guild_faction < guild->faction_pts + 5000)
        goto leave;
    GameSrv_DonateFaction(client, faction, 5000);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void MoveToPoint(Vec2f pos)
{
    MoveToCoord(pos.x, pos.y);
}

HQAPI void MoveToCoord(float x, float y)
{
    assert(client != NULL);

    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_MoveToCoord(client, x, y);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void RotateToAngle(float angle)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_RotateToAngle(client, angle);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void ReturnToOutpost(void)
{
    assert(client != NULL);

    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_ReturnToOutpost(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI Difficulty GetDifficulty(void)
{
    assert(client != NULL);
    Difficulty mode = Difficulty_Normal;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL)
        goto leave;
    mode = party->difficulty;
leave:
    thread_mutex_unlock(&client->mutex);
    return mode;
}

HQAPI void SetDifficulty(Difficulty mode)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_SetDifficulty(client, mode);
leave:
    thread_mutex_unlock(&client->mutex);
}


HQAPI void SendChat(Channel channel, const char *msg)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    DECLARE_KSTR(kmsg, 256);
    kstr_read_ascii(&kmsg, msg, strlen(msg));
    GameSrv_SendChat(client, channel, &kmsg);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void SendWhisper(const char *target, const char *msg)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    DECLARE_KSTR(kmsg, 120);
    DECLARE_KSTR(ktarget, 20);
    kstr_read_ascii(&kmsg, msg, strlen(msg));
    kstr_read_ascii(&ktarget, target, strlen(target));
    GameSrv_SendWhisper(client, &ktarget, &kmsg);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void SendDialog(uint32_t dialog_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_SendDialog(client, dialog_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

void MoveItem(uint32_t item_id, BagEnum bag, uint8_t slot)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Bag *bag_ptr = get_bag_safe(world, bag);
    if (!bag_ptr)
        goto leave;
    if ((size_t)slot >= bag_ptr->items.capacity)
        goto leave;
    Item *item = get_item_safe(world, item_id);
    if (!item)
        goto leave;
    GameSrv_MoveItem(client, item, bag_ptr, slot);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void UnequipItem(EquipedItemSlot equip_slot, BagEnum bag, uint8_t slot)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Bag *bag_ptr = get_bag_safe(world, bag);
    if (!bag_ptr)
        goto leave;
    if ((size_t)slot >= bag_ptr->items.capacity)
        goto leave;
    Bag *bag_inv = get_bag_safe(world, BagEnum_EquippedItems);
    if (!(bag_inv && bag_inv->items.data[equip_slot]))
        goto leave;
    GameSrv_UnequipItem(client, equip_slot, bag_ptr, slot);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void UseInventoryItem(uint32_t item_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item *item = get_item_safe(world, item_id);
    if (!item)
        goto leave;
    GameSrv_UseItem(client, item);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void PickupItem(uint32_t item_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item *item = get_item_safe(world, item_id);
    if (!item)
        goto leave;
    GameSrv_InteractItem(client, item->agent_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void InteractAgent(AgentId agent_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Agent *agent = get_agent_safe(world, agent_id);
    if (!agent) goto leave;
    switch (agent->type) {
    case AgentType_Living:
        GameSrv_InteractLiving(client, agent->agent_id);
        break;
    case AgentType_Gadget:
        GameSrv_InteractGadget(client, agent->agent_id);
        break;
    case AgentType_Item:
        GameSrv_InteractItem(client, agent->agent_id);
        break;
    default:
        LogError("Unsupported agent type %d", agent->type);
        break;
    }
leave:
    thread_mutex_unlock(&client->mutex);
}

// to deposit put negative number, to withdraw put positive
bool compute_gold_character(World *world, int diff, int *gold_character, int *gold_storage)
{
    int gold_stor_after = world->inventory.gold_storage - diff;
    int gold_char_after = world->inventory.gold_character + diff;

    if (gold_stor_after < 0 || (1000*1000) < gold_stor_after) return false;
    if (gold_char_after < 0 || (100*1000)  < gold_char_after) return false;

    *gold_character = gold_char_after;
    *gold_storage   = gold_stor_after;

    return true;
}

HQAPI void DepositGold(int quant)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    int gold_character, gold_storage;
    if (!compute_gold_character(world, -quant, &gold_character, &gold_storage))
        goto leave;
    GameSrv_ChangeGold(client, gold_character, gold_storage);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void WithdrawGold(int quant)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    int gold_character, gold_storage;
    if (!compute_gold_character(world, quant, &gold_character, &gold_storage))
        goto leave;
    GameSrv_ChangeGold(client, gold_character, gold_storage);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI int GetGoldStorage(void)
{
    assert(client != NULL);
    int gold = 0;
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    gold = client->world.inventory.gold_storage;
leave:
    thread_mutex_unlock(&client->mutex);
    return gold;
}

HQAPI int GetGoldCharacter(void)
{
    assert(client != NULL);
    int gold = 0;
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    gold = client->world.inventory.gold_character;
leave:
    thread_mutex_unlock(&client->mutex);
    return gold;
}

HQAPI const SkillInfo *GetSkillInfo(uint32_t skill_id)
{
    (void)skill_id;
    assert(client != NULL);
    return NULL;
}

HQAPI void GetSkillbar(uint32_t *skills, AgentId agent_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (agent_id == 0)
        agent_id = world->player_agent_id;
    Skillbar *sb = get_skillbar_safe(world, agent_id);
    if (!sb) goto leave;
    for (size_t i = 0; i < 8; i++)
        skills[i] = sb->skills[i].skill_id;
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI bool GetSkillCasting(size_t pos, AgentId *target_id)
{
    assert(client != NULL);
    assert(1 <= pos && pos <= 8);
    bool is_casting = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Skillbar *skillbar = get_skillbar_safe(world, world->player_agent_id);
    if (!skillbar)
        goto leave;
    Skill *skill = &skillbar->skills[pos - 1];
    is_casting = skill->casting || skill->casting_confirmed;
    // @Fix: We don't save the agent id;
    if (target_id)
        *target_id = 0;
leave:
    thread_mutex_unlock(&client->mutex);
    return is_casting;
}

HQAPI msec_t GetSkillRecharge(int pos)
{
    assert(client != NULL);
    assert(1 <= pos && pos <= 8);
    msec_t remaining = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Skillbar *skillbar = get_skillbar_safe(world, world->player_agent_id);
    if (!skillbar)
        goto leave;
    Skill *skill = &skillbar->skills[pos - 1];
    // @Cleanup: Actually save the time
    if (skill->recharging)
        remaining = (msec_t)-1;
leave:
    thread_mutex_unlock(&client->mutex);
    return remaining;
}

HQAPI void UseSkill(uint32_t skill_id, AgentId target_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_UseSkill(client, skill_id, 0, target_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void Attack(AgentId target_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_Attack(client, target_id, false);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void LoadPlayerSkillTemplate(const char* temp)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;

    SkillTemplate* skillTemplate = template_decode(temp);
    if (!skillTemplate) {
        LogDebug("Unable to decode");
        goto leave;
    }

    Skillbar* sb = get_skillbar_safe(world, GetMyAgentId());
    if (!sb) goto leave;

    if (sb->prof1 != skillTemplate->primary) {
        LogDebug("Invalid primary profession %u / %u", sb->prof1, skillTemplate->primary);
        goto leave;
    }

    if (sb->prof2 != skillTemplate->secondary && sb->prof2 != Profession_None) {
        GameSrv_PlayerChangeSecondary(client, skillTemplate->secondary);
    }

    GameSrv_PlayerLoadSkills(client, skillTemplate->skills);
    GameSrv_PlayerLoadAttributes(client, skillTemplate->attributes);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void LoadHeroSkillTemplate(const char* temp, uint32_t hero_index)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL)
        goto leave;
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index)) {
        goto leave;
    }
    AgentId hero_agent_id = heroes.data[hero_index].agent_id;

    SkillTemplate* skillTemplate = template_decode(temp);
    if (!skillTemplate) {
        goto leave;
    }

    Skillbar* sb = get_skillbar_safe(world, hero_agent_id);
    if (!sb) {
        goto leave;
    }

    if (sb->prof1 != skillTemplate->primary) {
        goto leave;
    }

    if (sb->prof2 != skillTemplate->secondary && sb->prof2 != Profession_None) {
        GameSrv_ChangeSecondary(client, hero_agent_id, skillTemplate->secondary);
    }

    GameSrv_LoadSkills(client, hero_agent_id, skillTemplate->skills);
    GameSrv_LoadAttributes(client, hero_agent_id, skillTemplate->attributes);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroAdd(HeroID hero_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;

    GameSrv_AddHero(client, hero_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroKick(HeroID hero_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;

    GameSrv_KickHero(client, hero_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroKickAll()
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;

    GameSrv_KickHero(client, HeroID_Count);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroFlagCancel(uint32_t hero_index)
{
    HeroFlag((Vec2f) { INFINITY, INFINITY }, hero_index);
}

HQAPI void HeroFlag(Vec2f pos, uint32_t hero_index)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL)
        goto leave;
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index))
        goto leave;
    AgentId hero_id = heroes.data[hero_index].agent_id;
    GameSrv_FlagHero(client, pos, hero_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroFlagCancelAll()
{
    HeroFlagAll((Vec2f) { INFINITY, INFINITY });
}

HQAPI void HeroFlagAll(Vec2f pos)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_FlagAllHero(client, pos);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroUseSkill(uint32_t hero_index, uint32_t skill_id, AgentId target_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL)
        goto leave;
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index))
        goto leave;
    AgentId hero_id = heroes.data[hero_index].agent_id;
    GameSrv_HeroUseSkill(client, hero_id, skill_id, 0, target_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroEnableSkill(uint32_t hero_index, uint32_t skill_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL)
        goto leave;
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index))
        goto leave;
    AgentId hero_id = heroes.data[hero_index].agent_id;
    Skillbar *sb = get_skillbar_safe(world, hero_id);
    if (!sb) goto leave;
    Skill *skill = skillbar_get_skill_by_id(sb, skill_id);
    if (!skill || !skill->disable)
        goto leave;
    int pos = indexof(sb->skills, skill);
    GameSrv_HeroSkillToggle(client, hero_id, pos);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroDisableSkill(uint32_t hero_index, uint32_t skill_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = get_player_party_safe(world, world->player_id)) == NULL) {
        goto leave;
    }
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index))
        goto leave;
    AgentId hero_id = heroes.data[hero_index].agent_id;
    Skillbar *sb = get_skillbar_safe(world, hero_id);
    if (!sb) goto leave;
    Skill *skill = skillbar_get_skill_by_id(sb, skill_id);
    if (!skill || skill->disable)
        goto leave;
    int pos = indexof(sb->skills, skill);
    GameSrv_HeroSkillToggle(client, hero_id, pos);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void HeroSetBehavior(AgentId hero_index, HeroBehavior behavior)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = get_player_party_safe(world, world->player_id)) == NULL) {
        goto leave;
    }
    ArrayPartyHero heroes = party->heroes;
    if (!array_inside(&heroes, hero_index))
        goto leave;
    AgentId hero_id = heroes.data[hero_index].agent_id;
    GameSrv_HeroSetBehavior(client, hero_id, behavior);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI size_t GetPartySize(uint32_t party_id)
{
    assert(client != NULL);
    size_t party_size = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party = get_party_safe(world, party_id);
    if (!party) goto leave;
    party_size += party->heroes.size;
    party_size += party->players.size;
    party_size += party->henchmans.size;
leave:
    thread_mutex_unlock(&client->mutex);
    return party_size;
}

HQAPI uint32_t GetPartyLeader(uint32_t party_id)
{
    assert(client != NULL);
    uint32_t player_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party = NULL;
    if (party_id) {
        party = get_party_safe(world, party_id);
    } else {
        party = GetPlayerPartyPtr(world);
    }
    if (!party) goto leave;
    ArrayPartyPlayer players = party->players;
    if (players.size > 0)
        player_id = players.data[0].player_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return player_id;
}

uint32_t GetPartyOfPlayer(uint32_t player_id)
{
    (void)player_id;
    assert(client != NULL);
    uint32_t party_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL) {
        goto leave;
    }
    party_id = party->party_id;
leave:
    thread_mutex_unlock(&client->mutex);
    return party_id;
}

HQAPI bool GetPartyIsDefeated(void)
{
    assert(client != NULL);
    bool defeated = false;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Party *party;
    if ((party = GetPlayerPartyPtr(world)) == NULL) {
        goto leave;
    }
    defeated = party->defeated;
leave:
    thread_mutex_unlock(&client->mutex);
    return defeated;
}

HQAPI void LeaveParty(void)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!(client->ingame && client->world.hash))
        goto leave;
    GameSrv_LeaveParty(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void AcceptInvite(uint32_t party_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!(client->ingame && client->world.hash))
        goto leave;
    GameSrv_AcceptInvite(client, party_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void RefuseInvite(uint32_t party_id)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!(client->ingame && client->world.hash))
        goto leave;
    GameSrv_RefuseInvite(client, party_id);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void GetPartyRecvInvites(uint32_t *buffer, size_t length);
HQAPI void GetPartySentInvites(uint32_t *buffer, size_t length);

HQAPI void SeekParty(PartySearchType type, const char *msg)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    DECLARE_KSTR(kmsg, 32);
    kstr_read_ascii(&kmsg, msg, strlen(msg));
    GameSrv_PS_SeekParty(client, type, &kmsg);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI uint32_t GetTraderId(void)
{
    assert(client != NULL);
    uint32_t player_id = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    TradeSession *session = &world->trade_session;
    if (!(session->state & TradeState_Opened))
        goto leave;
    player_id = session->trader;
leave:
    thread_mutex_unlock(&client->mutex);
    return player_id;
}

HQAPI int GetTradeGold(void)
{
    assert(client != NULL);
    int gold = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    TradeSession *session = &world->trade_session;
    if (!(session->state & TradeState_Received))
        goto leave;
    gold = session->trader_gold;
leave:
    thread_mutex_unlock(&client->mutex);
    return gold;
}

HQAPI TradeState GetTradeState(void)
{
    assert(client != NULL);
    TradeState state = TradeState_Closed;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    state = world->trade_session.state;
leave:
    thread_mutex_unlock(&client->mutex);
    return state;
}

HQAPI size_t GetTradeItems(TradeItem *buffer, size_t length)
{
    assert(client != NULL);
    size_t count = 0;
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    TradeSession *session = &world->trade_session;
    if (!(session->state & TradeState_Opened))
        goto leave;
    TradeItemArray items = session->trader_items;
    if (buffer == NULL) {
        count = items.size;
        goto leave;
    }
    for (size_t i = 0; (i < items.size) && (count < length); i++) {
        buffer[count++] = items.data[i];
    }
leave:
    thread_mutex_unlock(&client->mutex);
    return count;
}

HQAPI void TradeInitiate(AgentId partner)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_TradeInitiate(client, partner);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TradeAddItem(uint32_t item_id, uint32_t quantity)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item *item = get_item_safe(world, item_id);
    if (!(item && quantity && (item->quantity >= quantity)))
        goto leave;
    quantity = quantity ? quantity : item->quantity;
    if ((uint32_t)UINT8_MAX < quantity)
        goto leave;
    GameSrv_TradeAddItem(client, item_id, (uint8_t)quantity);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TradeRemoveItem(uint32_t item_id, uint32_t quantity)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_TradeRemoveItem(client, item_id, quantity);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TradeSendOffer(int gold)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_TradeSendOffer(client, gold);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TradeAccept(void)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_TradeAccept(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void TradeCancel(void)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    if (!client->ingame)
        goto leave;
    GameSrv_TradeCancel(client);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI bool ItemSalvage(ApiItem* apiItem, ApiItem *apiKit) {
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    bool attempt = false;
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (!apiItem || !apiKit)
        goto leave;
    if (apiItem->item_id == apiKit->item_id)
        goto leave;
    Item* item = get_item_safe(world, apiItem->item_id);
    if (!item)
        goto leave;
    if (!item->bag || item->bag->bag_id == BagEnum_EquipmentPack)
        goto leave;
#if 0
    if (!(item->value)) // in toolbox no value is considered most likely not salvageable, we're probably missing something here though as this doesn't work as intended
        goto leave;
#endif
    if (item->flags) {
        if ((item->flags & 0x1000000) != 0) // usable
            goto leave;
        if ((item->flags & 0x10) != 0) // green
            goto leave;
        // @fixme blue && unidentified
    }

    if (apiKit->type != ItemType_Kit)
        goto leave;

    if (apiKit->model_id != 2991 && apiKit->model_id != 2992 && apiKit->model_id != 5900) //
        goto leave;

    Item* kit = get_item_safe(world, apiKit->item_id);
    if (!kit)
        goto leave;

    attempt = true;
    GameSrv_StartSalvage(client, kit, item);
leave:
    thread_mutex_unlock(&client->mutex);
    return attempt;
}

HQAPI void SalvageMaterials() {
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    GameSrv_SalvageMaterials(client);
    thread_mutex_unlock(&client->mutex);
}

HQAPI bool ItemIdentify(ApiItem* apiItem, ApiItem *apiKit) {
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    bool attempt = false;
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    if (!apiItem || !apiKit)
        goto leave;

    Item* item = get_item_safe(world, apiItem->item_id);
    if (!item)
        goto leave;
    if ((item->flags & 1) != 0) // identified
        goto leave;

    if (apiKit->type != ItemType_Kit)
        goto leave;
    if (apiKit->model_id != 2989 && apiKit->model_id != 5899)
        goto leave;

    Item* kit = get_item_safe(world, apiKit->item_id);
    if (!kit)
        goto leave;

    GameSrv_Identify(client, kit, item);
    attempt = true;
leave:
    thread_mutex_unlock(&client->mutex);
    return attempt;
}

HQAPI void MerchantBuyItem(uint32_t model_id, uint32_t gold_value)
{
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);
    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item* item = NULL;
    ArrayItem items = world->merchant_items;
    for (size_t i = 0; (i < items.size); i++) {
        if (!items.data[i]) continue;
        if (items.data[i]->model_id == model_id) {
            item = items.data[i];
            break;
        }
    }
    if (!item)
        goto leave;

    TransactionInfo item_send = { 0 };
    uint32_t gold_recv = 0;
    TransactionInfo item_recv = { 0 };
    item_recv.item_count = 1;
    item_recv.item_ids[0] = item->item_id;
    item_recv.item_quants[0] = 1;

    GameSrv_TransactItems(client, TransactionType_MerchantBuy, gold_value, &item_send, gold_recv, &item_recv);
leave:
    thread_mutex_unlock(&client->mutex);
}

HQAPI void MerchantSellItem(ApiItem* apiItem) {
    assert(client != NULL);
    thread_mutex_lock(&client->mutex);

    World *world;
    if ((world = get_world(client)) == NULL)
        goto leave;
    Item* item = get_item_safe(world, apiItem->item_id);
    if (!item)
        goto leave;

    TransactionInfo item_recv = { 0 };
    TransactionInfo item_send = { 0 };
    item_send.item_count = 1;
    item_send.item_ids[0] = item->item_id;
    item_send.item_quants[0] = item->quantity;

    GameSrv_BuyMaterials(client, TransactionType_MerchantSell, 0, &item_send, item->value * item->quantity, &item_recv);
leave:
    thread_mutex_unlock(&client->mutex);
}
