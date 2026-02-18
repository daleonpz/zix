#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <assert.h>

#include "key.h"
#include "crypto.h"
#include "utils.h"

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

/******************************************************************************
 * Defines
 ****************************************************************************/
#define LOG_LEVEL CONFIG_APP_LOG_LEVEL
LOG_MODULE_REGISTER(CRYPTO);

#define CRYPTO_MAX_MESSAGE_SIZE (100)

/******************************************************************************
 * Global Variables
 ****************************************************************************/
static crypto_key_id _dev_keypair_id;
static crypto_key_id _dev_pubkey_id;
static crypto_key_id _root_pubkey_id;

/******************************************************************************
 * Helper functions
 ****************************************************************************/
static int import_key(const uint8_t *key, size_t key_size, crypto_key_id *key_id, int is_public)
{
    psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t status;

    assert(key != NULL);
    assert(key_id != NULL);
    assert(key_size == CRYPTO_PUBLIC_KEY_SIZE || key_size == CRYPTO_KEYPAIR_SIZE);

    // Configure the key attributes
    if (is_public) {
        psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
    } else {
        psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_SIGN_HASH);
        psa_set_key_type(&key_attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
    }
    psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
    psa_set_key_algorithm(&key_attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
    psa_set_key_bits(&key_attributes, 256);

    // Import the key into the PSA Crypto subsystem
    status = psa_import_key(&key_attributes, key, key_size, key_id);
    if (status != PSA_SUCCESS) {
        LOG_INF("psa_import_key failed! (Error: %d)", status);
        return -1;
    }

    // Reset key attributes and free any allocated resources.
    psa_reset_key_attributes(&key_attributes);
    return 0;
}

/******************************************************************************
 * Function Implementations
 ******************************************************************************/
/** Initialize the PSA Crypto subsystem.
 */
int crypto_init(void)
{
    psa_status_t status;

    status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        return -1;
    }
    return 0;
}

/** Clean up the PSA Crypto subsystem by destroying the imported keys.
 */
int crypto_clean(void)
{
    psa_status_t status;

    status = psa_destroy_key(_dev_keypair_id);
    if (status != PSA_SUCCESS) {
        LOG_INF("Destroying device key pair failed! (Error: %d)", status);
        return -1;
    }

    status = psa_destroy_key(_dev_pubkey_id);
    if (status != PSA_SUCCESS) {
        LOG_INF("Destroying device public key failed! (Error: %d)", status);
        return -1;
    }

    status = psa_destroy_key(_root_pubkey_id);
    if (status != PSA_SUCCESS) {
        LOG_INF("Destroying root public key failed! (Error: %d)", status);
        return -1;
    }
    return 0;
}

/** Import the device key pair, device public key, and root public key into the PSA Crypto subsystem.
 */
int crypto_import_keys(void)
{
    int status;

    status = import_key(dev_keypair, sizeof(dev_keypair), &_dev_keypair_id, 0);
    if (status != 0) {
        LOG_INF("Importing device key pair failed!");
        return -1;
    }

    status = import_key(dev_cert.pubkey, sizeof(dev_cert.pubkey), &_dev_pubkey_id, 1);
    if (status != 0) {
        LOG_INF("Importing device public key failed!");
        return -1;
    }

    status = import_key(root_cert.pubkey, sizeof(root_cert.pubkey), &_root_pubkey_id, 1);
    if (status != 0) {
        LOG_INF("Importing root public key failed!");
        return -1;
    }

    status = crypto_verify_cert(&dev_cert, NULL);
    if (status != 0) {
        LOG_INF("Device certificate verification failed during key import!");
        return -1;
    }

    return 0;
}

/** Verify the authenticity of the device certificate using the root public key.
 */
