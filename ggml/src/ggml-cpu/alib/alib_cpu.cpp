#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dlfcn.h>
#include <libgen.h>
#include <sys/time.h>
#include <time.h>

#include "asock.h"
#include "alib.h"

static bool is_startup = false;
static int inner_asock = -1;

static int64_t random_number()
{
    FILE *file = fopen("/proc/sys/kernel/random/uuid", "r");
    char uuid[37];
    char hex_str[16] = {0};
    int idx = 0;
    int64_t result = 0;

    if (!file) {
        return -1;
    }

    if (fgets(uuid, sizeof(uuid), file) == NULL) {
        fclose(file);
        return -1;
    }
    fclose(file);

    uuid[strcspn(uuid, "\n")] = '\0';

    for (int i = 0; uuid[i] != '\0' && idx < (int)sizeof(hex_str); i++) {
        if ((uuid[i] >= '0' && uuid[i] <= '9') ||
                (uuid[i] >= 'a' && uuid[i] <= 'f') ||
                (uuid[i] >= 'A' && uuid[i] <= 'F')) {
            hex_str[idx++] = uuid[i];
        }
    }

    for (int i = 0; i < (int)sizeof(hex_str); i++) {
        result = result * 16 + (hex_str[i] >= '0' &&
                hex_str[i] <= '9' ? hex_str[i] - '0' : (hex_str[i] >= 'a' &&
                    hex_str[i] <= 'f' ? hex_str[i] - 'a' + 10 : hex_str[i] - 'A' + 10));
    }

    return result;
}

static void af_set_asock(int asock)
{
    inner_asock = asock;
}

static int af_get_asock(int *asock)
{
    if (inner_asock > 0) {
        *asock = inner_asock;
        return 0;
    }

    return -1;
}

static bool af_valid_asock()
{
    if (inner_asock > 0) {
        return true;
    }
    return false;
}

static int af_seek_hub(char *hub, size_t size, int *port, int timeout)
{
    int ret;
    unsigned short sport;
    int asock;
    cluster peers;
    nub *b;

    if (!is_startup) {
        is_startup = true;
        Startup();
    }

    sport = GetServicePort();

    printf(">>>>>>>>>>> %s sport: %d \n", __func__, sport);

    asock = Attach(AT_COMMAND, NULL, sport, timeout);
    if (asock < 1) {
        return -1;
    }

    ret = GetPeers(asock, &peers);
    if (ret < 0) {
        Detach(asock);
        return -1;
    }

    Detach(asock);

    b = &peers.nubs[random_number() % peers.count];

    strncpy(hub, b->ip, size);
    *port = b->port;

    return 0;
}

static int af__load_library()
{
    int ret;
    char hub_addr[32] = {};
    int hub_port = 0;
    int asock;
    int timeout = 120 * 1000;
    char path[PATH_MAX] = "/home/github/llama.cpp/build/bin/libggml-cpu.so";

    if (af_valid_asock()) {
       return 0;
    }

    ret = af_seek_hub(hub_addr, sizeof(hub_addr), &hub_port, timeout);
    if (ret < 0) {
        fprintf(stderr, ">>> af_seek_hub error ... : %d\n", ret);
        return -1;
    }

    asock = Attach(AT_LIBRARY, hub_addr, hub_port, timeout);
    if (asock < 0) {
        fprintf(stderr, ">>> Attach error ... : %d\n", asock);
        return -1;
    }


    ret = LoadLibrary(asock, path);
    if (ret < 0) {
        fprintf(stderr, ">>> LoadLibraryd error ... : %d\n", ret);
        return -1;
    }

    af_set_asock(asock);

    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif


void __init__alib_cpu(void) __attribute__((constructor));

void __init__alib_cpu(void)
{
    af__load_library();
}

void wrap__ggml_vec_dot_q4_K_q8_K(int n, float * s, size_t bs, const void *vx, size_t bx, const void *vy, size_t by, int nrc)
{

}


#ifdef __cplusplus
}
#endif
