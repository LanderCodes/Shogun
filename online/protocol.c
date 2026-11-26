#include "protocol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_spaces(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    return p;
}

static int take_token(const char **cursor, char *out, size_t size)
{
    const char *p = skip_spaces(*cursor);
    size_t n = 0;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        if (out && n + 1 < size) out[n++] = *p;
        ++p;
    }
    if (out) out[n] = '\0';
    *cursor = p;
    return n > 0;
}

static void copy_rest(const char *p, char *out, size_t size)
{
    p = skip_spaces(p);
    size_t n = strlen(p);
    while (n > 0 && (p[n - 1] == '\r' || p[n - 1] == '\n' || p[n - 1] == ' ')) --n;
    if (!out || size == 0) return;
    if (n >= size) n = size - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

void online_sanitize_name(const char *input, char *output, size_t size)
{
    if (!output || size == 0) return;
    size_t n = 0;
    if (input) {
        while (*input == ' ') ++input;
        for (const unsigned char *p = (const unsigned char *)input; *p; ++p) {
            if (n + 1 >= size) break;
            unsigned char c = *p;
            if (isalnum(c) || c == ' ' || c == '_' || c == '-') {
                output[n++] = (char)toupper(c);
            }
        }
    }
    while (n > 0 && output[n - 1] == ' ') --n;
    output[n] = '\0';
}

int online_word(const char *line, const char *word)
{
    if (!line || !word) return 0;
    size_t n = strlen(word);
    if (strncmp(line, word, n) != 0) return 0;
    return line[n] == '\0' || line[n] == ' ' || line[n] == '\r' ||
           line[n] == '\n';
}

int online_parse_int(const char *text, int *value)
{
    if (!text || !value || !*text) return 0;
    char *end = NULL;
    long n = strtol(text, &end, 10);
    if (end == text || n < 0 || n > 0x7fffffffL) return 0;
    *value = (int)n;
    return 1;
}

int online_parse_start(const char *line, int *match_id, uint32_t *seed,
                       int *side)
{
    if (!line || !match_id || !seed || !side) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, "START")) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, match_id)) return 0;
    if (!take_token(&p, token, sizeof(token))) return 0;
    char *end = NULL;
    unsigned long value = strtoul(token, &end, 10);
    if (end == token || value > 0xffffffffUL) return 0;
    *seed = (uint32_t)value;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, side) || (*side != 0 && *side != 1)) return 0;
    return 1;
}

int online_parse_challenge(const char *line, int *match_id, char *name,
                           size_t size)
{
    if (!line || !match_id) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, "CHALLENGE")) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, match_id)) return 0;
    copy_rest(p, name, size);
    return name == NULL || name[0] != '\0';
}

int online_parse_cancelled(const char *line, int *match_id, char *reason,
                           size_t size)
{
    if (!line || !match_id) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, "CANCELLED")) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, match_id)) return 0;
    copy_rest(p, reason, size);
    return 1;
}

int online_parse_move(const char *line, int *match_id, int *from, int *to)
{
    if (!line || !match_id || !from || !to) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, "MOVE")) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, match_id)) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, from) || *from < 0 ||
        *from >= BOARD_SIZE * BOARD_SIZE) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, to) || *to < 0 ||
        *to >= BOARD_SIZE * BOARD_SIZE) return 0;
    return 1;
}

static int is_hex_hash(const char *text)
{
    if (!text || strlen(text) != ONLINE_HASH_HEX_SIZE - 1) return 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (!isxdigit(*p)) return 0;
    }
    return 1;
}

static int parse_hash_line(const char *line, const char *word, char *hash,
                           size_t size)
{
    if (!line || !hash || size < ONLINE_HASH_HEX_SIZE) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, word)) return 0;
    if (!take_token(&p, token, sizeof(token)) || !is_hex_hash(token)) return 0;
    if (*skip_spaces(p) != '\0') return 0;
    for (size_t i = 0; token[i]; ++i) {
        hash[i] = (char)tolower((unsigned char)token[i]);
    }
    hash[ONLINE_HASH_HEX_SIZE - 1] = '\0';
    return 1;
}

int online_parse_rematch(const char *line, int *match_id, int *accepted)
{
    if (!line || !match_id || !accepted) return 0;
    const char *p = line;
    char token[ONLINE_MAX_LINE];
    if (!take_token(&p, token, sizeof(token)) ||
        !online_word(token, "REMATCH")) return 0;
    if (!take_token(&p, token, sizeof(token)) ||
        !online_parse_int(token, match_id)) return 0;
    if (!take_token(&p, token, sizeof(token))) return 0;
    if (strcmp(token, "YES") == 0) *accepted = 1;
    else if (strcmp(token, "NO") == 0) *accepted = 0;
    else return 0;
    return 1;
}

const char *online_token(void)
{
    const char *token = getenv("SHOGUN_TOKEN");
    return token && *token ? token : NULL;
}

uint64_t online_hash(const char *nonce, const char *token)
{
    uint64_t hash = 1469598103934665603ULL;
    const char *parts[2] = {nonce ? nonce : "", token ? token : ""};
    for (int part = 0; part < 2; ++part) {
        for (const unsigned char *p = (const unsigned char *)parts[part]; *p;
             ++p) {
            hash ^= *p;
            hash *= 1099511628211ULL;
        }
        if (part == 0) {
            hash ^= ':';
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

char *online_hash_hex(uint64_t hash, char *out, size_t size)
{
    if (!out || size < ONLINE_HASH_HEX_SIZE) return NULL;
    snprintf(out, size, "%016llx", (unsigned long long)hash);
    return out;
}

int online_parse_auth(const char *line, char *hash, size_t size)
{
    return parse_hash_line(line, "AUTH", hash, size);
}

int online_parse_nonce(const char *line, char *nonce, size_t size)
{
    return parse_hash_line(line, "NONCE", nonce, size);
}