int crypto_verify_cert(const struct crypto_cert *cert, crypto_key_id *cert_pubkey_id)
{
    psa_status_t status;
    size_t len;
    uint8_t header[CRYPTO_CERT_HEADER_SIZE];
    crypto_key_id pubkey_id;

    if (cert == NULL) {
        LOG_INF("Certificate is NULL!");
        return -1;
    }

    len = CRYPTO_CERT_HEADER_SIZE;

    // NRF52840's cryptocell has a limitation where it can only process data up to 63 bytes if data is in flash.
    // To work around this, we copy the certificate header into RAM before calling psa_verify_message.
    memcpy(header, &cert->version, len);
    LOG_INF("Verifying certificate of length %u bytes...", len);
    status = psa_verify_message(_root_pubkey_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), header, len,
                                cert->signature, sizeof(cert->signature));
    if (status != PSA_SUCCESS) {
        LOG_INF("psa_verify_message failed! (Error: %d)", status);
        return -1;
    }

    LOG_INF("Certificate verification was successful!");

    if (cert_pubkey_id != NULL) {
        //
        // If caller provided a pointer to store the certificate's public key ID, import the public key and return its ID.
        //
        status = import_key(cert->pubkey, sizeof(cert->pubkey), &pubkey_id, 1);
        if (status != 0) {
            LOG_INF("Importing certificate public key failed!");
            return -1;
        }
        *cert_pubkey_id = pubkey_id;
    }

    return 0;
}

/** Sign a message using the device's private key.
 */
int crypto_sign_message(const uint8_t *message, size_t message_len, uint8_t *signature,
                        size_t *signature_len, crypto_key_id signing_key_id)
{
    uint32_t len;
    psa_status_t status;
    uint8_t msg[CRYPTO_MAX_MESSAGE_SIZE];
    crypto_key_id key_id;

    if (message == NULL || signature == NULL || signature_len == NULL) {
        LOG_INF("Invalid input to crypto_sign_message!");
        return -1;
    }

    if (message_len > CRYPTO_MAX_MESSAGE_SIZE) {
        LOG_INF("Message length exceeds maximum allowed size!");
        return -1;
    }

    if (*signature_len != CRYPTO_SIGNATURE_SIZE) {
        LOG_INF("Invalid signature buffer length! (Expected: %u, Actual: %u)",
                CRYPTO_SIGNATURE_SIZE, *signature_len);
        return -1;
    }

    // NRF52840's cryptocell has a limitation where it can only process data up to 63 bytes if data is in flash.
    // To work around this, we copy the certificate header into RAM before calling psa_verify_message.
    memcpy(msg, message, message_len);
    LOG_INF("Signing a message...");
    if (signing_key_id != 0) {
        key_id = signing_key_id;
    } else {
        key_id = _dev_keypair_id;
    }

    // len parameter is used to return the actual size of the generated signature.
    status = psa_sign_message(key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), msg, message_len, signature,
                              *signature_len, &len);
    if (status != PSA_SUCCESS) {
        LOG_INF("psa_sign_message failed! (Error: %d)", status);
        return -1;
    }

    // Sanity check, the generated signature length should always
    // be equal to CRYPTO_SIGNATURE_SIZE.
    if (len != CRYPTO_SIGNATURE_SIZE) {
        LOG_INF("Unexpected signature size! (Expected: %u, Actual: %u)", CRYPTO_SIGNATURE_SIZE,
                len);
        return -1;
    }

    LOG_INF("Signing was successful!");
    return 0;
}

/** Verify a message signature using the device's public key.
 */
int crypto_verify_message(const uint8_t *message, size_t message_len, const uint8_t *signature,
                          size_t signature_len)
{
    psa_status_t status;

    if (message == NULL || signature == NULL) {
        LOG_INF("Invalid input to crypto_verify_message!");
        return -1;
    }

    if (message_len > CRYPTO_MAX_MESSAGE_SIZE) {
        LOG_INF("Message length exceeds maximum allowed size!");
        return -1;
    }

    if (signature_len != CRYPTO_SIGNATURE_SIZE) {
        LOG_INF("Invalid signature length! (Expected: %u, Actual: %u)", CRYPTO_SIGNATURE_SIZE,
                signature_len);
        return -1;
    }

    LOG_INF("Verifying signature...");
    status = psa_verify_message(_dev_pubkey_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), message,
                                message_len, signature, signature_len);
    if (status != PSA_SUCCESS) {
        LOG_INF("psa_verify_message failed! (Error: %d)", status);
        return -1;
    }
    LOG_INF("Signature verification was successful!");
    return 0;
}
