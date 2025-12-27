#ifdef CORE_ITEM_C
#error "item.c included more than once"
#endif
#define CORE_ITEM_C

uint32_t item_mod_identifier(uint32_t mod)
{
    return mod >> 16;
}

uint32_t item_mod_arg1(uint32_t mod)
{
    return (mod & 0x0000FF00) >> 8;
}

uint32_t item_mod_arg2(uint32_t mod)
{
    return (mod & 0x000000FF);
}

Item *alloc_item()
{
    Item *item;
    if ((item = calloc(1, sizeof(*item))) != NULL) {
        array_init(&item->mod_struct);
    }

    return item;
}

void free_item(Item *item)
{
    if (item != NULL) {
        array_reset(&item->mod_struct);
        free(item);
    }
}

void remove_item_from_bag(Item *item)
{
    if (!item->bag) return;

    Bag *bag = item->bag;
    assert(array_inside(&bag->items, item->slot));
    assert(array_at(&bag->items, item->slot));

    array_set(&bag->items, item->slot, NULL);
    item->bag = NULL;
    item->slot = 0;
    bag->item_count--;
}

Item *get_item_safe(World *world, int32_t id)
{
    ArrayItem items = world->items;
    if (!array_inside(&items, id))
        return NULL;
    return array_at(&items, id);
}

Bag *get_bag_safe(World *world, BagEnum bag_id)
{
    return world->inventory.bags[bag_id];
}

void HandleItemStreamCreate(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        uint16_t stream_id;
        uint8_t  is_hero;
    } StreamCreate;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_STREAM_CREATE);
    assert(sizeof(StreamCreate) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}

void HandleItemStreamDestroy(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t stream_id;
    } StreamDestroy;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_STREAM_DESTROY);
    assert(sizeof(StreamDestroy) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}

void HandleItemGeneralInfo(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        /* +h0000 */ Header header;
        /* +h0002 */ int32_t item_id;
        /* +h0006 */ int32_t file_id;
        /* +h000A */ int8_t type;
        /* +h000B */ int8_t unk0;
        /* +h000C */ int16_t dye_color;
        /* +h000E */ int16_t materials;
        /* +h0010 */ int8_t unk1;
        /* +h0011 */ int32_t flags; // interaction
        /* +h0015 */ int32_t value;
        /* +h0019 */ int32_t model;
        /* +h001D */ int32_t quantity;
        /* +h0021 */ uint16_t name[64];
        /* +h009D */ uint32_t n_modifier;
        /* +h00A1 */ uint32_t modifier[64];
    } ItemInfo;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_GENERAL_INFO);
    assert(sizeof(ItemInfo) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    ItemInfo *pack = cast(ItemInfo *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    // LogInfo("ItemGeneralInfo {id: %d, modele: %d}", pack->item_id, pack->model);

    ArrayItem *items = &world->items;
    if (!array_inside(items, pack->item_id)) {
        array_resize(items, pack->item_id + 1);
        items->size = items->capacity;
    }

    Item *new_item = alloc_item();
    new_item->item_id = pack->item_id;
    new_item->flags = pack->flags;
    new_item->value = pack->value;
    new_item->model_id = pack->model;
    new_item->quantity = pack->quantity;
    new_item->type = pack->type;
    new_item->value = pack->value;
    kstr_hdr_read(&new_item->name, pack->name, ARRAY_SIZE(pack->name));

    uint32_t *buffer = array_push(&new_item->mod_struct, pack->n_modifier);
    for(size_t i = 0; i < pack->n_modifier; ++i) {
        buffer[i] = pack->modifier[i];
    }

    array_set(items, pack->item_id, new_item);
}

void HandleItemRemove(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t stream_id;
        int32_t item_id;
    } ItemRemove;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_REMOVE);
    assert(sizeof(ItemRemove) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    ItemRemove *pack = cast(ItemRemove *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    ArrayItem *items = &world->items;
    Item *item = get_item_safe(world, pack->item_id);
    if (!item) return;

    array_set(items, pack->item_id, NULL);
    remove_item_from_bag(item);
    free_item(item);
}

void HandleItemWeaponSet(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t stream_id;
        int8_t slot;
        int32_t leadhand;
        int32_t offhand;
    } WeaponSet;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_WEAPON_SET);
    assert(sizeof(WeaponSet) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);

    // LogInfo("New Weapon set !");
}

