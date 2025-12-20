#ifdef CORE_AGENT_C
#error "agent.c included more than once"
#endif
#define CORE_AGENT_C

#define AG_ATTR_FREEZE_PLAYER   (8)
#define AG_ATTR_SET_MARKER      (11)
#define AG_ATTR_REMOVE_MARKER   (12)
#define AG_ATTR_DAMAGE          (16)
#define AG_ATTR_CRITICAL_DAMAGE (17)
#define AG_ATTR_EARTH_SHAKING   (21)
#define AG_ATTR_SET_LEVEL       (36)
#define AG_ATTR_LEVEL_UP        (37)
#define AG_ATTR_SET_ENERGY      (41)
#define AG_ATTR_SET_HEALTH      (42)
#define AG_ATTR_SET_ENERGY_PS   (43)
#define AG_ATTR_SET_HEALTH_PS   (44)
#define AG_ATTR_ARMOR_IGNORING  (55)

void agent_set_distination(Agent *agent, Vec2f dest);

void ensure_agent_exist(World *world, AgentId id);
void remove_agent(World *world, AgentId id);

bool v2_equals(Vec2f v1, Vec2f v2)
{
    return (v1.x == v2.x) && (v1.y == v2.y);
}

float lerpf(float p1, float p2, float t)
{
    return t * p1 + (1 - t) * p2;
}

