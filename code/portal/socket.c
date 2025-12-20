#ifdef PORTAL_SOCKET_C
#error "socket.c included more than once"
#endif
#define PORTAL_SOCKET_C

int send_full(SOCKET fd, const uint8_t *buffer, size_t buffer_len)
{
    int ret;
    size_t written = 0;

    while (written < buffer_len) {
        const char *p = (const char *)buffer + written;
        int len = (int)min_size_t(INT_MAX, buffer_len - written);
        if ((ret = send(fd, p, len, 0)) <= 0)
            return ret;
        written += (size_t)ret;
    }

    return 0;
}

int recv_to_buffer(SOCKET fd, array_uint8_t *buffer)
{
    int ret;

    const size_t BUFFER_SIZE = 1024;
    uint8_t *ptr = array_push(buffer, BUFFER_SIZE);

    if ((ret = recv(fd, (char *)ptr, (int)BUFFER_SIZE, 0)) <= 0) {
        size_t size = array_size(buffer) - BUFFER_SIZE;
        buffer->size = size;
        // array_resize(buffer, size);
        return ret;
    }

    buffer->size = array_size(buffer) - (BUFFER_SIZE - (size_t)ret);
    // array_resize(buffer, size);
    return 0;
}

int resolve_dns(array_sockaddr_t *addresses, int af, const char *hostname, uint16_t port)
{
    int ret;
    struct addrinfo hints = {0};
    struct addrinfo *results = NULL;

    char port_as_str[sizeof("65535")];
    snprintf(port_as_str, sizeof(port_as_str), "%" PRIu16, port);

    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((ret = getaddrinfo(hostname, port_as_str, &hints, &results)) != 0) {
        fprintf(stderr, "Failed to resolved host '%s'\n", hostname);
        return 1;
    }

    for (struct addrinfo *it = results; it != NULL; it = it->ai_next) {
        array_add(addresses, *it->ai_addr);
    }

    freeaddrinfo(results);
    return 0;
}

int resolve_dns4(array_sockaddr_t *addresses, const char *hostname, uint16_t port)
{
    return resolve_dns(addresses, AF_INET, hostname, port);
}

int get_sockname(SOCKET fd, char *buffer, size_t buffer_len)
{
    const size_t MIN_BUF_LEN = MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN);
    if (buffer_len < MIN_BUF_LEN) {
        fprintf(stderr, "The minimum buffer length is %zu\n", MIN_BUF_LEN);
        return 1;
    }

    struct sockaddr_storage sa;
    socklen_t len = sizeof(sa);
    int ret = getsockname(fd, (struct sockaddr *)&sa, &len);
    if (ret != 0) {
        fprintf(stderr, "'getsockname' failed\n");
        return 1;
    }

    void *addr = NULL;
    switch (sa.ss_family)
    {
        case AF_INET:
            addr = &((struct sockaddr_in *)&sa)->sin_addr;
            break;
        case AF_INET6:
            addr = &((struct sockaddr_in6 *)&sa)->sin6_addr;
            break;
        default:
            return 1;
    }

    if (inet_ntop(sa.ss_family, addr, buffer, buffer_len) == NULL) {
        fprintf(stdout, "Couldn't parse ip address\n");
        return 1;
    }

    return 0;
}