void HandleInventoryItemQuantity(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t item_id;
        int32_t quantity;
    } ItemQuantity;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_UPDATE_QUANTITY);
    assert(sizeof(ItemQuantity) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    ItemQuantity *pack = cast(ItemQuantity *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    Item *item = get_item_safe(world, pack->item_id);
    if (!item) {
        LogInfo("Updated item quantity of item %d, but the item doesn't exist.", pack->item_id);
        return;
    }

    item->quantity = pack->quantity;
}

void HandleInventoryItemLocation(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t stream_id;
        int32_t item_id;
        int16_t bag_id;
        int8_t slot;
    } ItemLocation;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_MOVED_TO_LOCATION);
    assert(sizeof(ItemLocation) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    ItemLocation *pack = cast(ItemLocation *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

#if 0
    Item *item = get_item_safe(client, pack->item_id);
    if (!item) return;
#else
    ArrayItem items = world->items;
    if (!(array_inside(&items, pack->item_id) && array_at(&items, pack->item_id)))
        return;
    Item *item = array_at(&items, pack->item_id);
#endif

    BagArray *bags = &world->bags;
    uint32_t hash = hash_int32(pack->bag_id);
    uint32_t index = hash % bags->size;
    Bag *bag = &array_at(bags, index);

    while (bag->bag_id != pack->bag_id) {
        assert(bag->bag_id != 0);
        hash = hash_int32(hash);
        index = hash % bags->size;
        bag = &array_at(bags, index);
    }

    assert(bag->bag_id == pack->bag_id);
    assert(array_inside(&bag->items, pack->slot));
#if 0 // we sometimes receive this
    assert(array_at(bag->items, pack->slot) == NULL);
#endif

    item->bag = bag;
    item->slot = pack->slot;
    array_set(&bag->items, pack->slot, item);
    bag->item_count++;
}

void HandleInventoryCreateBag(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t stream_id;
        int8_t bag_type;
        int8_t bag_model_id;
        int16_t bag_id;
        int8_t slot_count;
        int32_t assoc_item_id;
    } BagInfo;
#pragma pack(pop)
    
    assert(packet->header == GAME_SMSG_INVENTORY_CREATE_BAG);
    assert(sizeof(BagInfo) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    BagInfo *pack = cast(BagInfo *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    BagArray *bags = &world->bags;
    uint32_t hash = hash_int32(pack->bag_id);
    uint32_t index = hash % bags->size;
    Bag *bag = &array_at(bags, index);

    while (bag->bag_id != 0) {
        hash = hash_int32(hash);
        index = hash % bags->size;
        bag = &array_at(bags, index);
    }

    assert(bag->bag_id == 0);
    bag->bag_id = pack->bag_id;
    bag->type = (BagType)pack->bag_type;
    bag->model = (BagEnum)pack->bag_model_id;
    array_init(&bag->items);
    array_resize(&bag->items, pack->slot_count);

    assert(pack->bag_model_id < BagEnum_Count);
    world->inventory.bags[pack->bag_model_id] = bag;
}

void RequestSellableItems(GwClient* client, TransactionType type, uint32_t item_type, uint32_t item_max_amount) {

    World* world = get_world_or_abort(client);

    // Buy items received; ask for sellable items

    if (type != TransactionType_TraderSell)
        return;

    QuoteInfo give;
    memset(&give, 0, sizeof(give));
    QuoteInfo recv;
    memset(&recv, 0, sizeof(recv));

    Bag* bag_ptr;
    Bag* bags[] = {
        world->inventory.backpack,
        world->inventory.bag1,
        world->inventory.bag2,
        world->inventory.beltpouch
    };

    for (size_t i = 0; i < sizeof(bags) / sizeof(*bags); i++) {
        bag_ptr = bags[i];
        if (!bag_ptr && bag_ptr->item_count)
            continue;
        Item** item;
        array_foreach(item, &bag_ptr->items) {
            if (!*item) continue;
            if ((*item)->quantity < item_max_amount)
                continue;
            if (item_type == TraderWindowType_Dyes) {
                if ((*item)->type != ItemType_Dye)
                    continue;
            } else if (item_type == TraderWindowType_CommonMaterials) {
                if ((*item)->type != ItemType_Material)
                    continue;
                bool is_common_material = false;
                uint32_t* mod_struct;
                array_foreach(mod_struct, &(*item)->mod_struct) {
                    uint16_t mod_id = ((*mod_struct) & 0xffff0000) >> 16;
                    if (mod_id == 0x2508) {
                        uint8_t mat_slot = (((*mod_struct) & 0x0000ff00) >> 8);
                        is_common_material = mat_slot < 12;
                        break;
                    }
                }
                if (!is_common_material)
                    continue;
            } else if (item_type == TraderWindowType_RareMaterials) {
                if ((*item)->type != ItemType_Material)
                    continue;
                bool is_rare_material = false;
                uint32_t* mod_struct;
                array_foreach(mod_struct, &(*item)->mod_struct) {
                    uint16_t mod_id = ((*mod_struct) & 0xffff0000) >> 16;
                    if (mod_id == 0x2508) {
                        uint8_t mat_slot = (((*mod_struct) & 0x0000ff00) >> 8);
                        is_rare_material = mat_slot > 11 && mat_slot < 38;
                        break;
                    }
                }
                if (!is_rare_material)
                    continue;
            } else if (item_type == TraderWindowType_Runes) {
                if ((*item)->type != ItemType_Rune)
                    continue;
                bool attaches_to_armor = false;
                uint32_t* mod_struct;
                array_foreach(mod_struct, &(*item)->mod_struct) {
                    if (*mod_struct == 0x25b80000) {
                        attaches_to_armor = true;
                        break;
                    }
                }
                if (!attaches_to_armor)
                    continue;
            } else {
                continue; // Other types not supported
            }

            if (give.item_count == sizeof(give.item_ids) / sizeof(*give.item_ids)) {
                world->tmp_merchant_pending_sell_preview++;
                GameSrv_RequestQuote(client, type, &give, &recv, true);
                memset(&give, 0, sizeof(give));
            }
            give.item_ids[give.item_count] = (*item)->item_id;
            give.item_count++;
        }
    }
    if (give.item_count) {
        world->tmp_merchant_pending_sell_preview++;
        GameSrv_RequestQuote(client, type, &give, &recv, true);
    }
}

void FinishMerchantWindowCreation(GwClient* client)
{
    World *world = get_world_or_abort(client);
    world->tmp_merchant_pending_sell_preview = 0;

    LogDebug("FinishMerchantWindowCreation called for %d items", world->tmp_merchant_items.size);
    for (size_t i = 0; i < world->tmp_merchant_items.size; i++) {
        array_add(&world->merchant_items, world->tmp_merchant_items.data[i]);
        if (array_inside(&world->tmp_merchant_prices, i)) {
            Item *item = world->tmp_merchant_items.data[i];
            // GW Bug: last sale price can be LOWER than the default item value!
            if (item->value < world->tmp_merchant_prices.data[i])
                item->value = world->tmp_merchant_prices.data[i];
        }
    }

    Event event;
    Event_Init(&event, EventType_DialogOpen);
    event.DialogOpen.sender_agent_id = world->merchant_agent_id;
    broadcast_event(&client->event_mgr, &event);
}

void HandleWindowItemStreamEnd(Connection* conn, size_t psize, Packet* packet) {
    #pragma pack(push, 1)
        typedef struct {
            Header header;
            uint8_t type;
        } ItemStreamEnd;
    #pragma pack(pop)

    assert(packet->header == GAME_SMSG_WINDOW_ITEM_STREAM_END);
    assert(sizeof(ItemStreamEnd) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    assert(client && client->game_srv.secured);

    ItemStreamEnd* pack = cast(ItemStreamEnd*)packet;

    LogInfo("HandleWindowItemStreamEnd, %d\n", pack->type);

    World* world = get_world_or_abort(client);

    if (pack->type == TransactionType_TraderSell) {
        // Sellable items received
        world->tmp_merchant_pending_sell_preview--;
    }
    if (world->tmp_merchant_pending_sell_preview == 0)
        FinishMerchantWindowCreation(client);
}

void HandleWindowMerchant(Connection* conn, size_t psize, Packet* packet) {
    #pragma pack(push, 1)
        typedef struct {
            Header header;
            uint8_t type; // TransactionType
            uint32_t item_type;
        } WindowMerchant;
    #pragma pack(pop)

    assert(packet->header == GAME_SMSG_WINDOW_MERCHANT);
    assert(sizeof(WindowMerchant) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    WindowMerchant* pack = cast(WindowMerchant*)packet;
    assert(client&& client->game_srv.secured);

    World* world = get_world_or_abort(client);

    if (pack->type == TransactionType_MerchantSell && !pack->item_type) {
        // Special case for merchant; only receives list of buyable items (i.e. no WindowPricesEnd packet; GW client figures out the sell tab)
        FinishMerchantWindowCreation(client);
    }
}

void HandleWindowTrader(Connection* conn, size_t psize, Packet* packet) {
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint8_t type; // TransactionType
        uint32_t item_type;
        uint8_t item_amount;
        uint8_t h000d;
    } WindowTrader;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_WINDOW_TRADER);
    assert(sizeof(WindowTrader) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    WindowTrader* pack = cast(WindowTrader*)packet;
    assert(client && client->game_srv.secured);

    RequestSellableItems(client, cast(TransactionType)pack->type, pack->item_type, pack->item_amount);

    if (pack->type == TransactionType_MerchantSell && !pack->item_type) {
        // Special case for merchant; only receives list of buyable items (i.e. no WindowPricesEnd packet; GW client figures out the sell tab)
        FinishMerchantWindowCreation(client);
    }
}

void HandleWindowOwner(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        AgentId agent_id;
    } WindowOwner;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_WINDOW_OWNER);
    assert(sizeof(WindowOwner) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    WindowOwner *pack = cast(WindowOwner *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    array_clear(&world->merchant_items);
    array_clear(&world->tmp_merchant_items);
    array_clear(&world->tmp_merchant_prices);
    world->merchant_agent_id = pack->agent_id;
    world->interact_with = pack->agent_id;
    Event event;
    Event_Init(&event, EventType_DialogOpen);
    event.DialogOpen.sender_agent_id = pack->agent_id;
    broadcast_event(&client->event_mgr, &event);
}

void HandleWindowAddItems(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint32_t n_items;
        uint32_t items[16];
    } AddItems;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_WINDOW_ADD_ITEMS);
    assert(sizeof(AddItems) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    AddItems *pack = cast(AddItems *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    ArrayItem *items = &world->items;
    ArrayItem *merchant_items = &world->tmp_merchant_items;

    for (size_t i = 0; i < pack->n_items; i++) {
        int32_t item_id = pack->items[i];
        if (!array_inside(items, item_id)) {
            LogError("Expected item '%d' but was not found", item_id);
            continue;
        }

        Item *item = array_at(items, item_id);
        if (!item) {
            LogError("Expected item '%d' but was not found", item_id);
            continue;
        }
        array_add(merchant_items, item);
    }
    printf("AddItems, %d\n", pack->n_items);
}

void HandleWindowAddPrices(Connection* conn, size_t psize, Packet* packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint32_t n_prices;
        int32_t prices[16];
    } AddPrices;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_PRICES);
    assert(sizeof(AddPrices) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    AddPrices* pack = cast(AddPrices*)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    array_uint32_t *merchant_prices = &world->tmp_merchant_prices;
    for (size_t i = 0; i < pack->n_prices; i++) {
        array_add(merchant_prices, pack->prices[i]);
    }
}

void HandleItemPriceQuote(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t item_id;
        int32_t price;
    } PriceQuote;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_PRICE_QUOTE);
    assert(sizeof(PriceQuote) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    PriceQuote *pack = cast(PriceQuote *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);

    ArrayItem *items = &world->items;
    assert(array_inside(items, pack->item_id));
    assert(array_at(items, pack->item_id));
    Item *item = array_at(items, pack->item_id);

    item->quote_price = pack->price;
    Event params;
    Event_Init(&params, EventType_ItemQuotePrice);
    params.ItemQuotePrice.item_id = item->item_id;
    params.ItemQuotePrice.quote_price = item->quote_price;
    broadcast_event(&client->event_mgr, &params);
}

void HandleItemChangeLocation(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int16_t stream_id;
        int32_t item_id;
        int16_t bag_id;
        int8_t  slot;
    } ChangeLocation;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_CHANGE_LOCATION);
    assert(sizeof(ChangeLocation) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    ChangeLocation *pack = cast(ChangeLocation *)packet;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    Item *item = get_item_safe(world, pack->item_id);
    if (!item) {
        LogError("HandleItemChangeLocation: Couldn't find item %d", pack->item_id);
        return;
    }

    BagArray *bags = &world->bags;
    uint32_t hash = hash_int32(pack->bag_id);
    uint32_t index = hash % bags->size;
    Bag *bag = &array_at(bags, index);

    while (bag->bag_id != pack->bag_id) {
        assert(bag->bag_id != 0);
        hash = hash_int32(hash);
        index = hash % bags->size;
        bag = &array_at(bags, index);
    }

    // @Cleanup
    remove_item_from_bag(item);
    item->bag  = bag;
    item->slot = pack->slot;
    bag->items.data[pack->slot] = item;
    bag->item_count++;
}

