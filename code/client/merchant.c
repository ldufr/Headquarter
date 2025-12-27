#ifdef CORE_MERCHANT_C
#error "merchant.c included more than once"
#endif
#define CORE_MERCHANT_C

void GameSrv_TransactItems(GwClient *client, TransactionType type,
    int gold_send, TransactionInfo *send_info,
    int gold_recv, TransactionInfo *recv_info)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int8_t type;
        int32_t gold_send;
        uint32_t n_item_ids_send;
        int32_t item_ids_send[16];
        uint32_t n_item_quant_send;
        uint8_t item_quant_send[16];
        int32_t gold_recv;
        uint32_t n_item_ids_recv;
        int32_t item_ids_recv[16];
        uint32_t n_item_quant_recv;
        uint8_t item_quant_recv[16];
    } TransactItem;
#pragma pack(pop)

    TransactItem packet = NewPacket(GAME_CMSG_TRANSACT_ITEMS);
    packet.type = type;

    packet.gold_send = gold_send;
    assert(0 <= send_info->item_count && send_info->item_count <= 16);

    packet.n_item_ids_send = (uint32_t)send_info->item_count;
    packet.n_item_quant_send = (uint32_t)send_info->item_count;
    for (size_t i = 0; i < send_info->item_count; i++) {
        packet.item_ids_send[i] = send_info->item_ids[i];
        packet.item_quant_send[i] = (int8_t)send_info->item_quants[i];
    }

    packet.gold_recv = gold_recv;
    assert(0 <= recv_info->item_count && recv_info->item_count <= 16);

    packet.n_item_ids_recv = (uint32_t)recv_info->item_count;
    packet.n_item_quant_recv = (uint32_t)recv_info->item_count;
    for (size_t i = 0; i < recv_info->item_count; i++) {
        packet.item_ids_recv[i] = recv_info->item_ids[i];
        packet.item_quant_recv[i] = (int8_t)recv_info->item_quants[i];
    }

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}


void GameSrv_BuyMaterials(GwClient *client, TransactionType type,
    int gold_send, TransactionInfo *send_info,
    int gold_recv, TransactionInfo *recv_info)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int8_t type;
        int32_t gold_send;
        uint32_t n_item_ids_send;
        int32_t item_ids_send[16];
        int32_t gold_recv;
        uint32_t n_item_ids_recv;
        int32_t item_ids_recv[16];
    } TransactItem;
#pragma pack(pop)

    TransactItem packet = NewPacket(GAME_CMSG_BUY_MATERIALS);
    packet.type = type;

    packet.gold_send = gold_send;
    assert(0 <= send_info->item_count && send_info->item_count <= 16);

    packet.n_item_ids_send = (uint32_t)send_info->item_count;
    for (size_t i = 0; i < send_info->item_count; i++) {
        packet.item_ids_send[i] = send_info->item_ids[i];
    }

    packet.gold_recv = gold_recv;
    assert(0 <= recv_info->item_count && recv_info->item_count <= 16);
    packet.n_item_ids_recv = (uint32_t)recv_info->item_count;
    for (size_t i = 0; i < recv_info->item_count; i++) {
        packet.item_ids_recv[i] = recv_info->item_ids[i];
    }

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}

void GameSrv_RequestQuote(GwClient *client, TransactionType type,
    QuoteInfo *send_info, QuoteInfo *recv_info, bool preview)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int8_t type; // TransactionType
        int8_t preview_quote; // 1 for "last paid", 0 for "quote"
        int32_t gold_send;
        uint32_t n_item_ids_send;
        int32_t item_ids_send[16];
        int32_t gold_recv;
        uint32_t n_item_ids_recv;
        int32_t item_ids_recv[16];
    } ReqQuote;
#pragma pack(pop)

    ReqQuote packet = NewPacket(GAME_CMSG_REQUEST_QUOTE);
    packet.type = type;

    packet.preview_quote = preview ? 1 : 0;

    packet.gold_send = 0;
    packet.n_item_ids_send = (uint32_t)send_info->item_count;
    for (size_t i = 0; i < send_info->item_count; i++)
        packet.item_ids_send[i] = send_info->item_ids[i];

    packet.gold_recv = 0;
    packet.n_item_ids_recv = (uint32_t)recv_info->item_count;
    for (size_t i = 0; i < recv_info->item_count; i++)
        packet.item_ids_recv[i] = recv_info->item_ids[i];

    SendPacket(&client->game_srv, sizeof(packet), &packet);
}
