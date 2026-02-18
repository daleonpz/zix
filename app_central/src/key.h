#pragma once

#include "crypto.h"

extern const struct crypto_cert root_cert;
extern const uint8_t root_keypair[CRYPTO_KEYPAIR_SIZE];

extern const struct crypto_cert dev_cert;
extern const uint8_t dev_keypair[CRYPTO_KEYPAIR_SIZE];