float clampf(float val, float min, float max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void agent_set_distination(Agent *agent, Vec2f dest)
{
    Vec2f pos = agent->position;

    float dx = dest.x - pos.x;
    float dy = dest.y - pos.y;
    float dist = sqrtf((dx * dx) + (dy * dy));

    agent->moving = true;
    agent->destination = dest;

    agent->direction.x = dx / dist;
    agent->direction.y = dy / dist;

    agent->rotation = atan2f(agent->direction.y, agent->direction.x);
}

Agent *get_agent_safe(World *world, AgentId id)
{
    if (!(world && id)) {
        return NULL;
    }
    ArrayAgent agents = world->agents;
    if (!array_inside(&agents, id))
        return NULL;
    return array_at(&agents, id);
}

void ensure_agent_exist(World *world, AgentId id)
{
    ArrayAgent *agents = &world->agents;
    if (array_inside(agents, id) && array_at(agents, id))
        return;

    if (!array_inside(agents, id)) {
        array_resize(agents, id + 1);
        agents->size = agents->capacity;
    }

    Agent *agent = calloc(1, sizeof(*agent));
    agent->agent_id = id;
    agent->speed_modifier = 1.f;
    array_set(agents, id, agent);
}

void remove_agent(World *world, AgentId id)
{
    ArrayAgent *agents = &world->agents;
    if (!array_inside(agents, id))
        return;

    free(agents->data[id]);
    agents->data[id] = NULL;
}

void update_agents_position(ArrayAgent *agents, msec_t diff)
{
    float delta = diff / 1000.f;

    Agent **it;
    array_foreach(it, agents) {
        Agent *agent = *it;

        if (agent && agent->moving) {
            agent_update_api(agent);
            float speed = agent->speed;

            float rem_x = agent->destination.x - agent->position.x;
            float rem_y = agent->destination.y - agent->position.y;

            float dist = delta * speed;
            float dist_rem = sqrtf((rem_x * rem_x) + (rem_y * rem_y));

            float dx = agent->direction.x * dist;
            float dy = agent->direction.y * dist;

            if (dist >= dist_rem) {
                agent->position.x = agent->destination.x;
                agent->position.y = agent->destination.y;
                agent_stop_moving(agent);
            } else {
                agent->position.x += dx;
                agent->position.y += dy;
            }

            // update health & energy
            float health_diff = delta * agent->health_per_sec;
            agent->health = clampf(agent->health + health_diff, 0.f, 1.f);

            float energy_diff = delta * agent->energy_per_sec;
            agent->energy = clampf(agent->energy + energy_diff, 0.f, 1.f);
        }
    }
}

void HandleAgentMovementTick(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t delta;
    } MovementTick;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_MOVEMENT_TICK);
    assert(sizeof(MovementTick) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    MovementTick *pack = cast(MovementTick *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world(client);
    if (!world)
        return; // This can happen if we've just started travelling!

    world->time_server += pack->delta;

    // We can definitly call this function more often with a smaller delta.
    // For instance every frame of our application to get a more exact position.
    // update_agents_position(&world->agents, (msec_t)pack->delta);
}

void HandleAgentInstanceTimer(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t time;
    } InstanceTimer;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_INSTANCE_TIMER);
    assert(sizeof(InstanceTimer) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    InstanceTimer *pack = cast(InstanceTimer *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world(client);
    if (!world) return;

    world->load_time = pack->time;
    world->time_server = pack->time;
}

void HandleAgentSpawned(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        /* +h0000 */ Header header;
        /* +h0002 */ uint32_t agent_id;
        /* +h0006 */ uint32_t model_id; // ho byte: 2=npc, 3=player
        /* +h000A */ uint8_t type; // 1=living, 2=gadget, 4=item
        /* +h000B */ uint8_t h000B;
        /* +h000C */ Vec2f position;
        /* +h0014 */ int16_t plane;
        /* +h0016 */ Vec2f direction;
        /* +h001E */ int8_t h001E;
        /* +h001F */ float speed_base;
        /* +h0023 */ float h0023;
        /* +h0027 */ int32_t h0027;
        /* +h002B */ int32_t model_type;
        /* +h002F */ int32_t h002F;
        /* +h0033 */ int32_t h0033;
        /* +h0037 */ int32_t h0037;
        /* +h003B */ int32_t h003B;
        /* +h003F */ int32_t h003F;
        /* +h0043 */ float h0043;
        /* +h0047 */ float h0047;
        /* +h004B */ float h004B;
        /* +h004F */ float h004F;
        /* +h0053 */ int16_t h0053;
        /* +h0055 */ int32_t h0055;
        /* +h0059 */ float h0059;
        /* +h005D */ float h005D;
        /* +h0061 */ int16_t h0061;
    } AgentSpawned;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_SPAWNED);
    assert(sizeof(AgentSpawned) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AgentSpawned *pack = cast(AgentSpawned *)packet;
    assert(client && client->game_srv.secured);
    World* world = get_world(client);
    if (!world) return;

    ArrayAgent *agents = &world->agents;
    ensure_agent_exist(world, pack->agent_id);
    Agent *agent = array_at(agents, pack->agent_id);
    assert(agent != NULL);

    agent->position = pack->position;
    agent->speed_base = pack->speed_base;

    // @Remark: npc_id_kind_of = (factionColor << 24) | npc_id
    uint32_t id  = pack->model_id & 0xffffff;
    uint32_t ext = (pack->model_id >> 28) & 0xf;

    agent->type = pack->type;
    switch (pack->type) {
    case 1:
        switch (ext) {
        case 2: agent->npc_id    = id; break;
        case 3: agent->player_id = id; break;
        default:
            LogError("Unknow AgentType_Living extra type: %u (model_id: %u, agent_id: %u)",
                ext, pack->model_id, pack->agent_id);
        }
        break;
    case 2:
        agent->gadget_id = id;
        break;
    case 4: {
        agent->item_id = id;
        Item *item = get_item_safe(world, agent->item_id);
        if (item) item->agent_id = agent->agent_id;
        break;
    }
    case 3:
        // printf("Pos (%f, %f)\n", agent->position.x, agent->position.y);
        break;
    default:
        LogError("Unknow agent type %u (model_id: %u, agent_id: %u)",
            pack->type, pack->model_id, pack->agent_id);
    }

    agent->spawned = true;
    agent->direction = pack->direction;
    agent->rotation = atan2f(pack->direction.y, pack->direction.x);
}

void HandleAgentDespawned(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
    } AgentDespawned;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_DESPAWNED);
    assert(sizeof(AgentDespawned) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AgentDespawned *pack = cast(AgentDespawned *)packet;
    assert(client && client->game_srv.secured);

    World* world = get_world(client);
    if (!world) return;
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) return;
    agent->spawned = false;

    if (agent->item_id) {
        Item *item = get_item_safe(world, agent->item_id);
        if (item) item->agent_id = 0;
    }

    remove_agent(world, pack->agent_id);

    Event params;
    Event_Init(&params, EventType_AgentDespawned);
    params.AgentDespawned.agent_id = pack->agent_id;
    broadcast_event(&client->event_mgr, &params);
}

void HandleAgentStopMoving(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int32_t agent_id;
    } AgentDespawned;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_STOP_MOVING);
    assert(sizeof(AgentDespawned) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AgentDespawned *pack = cast(AgentDespawned *)packet;
    assert(client && client->game_srv.secured);

    World* world = get_world(client);
    if (!world) return;
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("Received 'AgentStopMoving' before agent %d spawned", pack->agent_id);
        return;
    }

    // LogDebug("agent_stop_moving %d", pack->agent_id);
    // agent_stop_moving(agent);
}