void GameSrv_UseItem(GwClient *client, Item *item)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int32_t item_id;
    } ItemUse;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    ItemUse packet = NewPacket(GAME_CMSG_ITEM_USE);
    packet.item_id = item->item_id;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_InteractItem(GwClient *client, uint32_t agent_id)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        AgentId agent_id;
        uint8_t unk0;
    } ItemUse;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    ItemUse packet = NewPacket(GAME_CMSG_INTERACT_ITEM);
    packet.agent_id = agent_id;
    packet.unk0 = 0;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_MoveItem(GwClient *client, Item *item, Bag *bag, uint8_t slot)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int32_t item_id;
        int16_t bag_id;
        uint8_t slot;
    } ItemMove;
#pragma pack(pop)

    assert(client && client->game_srv.secured);

    ItemMove packet = NewPacket(GAME_CMSG_ITEM_MOVE);
    packet.item_id = item->item_id;
    packet.bag_id = bag->bag_id;
    packet.slot = slot;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_StartSalvage(GwClient *client, Item *kit, Item *item)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int16_t session_id;
        int32_t kit_item_id;
        int32_t item_id;
    } SalvagePacket;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    SalvageSession *session = &world->salvage_session;

    SalvagePacket packet = NewPacket(GAME_CMSG_ITEM_SALVAGE_SESSION_OPEN);
    packet.session_id = session->salvage_session_id = 21; // @fixme only true for first player in guild hall - client will get disconnected if it's wrong
    packet.kit_item_id = kit->item_id;
    packet.item_id = item->item_id;

    SendPacket(&client->game_srv, sizeof(packet), &packet);

    LogDebug("SendPacket Salvage { session: %u, item: %u, kit: %u }", packet.session_id, packet.item_id, packet.kit_item_id);
}

