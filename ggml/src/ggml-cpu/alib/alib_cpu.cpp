#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <dlfcn.h>
#include <libgen.h>
#include <sys/time.h>
#include <time.h>

#define GGML_COMMON_DECL_C
#include "ggml-common.h"
#include "ggml-impl.h"
#include "ggml-cpu-quants.h"

#include "asock.h"
#include "alib.h"

#define AF_FN(fn) "af" #fn

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
    short sport;
    int asock;
    cluster peers;
    nub *b;
    static bool is_startup = false;

    if (!is_startup) {
        ret = Startup();
        if (ret < 0) {
            return -1;
        }
        is_startup = true;
    }

    sport = GetServicePort();
    if (sport < 0) {
        return -1;
    }

    asock = Attach(AT_COMMAND, NULL, sport, timeout);
    if (asock < 0) {
        return -1;
    }

    ret = GetPeers(asock, &peers);
    if (ret < 0) {
        Detach(asock);
        return -1;
    }

    Detach(asock);

    b = &peers.nubs[random_number() % peers.count];

    strncpy(hub, b->ip, (strlen(b->ip) < size) ? (strlen(b->ip)) : size);
    *port = b->port;

    return 0;
}

static void af_search_library(const char *filename, char *full)
{
    size_t i;
    size_t len;
    char path[PATH_MAX] = {};
    const char *search_dir[] = {
        "/lib/",
        "/lib64/",
        "/usr/lib/",
        "/usr/lib64/",
        "/usr/local/lib/",
        "/usr/local/lib64/",
    };

    len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len > 0) {
        (void) dirname(path);
        strcat(path, "/");
        strcat(path, filename);
    } else {
        path[0] = '\0';
    }

    if (strlen(path) == 0 || access(path, F_OK) != 0) {
        for (i = 0; i < sizeof(search_dir) / sizeof(search_dir[0]); i++) {
            memset(path, 0, sizeof(path));
            strcpy(path, search_dir[i]);
            strcat(path, filename);
            if (access(path, F_OK) == 0) {
                break;
            }
        }
    }
    if (strlen(path) > 0 && access(path, F_OK) == 0) {
        (void) realpath(path, full);
    }
}


static int af__load_library()
{
    int ret;
    char hub_addr[64] = {};
    int hub_port = 0;
    int asock;
    int timeout = 120 * 1000;
    int i;
    char path[PATH_MAX] = {};

    if (af_valid_asock()) {
       return 0;
    }

    af_search_library("libggml-cpu.so", path);

    for (i = 0; i < 100; i++) {
        ret = af_seek_hub(hub_addr, sizeof(hub_addr), &hub_port, timeout);
        if (ret < 0) {
            continue;
        }
        break;
    }

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

    fprintf(stderr, ">>> load library ok ... : %d\n", asock);

    return 0;
}

static int af__release_library()
{
    int asock = -1;

    if (af_valid_asock()) {
        if (af_get_asock(&asock) == 0) {
            (void) ReleaseLibrary(asock);
            (void) Detach(asock);
        }
    }

    return 0;
}


int af__call(const char *fn, void *in, int inlen, void **out, int *outlen)
{
    int asock;
    if (af_get_asock(&asock)) {
        return -1;
    }
    return AFCall(asock, fn, in, inlen, out, outlen);
}

