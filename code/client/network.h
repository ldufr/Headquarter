#ifdef CORE_NETWORK_H
#error "network.h included more than once"
#endif
#define CORE_NETWORK_H

#if _BYTE_ORDER != _LITTLE_ENDIAN
# error "We currently don't support big endian machine"
#endif

typedef struct Connection Connection;

typedef enum ConnectionType {
    ConnectionType_None     = 0,
    ConnectionType_Auth     = 1,
    ConnectionType_Game     = 2,
    ConnectionType_Portal   = 3,

    ConnectionType_Count
} ConnectionType;

typedef enum ConnectionFlags {
    NETCONN_PAUSE       = 1 << 0,
    NETCONN_SHUTDOWN    = 1 << 1,
    NETCONN_REMOVE      = 1 << 2,
    NETCONN_RECORD      = 1 << 3,
    NETCONN_RECONNECT   = 1 << 4,
} ConnectionFlags;

typedef array(uint8_t)          ByteBuffer;
typedef array(MsgFormat)        MsgFormatArray;
typedef array(Connection *)     ConnectionArray;
typedef array(struct sockaddr)  SockAddressArray;

typedef uint16_t Header;
typedef struct Packet {
    Header header;
} Packet;

typedef void (*MsgHandler)(Connection *conn, size_t size, Packet *pack);
typedef array(MsgHandler) MsgHandlerArray;

#define NewPacket(opcode, ...) {(opcode)}

typedef struct Connection {
    void                   *data;

    char                   *name;
    uint32_t                flags;
    int                     proto;

    msec_t                  t0; // Time when socket is opened.
    struct socket           fd; // Os file descriptor / handle.
    struct sockaddr         host;

    msec_t                  ping;
    msec_t                  pong;
    msec_t                  latency;

    thread_mutex_t          mutex;
    ByteBuffer              in;
    ByteBuffer              out;

    bool                    secured;
    mbedtls_arc4_context    decrypt;
    mbedtls_arc4_context    encrypt;

    MsgFormatArray          server_msg_format;
    MsgFormatArray          client_msg_format;
    MsgHandlerArray         handlers;

    msec_t                  last_tick_time;
} Connection;

void init_connection(Connection *conn, void *data);

/*
 * Return if a ip address (ipv4) was found and copy it in sockaddr.
 */
bool IPv4ToAddr(const char *host, const char *port, struct sockaddr *sockaddr);

/*
 * Return all ip addresses (ipv4) from a host and a port.
 */
SockAddressArray IPv4ToAddrEx(const char *host, const char *port);

/*
 * Initialize Winsock, mbedtls and find dns.
 */
void Network_Init(void);

/*
 * Release ressources acquired in `Network_Init`.
 */
void Network_Shutdown(void);

/*
 * Free all connection ressources.
 */
void NetConn_Reset(Connection *conn);

/*
 * Flags the socket so it get removed and shutdown the socket.
 * Note that the data that were already received can be readed.
 */
void NetConn_Shutdown(Connection *conn);

/*
 * Return true if the network is ready to be used for an other connection.
 */
bool NetConn_IsShutdown(Connection *conn);

/*
 * Encryot and write the out buffer to the socket.
 */
void NetConn_Send(Connection *conn);

/*
 * Read, decrypt and dispatch the packet from the socket.
 */
void NetConn_Recv(Connection *conn);

/*
 * Deserialize and dispatch packets.
 */
void NetConn_DispatchPackets(Connection *conn);

/*
 * Send and receive the new packets
 */
void NetConn_Update(Connection *conn);

/*
 * Pack & write a packet in the out buffer.
 * If the buffer is full, it write the data to the socket.
 */
void SendPacket(Connection *conn, size_t size, void *p);

/*
 * Make the handshake and return true if succeed.
 */
bool AuthSrv_Connect(Connection *conn);

bool GameSrv_Connect(Connection *conn,
    const struct uuid *account, const struct uuid *character,
    uint32_t world_hash, uint32_t player_hash, uint32_t map);

/*
 * Return the number of bytes written.
 */
size_t NetMsg_Pack(Packet *packet, size_t pack_size,
    uint8_t *buffer, size_t buff_size, MsgFormat *format);

/*
 * Return the number of bytes readed.
 */
size_t NetMsg_Unpack(const uint8_t *data, size_t data_size,
    Packet *packet, size_t pack_size, MsgFormat *format);

void Sha1(const void *data, size_t size, char digest[20]);