void GameSrv_CancelSalvage(GwClient *client)
{
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    SalvageSession *session = &world->salvage_session;
    if (!session->is_open) return;

    Packet packet = NewPacket(GAME_CMSG_ITEM_SALVAGE_SESSION_CANCEL);
    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_SalvageDone(GwClient *client)
{
    assert(client && client->game_srv.secured);
    Packet packet = NewPacket(GAME_CMSG_ITEM_SALVAGE_SESSION_DONE);
    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_SalvageMaterials(GwClient *client)
{
    assert(client && client->game_srv.secured);
    Packet packet = NewPacket(GAME_CMSG_ITEM_SALVAGE_MATERIALS);
    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_SalvageUpgrade(GwClient *client, size_t index)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        uint8_t upgrade_index;
    } SalvagePacket;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    SalvageSession *session = &world->salvage_session;
    if (index >= session->n_upgrades) {
        LogError("The maximum upgrade index is %u, but you entered %u", session->n_upgrades, index);
        return;
    }

    SalvagePacket packet = NewPacket(GAME_CMSG_ITEM_SALVAGE_UPGRADE);
    packet.upgrade_index = cast(uint8_t)index;
    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void HandleSalvageSessionStart(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        uint16_t salvage_session_id;
        uint32_t item_id; // representing the trade
        uint32_t n_upgrades;
        uint32_t upgrades[3];
    } SalvagePacket;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_SALVAGE_SESSION_START);
    assert(sizeof(SalvagePacket) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    SalvagePacket *pack = cast(SalvagePacket *)packet;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    SalvageSession *session = &world->salvage_session;

    if (pack->salvage_session_id != session->salvage_session_id) {
        LogError("salvage_session_id mismatch {received: %d, expected: %d}",
            pack->salvage_session_id, session->salvage_session_id);
        return;
    }

    session->n_upgrades = pack->n_upgrades;
    for (size_t i = 0; i < session->n_upgrades; i++) {
        uint32_t item_id = pack->upgrades[i];
        session->upgrades[i] = get_item_safe(world, item_id);
    }
    session->is_open = true;

    Event event;
    Event_Init(&event, EventType_SalvageSessionStart);
    event.SalvageSessionStart.item_id = pack->item_id;

    // @Enhancement: pass possible salvage options
    broadcast_event(&client->event_mgr, &event);
}

void HandleSalvageSessionCancel(Connection *conn, size_t psize, Packet *packet)
{
    assert(packet->header == GAME_SMSG_ITEM_SALVAGE_SESSION_CANCEL);
    assert(sizeof(Packet) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);

    World *world = get_world_or_abort(client);
    SalvageSession *session = &world->salvage_session;
    session->is_open = false;
    session->n_upgrades = 0;
}

void HandleSalvageSessionDone(Connection* conn, size_t psize, Packet* packet)
{
    assert(packet->header == GAME_SMSG_ITEM_SALVAGE_SESSION_DONE);
    assert(sizeof(Packet) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    assert(client && client->game_srv.secured);
    World *world = get_world_or_abort(client);
    SalvageSession* session = &world->salvage_session;

    session->is_open = false;
    session->n_upgrades = 0;
}

void HandleSalvageSessionSuccess(Connection* conn, size_t psize, Packet* packet)
{
    assert(packet->header == GAME_SMSG_ITEM_SALVAGE_SESSION_SUCCESS);
    assert(sizeof(Packet) == psize);

    GwClient* client = cast(GwClient*)conn->data;
    assert(client && client->game_srv.secured);

    GameSrv_SalvageDone(client);
}

void HandleSalvageSessionItemKept(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        uint8_t item_kept;
    } SalvagePacket;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_ITEM_SALVAGE_SESSION_ITEM_KEPT);
    assert(sizeof(SalvagePacket) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}

void GameSrv_UnequipItem(GwClient *client, EquipedItemSlot equip_slot, Bag *bag, uint8_t slot)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        uint8_t equip_slot;
        int16_t bag_id;
        uint8_t bag_slot;
    } UnequipItemPacket;
#pragma pack(pop)

    assert(client && client->game_srv.secured);

    UnequipItemPacket packet = NewPacket(GAME_CMSG_UNEQUIP_ITEM);
    packet.equip_slot = equip_slot;
    packet.bag_id = bag->bag_id;
    packet.bag_slot = slot;

    SendPacket(&client->game_srv, sizeof(packet), &packet);    
}

void GameSrv_Identify(GwClient* client, Item* kit, Item* item)
{
#pragma pack(push, 1)
    typedef struct {
        Header  header;
        int32_t kit_item_id;
        int32_t item_id;
    } IdentifyPacket;
#pragma pack(pop)

    assert(client && client->game_srv.secured);
    IdentifyPacket packet = NewPacket(GAME_CMSG_ITEM_IDENTIFY);
    packet.kit_item_id = kit->item_id;
    packet.item_id = item->item_id;

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}