void player_is_created(GwClient *client, AgentId agent_id);
void HandleAgentSetPlayer(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        int32_t unk0;
    } SetPlayer;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_SET_PLAYER);
    assert(sizeof(SetPlayer) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    SetPlayer *pack = cast(SetPlayer *)packet;
    assert(client && client->game_srv.secured);

    World* world = get_world(client);
    if (!world) return;
    Agent *me = get_agent_safe(world, pack->agent_id);
    // @Cleanup:
    // When we enter a cinematic our player id is set, but no agent are spawned.
    // It make sense since agent can be created, but not spawned.
    // We need a second array for created agents or a flag for spwned agents.
    if (me != NULL) {
        // @Remark:
        // Since the client receive many player's related information before we know
        // the player id, we store them and when we get them, write them into Player struct.
        player_is_created(client, pack->agent_id);
    }
}

void HandleAgentUpdateDirection(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        AgentId agent_id;
        Vec2f   direction;
        uint8_t orientation; // u8
    } UpdateDirection;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_DIRECTION);
    assert(sizeof(UpdateDirection) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateDirection *pack = cast(UpdateDirection *)packet;
    assert(client && client->game_srv.secured);

    World* world = get_world(client);
    if (!world) return;
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("AgentUpdateDirection received before the agent %d spawned.", pack->agent_id);
        return;
    }

    agent->direction = pack->direction;
    agent->rotation = atan2f(agent->direction.y, agent->direction.x);
}

void HandleAgentUpdateSpeedBase(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        float speed_base;
    } UpdateSpeedBase;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_SPEED_BASE);
    assert(sizeof(UpdateSpeedBase) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateSpeedBase *pack = cast(UpdateSpeedBase *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("AgentUpdateSpeedBase received before the agent %d spawned.", pack->agent_id);
        return;
    }

    agent->speed_base = pack->speed_base;
}

void HandleAgentUpdateSpeedModifier(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        float speed_modifier;
        uint8_t unk0; // rotation ?
    } UpdateSpeedModifier;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_SPEED);
    assert(sizeof(UpdateSpeedModifier) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateSpeedModifier *pack = cast(UpdateSpeedModifier *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world(client);
    if (!world) {
        LogError("AgentUpdateSpeedModifier received before the world %d spawned.", pack->agent_id);
        return;
    }
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("AgentUpdateSpeedModifier received before the agent %d spawned.", pack->agent_id);
        return;
    }

    agent->speed_modifier = pack->speed_modifier;
}

void HandleAgentUpdatePosition(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        Vec2f position;
        int16_t plane;
    } UpdatePosition;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_POSITION);
    assert(sizeof(UpdatePosition) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdatePosition *pack = cast(UpdatePosition *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("AgentUpdatePosition received before the agent %d spawn.", pack->agent_id);
        return;
    }

    agent->position = pack->position;
}

void HandleAgentUpdateRotation(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint32_t agent_id;
        uint32_t rotation;
        uint32_t unk0;
    } UpdateRotation;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_ROTATION);
    assert(sizeof(UpdateRotation) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateRotation *pack = cast(UpdateRotation *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world(client);
    Agent *agent = world ? get_agent_safe(world, pack->agent_id) : NULL;
    if (!agent) {
        LogError("AgentUpdateRotation received before then agent %d spawn.", pack->agent_id);
        return;
    }

    // @Cleanup: Not portable
    memcpy(&agent->rotation, &pack->rotation, 4);
    agent->direction.x = cosf(agent->rotation);
    agent->direction.y = sinf(agent->rotation);
}

void HandleAgentMoveToPoint(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        Vec2f dest;
        int16_t plane;
        int16_t current_plane;
    } Destination;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_MOVE_TO_POINT);
    assert(sizeof(Destination) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    Destination *pack = cast(Destination *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world(client);

    // @Cleanup:
    // It happend to receive a `MoveToPoint` before spawning an agent.
    // In this case we just ignore the packet.
    // We should still process it, but the current system doesn't allow to do it effeciently.
    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogInfo("We receive a AgentMoveToPoint for agent '%d' before spawning it.", pack->agent_id);
        return;
    }

    if (agent->agent_id == world->player_agent_id) {
        free_dialog_info(&world->dialog);
        world->interact_with = 0;
    }
    agent_set_distination(agent, pack->dest);
}

