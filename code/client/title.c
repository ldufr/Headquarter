#ifdef CORE_TITLE_C
#error "title.c included more than once"
#endif
#define CORE_TITLE_C

void HandleTitleRankData(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header   header;
        int32_t  rank_id;
        int32_t  unk0;
        int32_t  rank;
        uint16_t name[8];
    } RankData;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_TITLE_RANK_DATA);
    assert(sizeof(RankData) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    RankData *pack = cast(RankData *)packet;
    assert(client && client->game_srv.secured);
    (void)pack;
}

void HandleTitleRankDisplay(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int16_t player_id;
        int32_t rank_id;
    } RankDisplay;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_TITLE_RANK_DISPLAY);
    assert(sizeof(RankDisplay) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    RankDisplay *pack = cast(RankDisplay *)packet;
    assert(client && client->game_srv.secured);
    (void)pack;
}

void HandleTitleTrackInfo(Connection *conn, size_t psize, Packet *packet)
{
#pragma pack(push, 1)
    typedef struct {
        Header header;
        int32_t track_id;
        int32_t group_id;
        int32_t current_points;
        int32_t current_rank;
        int32_t points_needed_to_reach_current_rank;
        int32_t title_mode_flags; // see TitleModeFlags
        int32_t next_title_rank_id;
        int32_t points_needed_to_reach_next_rank;
        int32_t max_rank;
        int32_t max_rank_id;
        uint16_t point_name[8];
        uint16_t description[8];
    } TrackInfo;
#pragma pack(pop)

    assert(packet->header == GAME_SMSG_TITLE_TRACK_INFO);
    assert(sizeof(TrackInfo) == psize);

    GwClient *client = cast(GwClient *)conn->data;
    assert(client && client->game_srv.secured);
}
