// Copyright (c) 2026 Daniel Paredes (daleonpz)
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <psa/crypto.h>

/******************************************************************************
 * Defines
 ****************************************************************************/
#define CRYPTO_DEV_ID_SIZE      (16)
#define CRYPTO_SIGNATURE_SIZE   (64)
#define CRYPTO_PUBLIC_KEY_SIZE  (65)
#define CRYPTO_KEYPAIR_SIZE     (32)
//
// Certificate structure:
// - version (1 byte)
// - key type (1 byte)
// - device id (16 bytes)
// - public key (65 bytes for uncompressed ECDSA)
// - signature (64 bytes for ECDSA)
//
#define CRYPTO_CERT_HEADER_SIZE (1 + 1 + CRYPTO_DEV_ID_SIZE + CRYPTO_PUBLIC_KEY_SIZE)
#define CRYPTO_CERT_SIZE        (CRYPTO_CERT_HEADER_SIZE + CRYPTO_SIGNATURE_SIZE)
#define CRYPTO_MAX_MESSAGE_SIZE (100)

typedef psa_key_id_t crypto_key_id;

/******************************************************************************
 * Data Structures
 ****************************************************************************/

/** @brief Structure representing a device certificate.
 *
 *  This structure contains the fields of a device certificate, including:
 *  - version    The version of the certificate format.
 *  - key_type   The type of the public key (e.g., ECDSA).
 *  - device_id  A unique identifier for the device.
 *  - pubkey     The public key associated with the device.
 *  - signature  The signature of the certificate, signed by the root private key.
 */
// TODO: should this be packed and aligned?
struct crypto_cert {
    uint8_t version;
    uint8_t key_type;
    uint8_t device_id[CRYPTO_DEV_ID_SIZE];
    uint8_t pubkey[CRYPTO_PUBLIC_KEY_SIZE];
    uint8_t signature[CRYPTO_SIGNATURE_SIZE];
};
_Static_assert(sizeof(struct crypto_cert) == CRYPTO_CERT_SIZE,
               "Certificate structure size mismatch!");

/** @brief Initialize the cryptographic subsystem.
 *
 *  @return 0 on success, negative error value on failure.
 */
int crypto_init(void);

/** @brief Import the device's key pair and the root public key into the cryptographic subsystem.
 *
 *  @return 0 on success, negative error value on failure.
 */
int crypto_import_keys(void);

/** @brief Sign a message using the device's private key.
 *
 *  @param message         The message to be signed.
 *  @param message_len     The length of the message in bytes.
 *  @param signature       Buffer to hold the generated signature.
 *  @param signature_len   Pointer to a variable that holds the size of the signature buffer.
 *                         On success, it will be updated with the actual size of the generated signature.
 *  @param signing_key_id  Optional:
 *                         The key ID to use for signing.
 *                         If 0, the device's default key pair will be used.
 *
 *  @return 0 on success, negative error value on failure.
 */
int crypto_sign_message(const uint8_t *message, size_t message_len, uint8_t *signature,
                        size_t *signature_len, crypto_key_id signing_key_id);

/** @brief Verify a message signature using the device's public key.
 *
 *  @param message       The original message that was signed.
 *  @param message_len   The length of the message in bytes.
 *  @param signature     The signature to be verified.
 *  @param signature_len The length of the signature in bytes.
 *
 *  @return 0 if the signature is valid, negative error value on failure.
 */
int crypto_verify_message(const uint8_t *message, size_t message_len, const uint8_t *signature,
                          size_t signature_len);

/**  @brief Verify the device's certificate using the root public key.
 *
 *  @param cert              Pointer to the certificate to be verified.
 *  @param cert_pubkey       Optional
 *                           Pointer to a variable to receive the key ID of the certificate's public key.
 *                           If provided, the certificate's public key will be imported
 *                           and its key ID will be stored in this variable.
 *
 *  @return 0 if the certificate is valid, negative error value on failure.
 */
int crypto_verify_cert(const struct crypto_cert *cert, crypto_key_id *cert_pubkey);

/** @brief Clean up the cryptographic subsystem, releasing any allocated resources.
 *
 *  @return 0 on success, negative error value on failure.
 */
int crypto_clean(void);
