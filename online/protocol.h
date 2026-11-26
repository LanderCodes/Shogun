#ifndef SHOGUN_ONLINE_PROTOCOL_H
#define SHOGUN_ONLINE_PROTOCOL_H

#include "core/types.h"

#include <stddef.h>
#include <stdint.h>

#define ONLINE_DEFAULT_PORT 7777
#define ONLINE_MAX_LINE 256
#define ONLINE_HASH_HEX_SIZE 17

void online_sanitize_name(const char *input, char *output, size_t size);
int online_word(const char *line, const char *word);
int online_parse_int(const char *text, int *value);
int online_parse_start(const char *line, int *match_id, uint32_t *seed,
                       int *side);
int online_parse_challenge(const char *line, int *match_id, char *name,
                           size_t size);
int online_parse_cancelled(const char *line, int *match_id, char *reason,
                           size_t size);
int online_parse_move(const char *line, int *match_id, int *from, int *to);
int online_parse_rematch(const char *line, int *match_id, int *accepted);

const char *online_token(void);
uint64_t online_hash(const char *nonce, const char *token);
char *online_hash_hex(uint64_t hash, char *out, size_t size);
int online_parse_auth(const char *line, char *hash, size_t size);
int online_parse_nonce(const char *line, char *nonce, size_t size);

#endif