#ifdef __cplusplus
extern "C" {
#endif


void __init__alib_cpu(void) __attribute__((constructor));
void __exit__alib_cpu(void) __attribute__((destructor));

void __init__alib_cpu(void)
{
    af__load_library();
    for (int i = 0; i < (1 << 16); ++i) {
        union {
            uint16_t u16;
            ggml_fp16_t fp16;
        } u = {i};
        ggml_table_f32_f16[i] = GGML_COMPUTE_FP16_TO_FP32(u.fp16);
    }
}

/**
 *  This code will be executed after main exits, maybe not if you end the
 *  program with Ctrl+C
 */
void __exit__alib_cpu(void)
{
    af__release_library();
}


uint8_t *serialize_ggml_vec_dot_q4_K_q8_K(int n, size_t bs,
                                            const block_q4_K *vx, size_t bx,
                                            const block_q8_K *vy, size_t by,
                                            int nrc,
                                            size_t *out_size)
{
    int qk = QK_K;
    int nb = n / qk;


    size_t total_size = sizeof(int) + sizeof(size_t) * 3 + sizeof(int) + nb * (sizeof(block_q4_K) + sizeof(block_q8_K));
    uint8_t *data = (uint8_t *)malloc(total_size);


    uint8_t *ptr = data;

    // Serialize n
    memcpy(ptr, &n, sizeof(int));
    ptr += sizeof(int);

    // Serialize bs
    memcpy(ptr, &bs, sizeof(size_t));
    ptr += sizeof(size_t);


    // Serialize bx
    memcpy(ptr, &bx, sizeof(size_t));
    ptr += sizeof(size_t);

    // Serialize by
    memcpy(ptr, &by, sizeof(size_t));
    ptr += sizeof(size_t);

    // Serialize nrc
    memcpy(ptr, &nrc, sizeof(int));
    ptr += sizeof(int);

    // Serialize vx (nb blocks of block_q4_K)
    for (int i = 0; i < nb; i++) {
        memcpy(ptr, &vx[i], sizeof(block_q4_K));
        ptr += sizeof(block_q4_K);
    }

    // Serialize vy (nb blocks of block_q8_K)
    for (int i = 0; i < nb; i++) {
        memcpy(ptr, &vy[i], sizeof(block_q8_K));
        ptr += sizeof(block_q8_K);
    }

    *out_size = total_size;
    return data;
}

void deserialize_ggml_vec_dot_q4_K_q8_K(uint8_t *data, int *n, size_t *bs,
                                            block_q4_K **vx, size_t *bx,
                                            block_q8_K **vy, size_t *by,
                                            int *nrc)
{
    uint8_t *ptr = data;
    int qk = QK_K;
    int nb = 0;


    // Deserialize n
    memcpy(n, ptr, sizeof(int));
    ptr += sizeof(int);

    nb = *n / qk;


    // Deserialize bs
    memcpy(bs, ptr, sizeof(size_t));
    ptr += sizeof(size_t);

    // Deserialize bx
    memcpy(bx, ptr, sizeof(size_t));
    ptr += sizeof(size_t);

    // Deserialize by
    memcpy(by, ptr, sizeof(size_t));
    ptr += sizeof(size_t);

    // Deserialize nrc
    memcpy(nrc, ptr, sizeof(int));
    ptr += sizeof(int);


    // Deserialize vx (nb blocks of block_q4_K)
    *vx = (block_q4_K *)malloc(nb * sizeof(block_q4_K));
    for (int i = 0; i < nb; i++) {
        memcpy(&(*vx)[i], ptr, sizeof(block_q4_K));
        ptr += sizeof(block_q4_K);
    }


    // Deserialize vy (nb blocks of block_q8_K)
    *vy = (block_q8_K *)malloc(nb * sizeof(block_q8_K));
    for (int i = 0; i < nb; i++) {
        memcpy(&(*vy)[i], ptr, sizeof(block_q8_K));
        ptr += sizeof(block_q8_K);
    }
}

int afwrap__ggml_vec_dot_q4_K_q8_K(void *in, int inlen, void **out, int *outlen)
{
    int n;
    int qk = QK_K;
    size_t bs;
    size_t bx;
    size_t by;
    int nrc;
    block_q4_K *vx = NULL;
    block_q8_K *vy = NULL;
    float *s = NULL;

    deserialize_ggml_vec_dot_q4_K_q8_K((uint8_t *)in, &n, &bs, &vx, &bx, &vy, &by, &nrc);


    s = (float *)calloc(1, sizeof(float) * n / qk);

    ex__ggml_vec_dot_q4_K_q8_K(n, s, bs, vx, bx, vy, by, nrc);

    if (vx) {
        free(vx);
    }
    if (vy) {
        free(vy);
    }

    *out = s;
    *outlen = sizeof(float) * n / qk;

    return 0;
}

void wrap__ggml_vec_dot_q4_K_q8_K(int n, float * s, size_t bs, const void *vx, size_t bx, const void *vy, size_t by, int nrc)
{
    void *in = NULL;
    size_t inlen = 0;

    void *out = NULL;
    size_t outlen = 0;

    in = serialize_ggml_vec_dot_q4_K_q8_K(n, bs, (block_q4_K *)vx, bx,
            (block_q8_K *)vy, by,
            nrc, &inlen);

    af__call(AF_FN(wrap__ggml_vec_dot_q4_K_q8_K), in, inlen, &out, (int *)&outlen);

    if (out && outlen > 0) {
        memcpy(s, out, outlen);
    }

    if (out) {
        free(out);
    }

    if (in) {
        free(in);
    }

}


#ifdef __cplusplus
}
#endif