void HandleAgentUpdateDestination(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        Vec2f dest;
        int16_t plane;
        int16_t unk0;
        int32_t target_id; // agent id
    } Destination;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_DESTINATION);
    assert(sizeof(Destination) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    Destination *pack = cast(Destination *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("AgentUpdateDestination received before the agent %d spawned.", pack->agent_id);
        return;
    }

    if (agent->agent_id == world->player_agent_id) {
        free_dialog_info(&world->dialog);
        world->interact_with = 0;
    }
    agent_set_distination(agent, pack->dest);
}

void HandleAgentDestroyPlayer(Connection* conn, size_t psize, Packet* packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header      header;
        int32_t     player_id;
    } PlayerInfo;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_DESTROY_PLAYER);
    assert(sizeof(PlayerInfo) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    PlayerInfo* pack = cast(PlayerInfo*)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    assert(array_inside(&world->players, pack->player_id));
    /*Player* player = array_at(players, pack->player_id);
    game_object_free(&client->object_mgr, &player->object);*/

    world->player_count--;

    Player* player = array_at(&world->players, pack->player_id);
    if (player && player->agent_id) {
        Event params;
        Event_Init(&params, EventType_AgentDespawned);
        params.AgentDespawned.agent_id = player->agent_id;
        broadcast_event(&client->event_mgr, &params);
    }
}

void HandleUpdatePlayerInfo(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header      header;
        int32_t     player_id;
        AgentId     agent_id;
        Appearance  appearance;
        int8_t      unk0;
        int32_t     unk1;
        int32_t     unk2;
        uint16_t    name[32];
    } PlayerInfo;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_UPDATE_PLAYER_INFO);
    assert(sizeof(PlayerInfo) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    PlayerInfo *pack = cast(PlayerInfo *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    ArrayAgent *agents = &world->agents;
    ensure_agent_exist(world, pack->agent_id);
    Agent *agent = array_at(agents, pack->agent_id);
    assert(agent);

    ArrayPlayer *players = &world->players;
    if (!array_inside(players, pack->player_id)) {
        array_resize(players, pack->player_id + 1);
        players->size = players->capacity;
    }

    assert(array_inside(players, pack->player_id));
    Player *player = array_at(players, pack->player_id);

    if (player == NULL) {
        player = calloc(1, sizeof(*player));
        assert(player != NULL);
        init_player(player);
        array_set(players, pack->player_id, player);
    }

    world->player_count++;

    init_player(player);
    player->player_id = pack->player_id;
    player->agent_id = pack->agent_id;

    kstr_hdr_read(&player->name, pack->name, ARRAY_SIZE(pack->name));
    agent->player_id = pack->player_id;
}

void HandleAgentUpdateProfession(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        uint8_t prof1;
        uint8_t prof2;
    } UpdateProfession;
#pragma pack(pop)
    
    assert(packet->header == GAME_SMSG_AGENT_UPDATE_PROFESSION);
    assert(sizeof(UpdateProfession) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateProfession *pack = cast(UpdateProfession *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    ArrayAgent *agents = &world->agents;
    ensure_agent_exist(world, pack->agent_id);
    Agent *agent = array_at(agents, pack->agent_id);
    assert(agent);
    
    agent->prof1 = (Profession)pack->prof1;
    agent->prof2 = (Profession)pack->prof2;
}

void HandleAgentAttrUpdateInt(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t attr_id;
        AgentId agent_id;
        int32_t value;
    } AttrValue;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_ATTR_UPDATE_INT);
    assert(sizeof(AttrValue) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AttrValue *pack = cast(AttrValue *)packet;
    assert(client && client->game_srv.secured);
    World* world = get_world(client);
    if (!world) {
        LogError("Failed to get_world in HandleAgentAttrUpdateInt");
        return;
    }

    int attr_id = pack->attr_id;
    if (attr_id < 0 || 66 < attr_id) {
        LogError("Unvalid generic value {id: %d, value: %d}", attr_id, pack->value);
        return;
    }

    ArrayAgent *agents = &world->agents;
    ensure_agent_exist(world, pack->agent_id);
    Agent *agent = array_at(agents, pack->agent_id);
    assert(agent);

    switch (pack->attr_id) {
    case AG_ATTR_SET_MARKER:
        agent->marker = (AgentMarker)pack->value;
        break;

    case AG_ATTR_REMOVE_MARKER:
        agent->marker = AgentMarker_None;
        break;

    case AG_ATTR_SET_LEVEL:
        agent->level = pack->value;
        break;

    case AG_ATTR_LEVEL_UP:
        agent->level += 1;
        break;

    case AG_ATTR_SET_ENERGY:
        agent->energy = 1.f;
        agent->energy_max = pack->value;
        break;

    case AG_ATTR_SET_HEALTH:
        agent->health = 1.f;
        agent->health_max = pack->value;
        break;
    }

    // update_agent_attribute(agent, attr_id, pack->value);
}

void HandleAgentAttrUpdateIntTarget(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t attr_id;
        AgentId target_id;
        AgentId cause_id;
        int32_t value;
    } AttrValue;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_ATTR_UPDATE_INT_TARGET);
    assert(sizeof(AttrValue) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AttrValue *pack = cast(AttrValue *)packet;
    assert(client && client->game_srv.secured);

    int attr_id = pack->attr_id;
    if (attr_id < 0 || 65 < attr_id) {
        LogError("Unvalid generic value {id: %d, value: %d}", attr_id, pack->value);
        return;
    }
}

