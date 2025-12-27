#ifdef CORE_FILESRV_C
#error "filesrv.c included more than once"
#endif
#define CORE_FILESRV_C

#pragma pack(push, 1)
typedef struct _FILE_CMSG_CONNECT {
    uint8_t  h0000; // 1
    uint32_t h0001; // 0
} FILE_CMSG_CONNECT;

typedef struct _FILE_CMSG_REQUEST_GAME {
    uint16_t header; // 0xF1;
    uint16_t length; // 0x10
    uint32_t game;  // (1 for Guild Wars)
    uint32_t h0008; // 0
    uint32_t h000C; // 0
} FILE_CMSG_REQUEST_GAME; // size 21 (0x15)

typedef struct _FILE_SMSG_GAME_IDS {
    uint16_t header;
    uint16_t length;
    uint32_t manifest_id;
    uint32_t backup_exe_id;
    uint32_t h000C;
    uint32_t h0010;
    uint32_t h0014;
    uint32_t h0018;
    uint32_t latest_exe_id;
} FILE_SMSG_GAME_IDS;
#pragma pack(pop)

int FsConnect(struct socket *result, struct sockaddr *sa)
{
    struct socket sock;
    sock.handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock.handle == INVALID_SOCKET) {
        log_error("Failed to create a socket, err: %d", os_errno);
        return 1;
    }
    if (connect(sock.handle, sa, sizeof(*sa)) == SOCKET_ERROR) {
        log_error("Failed to connect ot the file server, err: %d", os_errno);
        closesocket(sock.handle);
        return 1;
    }

    FILE_CMSG_CONNECT msg_connect = {
        .h0000 = 1,
        .h0001 = 0,
    };

    FILE_CMSG_REQUEST_GAME msg_game = {
        .header = 0xF1,
        .length = sizeof(msg_game),
        .game = 1,
        .h0008 = 0,
        .h000C = 0,
    };

    char buffer[sizeof(msg_connect) + sizeof(msg_game)];
    memcpy(buffer, &msg_connect, sizeof(msg_connect));
    memcpy(buffer + sizeof(msg_connect), &msg_game, sizeof(msg_game));

    int err;
    if ((err = send(sock.handle, buffer, sizeof(buffer), 0)) != sizeof(buffer)) {
        if (err == SOCKET_ERROR) {
            log_error("Failed to send, err: %d", os_errno);
        } else {
            log_error("Failed to send all bytes (sent: %d, expected: %zu), err: %d", err, sizeof(buffer), os_errno);
        }
        closesocket(sock.handle);
        return 1;
    }

    *result = sock;
    return 0;
}

int FsRecvExact(struct socket sock, void *buffer, size_t size)
{
    int err;
    int max_recv = (int) min_size_t(INT_MAX, size);
    if ((err = recv(sock.handle, buffer, max_recv, 0)) < 0) {
        log_error("recv failed, err: %d", os_errno);
        return 1;
    }
    if (err == 0) {
        log_warn("Server disconnected");
        return 1;
    }
    if ((size_t) err != size) {
        log_warn("Expected %zu bytes, got %d", size, err);
        return 1;
    }
    return 0;
}

int FsGetLatestExeFileId(uint32_t *result)
{
    int err;
    char hostname[64];

    SockAddressArray ips = {0};
    const size_t max_idx = 11;

    size_t idx;
    for (idx = 1; idx < max_idx; ++idx) {
        int ret = snprintf(hostname, sizeof(hostname), "File%zu.ArenaNetworks.com", idx);
        if (ret < 0 || sizeof(hostname) <= ret) {
            log_error("Couldn't create the DNS name for the file server");
            continue;
        }

        ips = IPv4ToAddrEx(hostname, "6112");

        size_t jdx;
        for (jdx = 0; jdx < ips.size; ++jdx) {
            struct sockaddr sa = ips.data[jdx];

            struct socket sock;
            if ((err = FsConnect(&sock, &sa)) != 0)
                continue;

            FILE_SMSG_GAME_IDS game_ids;
            err = FsRecvExact(sock, &game_ids, sizeof(game_ids));
            closesocket(sock.handle);

            if (err != 0) {
                log_error("FILE_SMSG_GAME_IDS failed");
                continue;
            }

            *result = game_ids.latest_exe_id;
            break;
        }

        if (jdx != ips.size) {
            break;
        }

        array_clear(&ips);
    }

    array_reset(&ips);

    if (idx == max_idx) {
        log_error("Failed to get the latest file id");
        return 1;
    }

    return 0;
}