void HandleAgentAttrUpdateFloat(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t attr_id;
        AgentId agent_id;
        float value;
    } AttrValue;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_ATTR_UPDATE_FLOAT);
    assert(sizeof(AttrValue) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AttrValue *pack = cast(AttrValue *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    int attr_id = pack->attr_id;
    if (attr_id < 0 || 65 < attr_id) {
        LogError("Unvalid generic value {id: %d, value: %f}", attr_id, pack->value);
        return;
    }

    ArrayAgent *agents = &world->agents;
    Agent* agent = array_inside(agents, pack->agent_id) ? array_at(agents, pack->agent_id) : NULL;
    if (!agent) {
        LogError("HandleAgentAttrUpdateFloat: received for agent '%d' not spawned", pack->agent_id);
        return;
    }
    

    switch (pack->attr_id) {
    case AG_ATTR_SET_ENERGY_PS:
        agent->energy_per_sec = pack->value;
        break;
    case AG_ATTR_SET_HEALTH_PS:
        agent->health_per_sec = pack->value;
        break;
    }

    // update_agent_attribute(agent, attr_id, pack->value);
}

void HandleAgentAttrUpdateFloatTarget(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t attr_id;
        AgentId target_id;
        AgentId cause_id;
        float value;
    } AttrValue;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_ATTR_UPDATE_FLOAT_TARGET);
    assert(sizeof(AttrValue) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AttrValue *pack = cast(AttrValue *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    int attr_id = pack->attr_id;
    if (attr_id < 0 || 65 < attr_id) {
        LogError("Unvalid generic value {id: %d, value: %f}", attr_id, pack->value);
        return;
    }

    ArrayAgent *agents = &world->agents;
    if (!array_inside(agents, pack->target_id)) {
        LogError("HandleAgentAttrUpdateFloatTarget: received for target '%d' not spawned", pack->target_id);
        return;
    }

    Agent *target = array_at(agents, pack->target_id);
    if (target == NULL) {
        LogError("HandleAgentAttrUpdateFloatTarget: received for target '%d' not spawned", pack->target_id);
        return;
    }


    switch (pack->attr_id) {
    case AG_ATTR_DAMAGE:
    case AG_ATTR_CRITICAL_DAMAGE:
    case AG_ATTR_ARMOR_IGNORING:
        target->health = clampf(target->health + pack->value, 0.f, (float)target->health_max);
        break;
    }
}

void HandleAgentUpdateNpcName(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        AgentId  agent_id;
        uint16_t name[32];
    } NpcName;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_NPC_NAME);
    assert(sizeof(NpcName) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}

void HandleNPCUpdateProperties(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        int32_t  model_id;
        int32_t  unk2;
        int32_t  unk3;
        int32_t  unk4;
        int32_t  unk5;
        int32_t  unk6;
        int8_t   unk7;
        int8_t   unk8;
        uint16_t name[8];
    } NPCProperties;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_NPC_UPDATE_PROPERTIES);
    assert(sizeof(NPCProperties) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}

void HandleNPCUpdateModel(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint32_t npc_id;
        uint32_t n_models;
        uint32_t models[8];
    } NpcModel;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_NPC_UPDATE_MODEL);
    assert(sizeof(NpcModel) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);

    (void)packet;
}

void HandleAgentCreateNPC(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        int32_t agent_class;
        int32_t npc_id;
    } NPCAgent;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_CREATE_NPC);
    assert(sizeof(NPCAgent) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);

    (void)packet;
}

void HandlePlayerUnlockedProfession(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        int32_t unlocked;
    } UnlockedProf;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_PLAYER_UNLOCKED_PROFESSION);
    assert(sizeof(UnlockedProf) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UnlockedProf *pack = cast(UnlockedProf *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    // Not saving the value for other agents.
    if (world->player_agent_id != pack->agent_id) {
        return;
    }

    Player *player;
    if ((player = get_player_safe(world, world->player_id)) == NULL) {
        LogWarn("Can't update unlocked profession, the player doesn't exist");
        return;
    }

    player->unlocked_profession = pack->unlocked;
}

void HandlePlayerUpdateProfession(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        int8_t prof1;
        int8_t prof2;
        int8_t is_pvp;
    } UpdateProf;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_PLAYER_UPDATE_PROFESSION);
    assert(sizeof(UpdateProf) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateProf *pack = cast(UpdateProf *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    // 1. Update in agents array
    ensure_agent_exist(&client->world, pack->agent_id);
    Agent* agent = array_at(&client->world.agents, pack->agent_id);
    assert(agent);

    agent->prof1 = (Profession)pack->prof1;
    agent->prof2 = (Profession)pack->prof2;

    // 2. Update in players array
    Skillbar *sb;
    array_foreach(sb, &world->skillbars) {
        if (sb->owner_agent_id == pack->agent_id)
            break;
    }

    if (sb == NULL) {
        //LogError("Couldn't find agent %d skillbar", pack->agent_id);
        return;
    }

    sb->prof1 = pack->prof1;
    sb->prof2 = pack->prof2;
}

void HandleAgentUpdateEffects(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        AgentId  agent_id;
        uint32_t effects;
    } UpdateEffects;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_UPDATE_EFFECTS);
    assert(sizeof(UpdateEffects) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    UpdateEffects *pack = cast(UpdateEffects *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    Agent *agent = get_agent_safe(world, pack->agent_id);
    if (!agent) {
        LogError("Receive 'AgentUpdateEffects', but agent %d isn't spawned", pack->agent_id);
        return;
    }
    agent->effects = pack->effects;
}

void GameSrv_MoveToCoord(GwClient *client, float x, float y)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        Vec2f pos;
        int32_t plane;
    } MovePacket;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    MovePacket move_pack = NewPacket(GAME_CMSG_MOVE_TO_COORD);
    move_pack.pos.x = x;
    move_pack.pos.y = y;
    move_pack.plane = 0;

    SendPacket(&client->game_srv, sizeof(move_pack), &move_pack);

    World *world = get_world_or_abort(client);
    Agent *player = array_at(&world->agents, world->player_agent_id);
    assert(player != NULL);
    player->maybe_moving = true;
}

void GameSrv_InteractLiving(GwClient *client, AgentId agent_id)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        uint8_t unk0;
    } Payload;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    Payload packet = NewPacket(GAME_CMSG_INTERACT_LIVING);
    packet.agent_id = agent_id;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_InteractGadget(GwClient *client, AgentId agent_id)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
        uint8_t unk0;
    } Payload;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    Payload packet = NewPacket(GAME_CMSG_INTERACT_GADGET);
    packet.agent_id = agent_id;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_RotateToAngle(GwClient *client, float rotation)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        uint32_t rotation;
        uint32_t unk;
    } RotatePlayer;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    RotatePlayer packet = NewPacket(GAME_CMSG_ROTATE_PLAYER);
    // @Cleanup: Not portable
    memcpy(&packet.rotation, &rotation, 4);
    packet.unk = 0x3F800000;
    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void HandleAgentPingeg(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        uint16_t player_id;
        uint32_t agent_id;
    } AgentPinged;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_AGENT_PINGED);
    assert(sizeof(AgentPinged) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AgentPinged *pack = cast(AgentPinged *)packet;
    assert(client && client->game_srv.secured);

    Event params;
    Event_Init(&params, EventType_AgentFocus);
    params.AgentFocus.agent_id  = pack->agent_id;
    params.AgentFocus.player_id = pack->player_id;
    broadcast_event(&client->event_mgr, &params);
}
