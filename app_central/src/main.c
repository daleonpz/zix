/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <stdint.h>
#include <string.h>

#include <zephyr/net_buf.h>

#include <common/bt_str.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

//
// Enable or disable settings
//
#define CERT_EXCHANGE_ENABLE 1
#define OOB_EXCHANGE_ENABLE  1
#define CERT_LOG_ENABLE      0
#define OOB_LOG_ENABLE       0


struct bt_le_oob oob_local;
struct bt_le_oob oob_remote;


/******************************************************************************
 * Hardcoded Certs for testing - replace with actual certs as needed
 * ****************************************************************************/
#define MAX_CERT_SIZE 256
static const uint8_t _CENTRAL_CERT[MAX_CERT_SIZE] = "---BEGIN CERTIFICATE---\n"
                        "miibiJanbGKQHKIg9W0baqefaaocaq8amiibcGkcaqeaZvHe\n"
                        "-----END CERTIFICATE-----\n";
static const uint16_t _CENTRAL_CERT_LEN = sizeof(_CENTRAL_CERT);
static uint8_t _DEV_CERT[MAX_CERT_SIZE];
static uint16_t _DEV_CERT_LEN = 0;

/******************************************************************************
 * Custom Services UUIDS for certificate validation and secure data write
 ******************************************************************************/
#define BT_UUID_CERT_SERVICE_VAL                                                                   \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

// Characteristic UUID for writing device certificate
#define BT_UUID_CERT_DEVICE_CERTIFICATE_UUID_VAL                                                   \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

// Characteristic UUID for reading central certificate
#define BT_UUID_CERT_CENTRAL_CERTIFICATE_UUID_VAL                                                  \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

// Characteristic UUID for writing OOB data
#define BT_UUID_CERT_DEVICE_OOB_DATA_UUID_VAL                                                        \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

// Characteristic UUID for reading OOB data
#define BT_UUID_CERT_CENTRAL_OOB_DATA_UUID_VAL                                                        \
    BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef4)

// 128-bit UUID definitions
static const struct bt_uuid_128 cert_service_uuid = BT_UUID_INIT_128(BT_UUID_CERT_SERVICE_VAL);
static const struct bt_uuid_128 device_certificate_uuid =
    BT_UUID_INIT_128(BT_UUID_CERT_DEVICE_CERTIFICATE_UUID_VAL);
static const struct bt_uuid_128 central_certificate_uuid =
    BT_UUID_INIT_128(BT_UUID_CERT_CENTRAL_CERTIFICATE_UUID_VAL);
static const struct bt_uuid_128 device_oob_data_uuid =
    BT_UUID_INIT_128(BT_UUID_CERT_DEVICE_OOB_DATA_UUID_VAL);
static const struct bt_uuid_128 central_oob_data_uuid =
    BT_UUID_INIT_128(BT_UUID_CERT_CENTRAL_OOB_DATA_UUID_VAL);


uint16_t handle_send_central_cert;
uint16_t handle_recv_device_cert;
uint16_t handle_send_central_oob;
uint16_t handle_recv_device_oob;
/******************************************************************************
 * GATT Read/Write Variables and Callbacks
 ******************************************************************************/
// READ from Central Certificate Characteristic Callback
// WRITE to Device Certificate Characteristic Callback
static struct bt_gatt_exchange_params mtu_exchange_params;

static inline void await_signal(struct k_poll_signal *sig)
{
    struct k_poll_event events[] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
    };
    k_poll(events, ARRAY_SIZE(events), K_FOREVER);
}

static uint8_t *received_data;
static size_t received_data_size;
// static struct key_material keymat;

static bt_addr_le_t peer_addr;
static struct bt_conn *default_conn;

static struct bt_conn_cb central_cb;
static struct bt_conn_auth_cb central_auth_cb;
static struct bt_conn_auth_info_cb central_auth_info_cb;

static struct k_poll_signal conn_signal;
static struct k_poll_signal passkey_enter_signal;
static struct k_poll_signal device_found_cb_completed;

/* GATT Discover data */
static uint8_t gatt_disc_err;
static uint16_t gatt_disc_end_handle;
static uint16_t gatt_disc_start_handle;
static struct k_poll_signal gatt_disc_signal;

/* GATT Read data */
static uint8_t gatt_read_err;
static uint8_t *gatt_read_res;
static uint16_t gatt_read_len;
static uint16_t gatt_read_handle;
static struct k_poll_signal gatt_read_signal;

/* GATT Write data */
static uint8_t gatt_write_err;
static uint16_t gatt_write_handle;
static struct k_poll_signal gatt_write_signal;

// static bool data_parse_cb(struct bt_data *data, void *user_data)
// {
//  size_t *parsed_data_size = (size_t *)user_data;
// 
//  if (data->type == BT_DATA_ENCRYPTED_AD_DATA) {
//      int err;
//      struct net_buf_simple decrypted_buf;
//      size_t decrypted_data_size = BT_EAD_DECRYPTED_PAYLOAD_SIZE(data->data_len);
//      uint8_t decrypted_data[decrypted_data_size];
// 
//      err = bt_ead_decrypt(keymat.session_key, keymat.iv, data->data, data->data_len,
//                   decrypted_data);
//      if (err < 0) {
//          LOG_ERR("Error during decryption (err %d)", err);
//      }
// 
//      net_buf_simple_init_with_data(&decrypted_buf, &decrypted_data[0],
//                        decrypted_data_size);
// 
//      bt_data_parse(&decrypted_buf, &data_parse_cb, user_data);
//  } else {
//      LOG_INF("len : %u", data->data_len);
//      LOG_INF("type: 0x%02x", data->type);
//      LOG_HEXDUMP_INF(data->data, data->data_len, "data:");
// 
//      /* Copy the data out if we are running in a test context */
//      if (received_data != NULL) {
//          if (bt_data_get_len(data, 1) <=
//              (received_data_size - (*parsed_data_size))) {
//              *parsed_data_size +=
//                  bt_data_serialize(data, &received_data[*parsed_data_size]);
//          }
//      } else {
//          *parsed_data_size += bt_data_get_len(data, 1);
//      }
//  }
// 
//  return true;
// }

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
             struct net_buf_simple *ad)
{
    int err;
    size_t parsed_data_size;
    char addr_str[BT_ADDR_LE_STR_LEN];

    // Ignore if we are already connected
    if (default_conn) {
        LOG_DBG("Already connected, ignoring device found.");
        return;
    }

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    /* We are only interested in the previously connected device. */
    if (!bt_addr_le_eq(addr, &peer_addr)) {
//      LOG_DBG("Ignoring unrecognized device: %s (RSSI %d)", addr_str, rssi);
        return;
    }

    LOG_DBG("Peer found.");

//  parsed_data_size = 0;
//  LOG_INF("Received data size: %zu", ad->len);
//  bt_data_parse(ad, data_parse_cb, &parsed_data_size);

//  LOG_DBG("All data parsed. (total size: %zu)", parsed_data_size);

    err = bt_le_scan_stop();
    if (err) {
        LOG_DBG("Failed to stop scanner (err %d)", err);
        return;
    }

    k_poll_signal_raise(&device_found_cb_completed, 0);
}

static void connect_device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                 struct net_buf_simple *ad)
{
    int err;
    char addr_str[BT_ADDR_LE_STR_LEN];

    if (default_conn) {
        LOG_DBG("Already connected, ignoring device found.");
        return;
    }

    /* Connect only to devices in close range */
    if (rssi < -70) {
        LOG_DBG("Device found (%d dBm) is out of range", rssi);
        return;
    }

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    LOG_DBG("Device found: %s (RSSI %d)", addr_str, rssi);

    err = bt_le_scan_stop();
    if (err) {
        LOG_DBG("Failed to stop scanner (err %d)", err);
        return;
    }

    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
                &default_conn);
    if (err) {
        LOG_DBG("Failed to connect to %s (err %d)", addr_str, err);
        return;
    }

    k_poll_signal_raise(&device_found_cb_completed, 0);
}

static int start_scan(bool connect)
{
    int err;

    k_poll_signal_reset(&conn_signal);
    k_poll_signal_reset(&device_found_cb_completed);

    //  Passive scanning to save power, but for testing use active scanning
//  err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, connect ? connect_device_found : device_found);
    err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, connect ? connect_device_found : device_found);
    if (err) {
        LOG_DBG("Scanning failed to start (err %d)", err);
        return -1;
    }

    LOG_DBG("Scanning started.");

    if (connect) {
        LOG_DBG("Waiting for connection");
        await_signal(&conn_signal);
    }

    await_signal(&device_found_cb_completed);
    return 0;
}

static uint8_t gatt_read_cb(struct bt_conn *conn, uint8_t att_err,
                struct bt_gatt_read_params *params, const void *data, uint16_t read_len)
{
    gatt_read_err = att_err;
    gatt_read_len = read_len;
    gatt_read_handle = params->by_uuid.start_handle;

    if (!att_err) {
        LOG_DBG("GATT read complete for handle 0x%04x, read %u bytes, offset %u",
            params->by_uuid.start_handle, read_len, params->single.offset);
        LOG_HEXDUMP_DBG(data, read_len, "GATT read data:");
        memcpy(gatt_read_res, data, read_len);
        k_poll_signal_raise(&gatt_read_signal, 0);
    } else {
        LOG_ERR("Read ATT error (err %d)", att_err);
    }

    return BT_GATT_ITER_STOP;
}

static int gatt_write_cb(struct bt_conn *conn, uint8_t att_err,
              struct bt_gatt_write_params *params)
{
    gatt_write_err = att_err;
    gatt_write_handle = params->handle;
    if (!att_err) {
        LOG_DBG("GATT write complete for handle 0x%04x", params->handle);
        k_poll_signal_raise(&gatt_write_signal, 0);
    } else {
        LOG_ERR("Write ATT error (err %d)", att_err);
    }

    return 0;
}

// TODO: WIP - gatt writing is not working yet
static int gatt_write(struct bt_conn *conn, const struct bt_uuid *uuid, const uint8_t *data,
              size_t write_size, uint16_t start_handle, uint16_t end_handle)
{
    int err;
    size_t offset;
    uint16_t handle;
    struct bt_gatt_write_params params;

    offset = 0;

//  //  The whole data cannot be written in one write without response
//  err = bt_gatt_write_without_response(conn, handle, data, write_size, false);
//  if (err) {
//      LOG_DBG("GATT write failed (err %d)", err);
//      return -1;
//  }

//// this sends nothing, I got [00:00:00.311,000] <err> app: Write ATT error (err 6)
//  params.func = gatt_write_cb;
//  params.handle = start_handle + 2; // hardcoded : need a way to find the correct handle for the characteristic
//  params.offset = 0;
//  params.data = data;
//  params.length = write_size;
//  LOG_DBG("Writing data to handle 0x%04x", params.handle);
//  k_poll_signal_reset(&gatt_write_signal);
//  err = bt_gatt_write(conn, &params);
//  if (err) {
//      LOG_DBG("GATT write failed (err %d)", err);
//      return -1;
//  }
//  await_signal(&gatt_write_signal);



    size_t max_chunk_size = bt_gatt_get_mtu(conn) - 3;
    if (max_chunk_size > BT_ATT_MAX_ATTRIBUTE_LEN) {
        max_chunk_size = BT_ATT_MAX_ATTRIBUTE_LEN;
    }
    // hardcoded : need a way to find the correct handle for the characteristic
//     handle = start_handle + 2;
    

    if (bt_uuid_cmp(uuid, &central_certificate_uuid.uuid) == 0) {
        handle = handle_send_central_cert;
    } else if (bt_uuid_cmp(uuid, &central_oob_data_uuid.uuid) == 0) {
        handle = handle_send_central_oob;
    } else {
        LOG_ERR("Unknown UUID for GATT write");
        return -1;
    }
    LOG_DBG("Determined handle 0x%04x for UUID %s", handle, bt_uuid_str(uuid));

    while (offset < write_size) {
        LOG_DBG("Writing data at offset %zu to handle 0x%04x", offset, handle);
        size_t chunk_size = write_size - offset;
        if (chunk_size > max_chunk_size) {
            chunk_size = max_chunk_size;
        }
        params.handle = handle;
        params.offset = offset;
        params.data = &data[offset];
        params.length = chunk_size;
        params.func  = gatt_write_cb;

        k_poll_signal_reset(&gatt_write_signal);
        err = bt_gatt_write(conn, &params);
        if (err) {
            LOG_DBG("GATT write failed (err %d)", err);
            return -1;
        }
        await_signal(&gatt_write_signal);
        LOG_HEXDUMP_DBG(&data[offset], chunk_size, "GATT write data:");
        offset += chunk_size;
    }


//  while (offset < write_size) {
//      LOG_DBG("Writing data at offset %zu to handle 0x%04x", offset, handle);
//      size_t chunk_size = write_size - offset;
//      if (chunk_size > max_chunk_size) {
//          chunk_size = max_chunk_size;
//      }
//      LOG_HEXDUMP_DBG(&data[offset], chunk_size, "GATT write data:");
//      err = bt_gatt_write_without_response(conn, handle, &data[offset], chunk_size, false);
//      if (err) {
//          LOG_DBG("GATT write failed (err %d)", err);
//          return -1;
//      }
// 
//      offset += chunk_size;
//  }

    return 0;
}

static int gatt_read(struct bt_conn *conn, const struct bt_uuid *uuid, size_t read_size,
             uint16_t start_handle, uint16_t end_handle, uint8_t *buf)
{
    int err;
    size_t offset;
    uint16_t handle;
    struct bt_gatt_read_params params;

    gatt_read_res = &buf[0];

    params.handle_count = 0;
    params.by_uuid.start_handle = start_handle;
    params.by_uuid.end_handle = end_handle;
    params.by_uuid.uuid = uuid;
    params.func = gatt_read_cb;

    k_poll_signal_reset(&gatt_read_signal);

    err = bt_gatt_read(conn, &params);
    if (err) {
        LOG_DBG("GATT read failed (err %d)", err);
        return -1;
    }

    await_signal(&gatt_read_signal);

    offset = gatt_read_len;
    handle = gatt_read_handle;
    LOG_DBG("Read %u bytes from handle 0x%04x", gatt_read_len, handle);

    while (offset < read_size) {
        gatt_read_res = &buf[offset];

        params.handle_count = 1;
        params.single.handle = handle;
        params.single.offset = offset;

        k_poll_signal_reset(&gatt_read_signal);

        err = bt_gatt_read(conn, &params);
        if (err) {
            LOG_DBG("GATT read failed (err %d)", err);
            return -1;
        }

        await_signal(&gatt_read_signal);

        offset += gatt_read_len;
    }

    _DEV_CERT_LEN = offset;
    LOG_DBG("Total read size: %u", _DEV_CERT_LEN);
    return 0;
}

static uint8_t gatt_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                struct bt_gatt_discover_params *params)
{
    gatt_disc_err = attr ? 0 : BT_ATT_ERR_ATTRIBUTE_NOT_FOUND;

    LOG_DBG("GATT discover complete, err %d", gatt_disc_err);
    if (attr) {
        LOG_DBG("Attribute found: handle 0x%04x", attr->handle);
        gatt_disc_start_handle = attr->handle;
        gatt_disc_end_handle = ((struct bt_gatt_service_val *)attr->user_data)->end_handle;
    }

    struct bt_gatt_chrc *chrc;
    char str[BT_UUID_STR_LEN];
    chrc = (struct bt_gatt_chrc *)attr->user_data;
    bt_uuid_to_str(chrc->uuid, str, sizeof(str));
    LOG_DBG("Characteristic UUID: %s", str);

    if (bt_uuid_cmp(chrc->uuid, &central_certificate_uuid.uuid) == 0) {
        LOG_DBG("Found Central Certificate Characteristic");
        handle_send_central_cert = chrc->value_handle;
        LOG_DBG("Central Certificate Characteristic handle: 0x%04x", handle_send_central_cert);
    } else if (bt_uuid_cmp(chrc->uuid, &central_oob_data_uuid.uuid) == 0) {
        LOG_DBG("Found Central OOB Data Characteristic");
        handle_send_central_oob = chrc->value_handle;
        LOG_DBG("Central OOB Data Characteristic handle: 0x%04x", handle_send_central_oob);
    } else if (bt_uuid_cmp(chrc->uuid, &device_certificate_uuid.uuid) == 0) {
        LOG_DBG("Found Device Certificate Characteristic");
        handle_recv_device_cert = chrc->value_handle;
        LOG_DBG("Device Certificate Characteristic handle: 0x%04x", handle_recv_device_cert);
    } else if (bt_uuid_cmp(chrc->uuid, &device_oob_data_uuid.uuid) == 0) {
        LOG_DBG("Found Device OOB Data Characteristic");
        handle_recv_device_oob = chrc->value_handle;
        LOG_DBG("Device OOB Data Characteristic handle: 0x%04x", handle_recv_device_oob);
    } else {
        LOG_DBG("Unknown characteristic found in discovery");
    }

    k_poll_signal_raise(&gatt_disc_signal, 0);

    return BT_GATT_ITER_STOP;
}

static int gatt_discover_by_uuid(struct bt_conn *conn, const struct bt_uuid *service_type,
                     uint16_t *start_handle, uint16_t *end_handle, uint8_t type)
{
    int err;
    struct bt_gatt_discover_params params;

    LOG_DBG("Discovering primary service");
//     params.type = BT_GATT_DISCOVER_PRIMARY;
    params.type = type;
    params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    params.uuid = service_type;
    params.func = gatt_discover_cb;

    k_poll_signal_reset(&gatt_disc_signal);

    err = bt_gatt_discover(conn, &params);
    if (err) {
        LOG_DBG("Primary service discover failed (err %d)", err);
        return -1;
    }

    await_signal(&gatt_disc_signal);

    *start_handle = gatt_disc_start_handle;
    *end_handle = gatt_disc_end_handle;
    LOG_DBG("Service handles: start 0x%04x, end 0x%04x", *start_handle, *end_handle);

    return gatt_disc_err;
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params) {
    LOG_DBG("%s: MTU exchange %s (%u)", __func__, err == 0U ? "successful" : "failed", bt_gatt_get_mtu(conn));
}

static int mtu_exchange(struct bt_conn *conn)
{
    int err;
    LOG_DBG("%s: Current MTU = %u", __func__, bt_gatt_get_mtu(conn));
    mtu_exchange_params.func = mtu_exchange_cb;
    LOG_DBG("%s: Exchanging MTU...", __func__);
    err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
    if (err) {
        LOG_ERR("%s: MTU exchange failed (err %d)", __func__, err);
    }
    return err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_DBG("Failed to connect to %s (err %u)", addr, conn_err);
        bt_conn_unref(default_conn);
        default_conn = NULL;
        (void)start_scan(true);
        return;
    }

    LOG_DBG("Connected to: %s", addr);
    (void)mtu_exchange(conn);





// #if defined(CONFIG_BT_SMP)
//  bt_le_oob_set_sc_flag(true); // enable LESC OOB for this connection
// #endif

// #if defined(CONFIG_BT_SMP)
//  /* Update connection security level */
//  err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
//  if (err) {
//      LOG_ERR("Failed to set security (err %d)", err);
//      return -3;
//  }
// #endif

    k_poll_signal_raise(&conn_signal, 0);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Disconnected: %s, reason 0x%02x %s", addr, reason, bt_hci_err_to_str(reason));
    if (default_conn != conn) {
        return;
    }
    bt_conn_unref(default_conn);
    default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        LOG_DBG("Security changed: %s level %u", addr, level);
    } else {
        LOG_DBG("Security failed: %s level %u err %d %s", addr, level,
            err, bt_security_err_to_str(err));
    }
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                  const bt_addr_le_t *identity)
{
    char addr_identity[BT_ADDR_LE_STR_LEN];
    char addr_rpa[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
    bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

    LOG_DBG("Identity resolved %s -> %s", addr_rpa, addr_identity);

    bt_addr_le_copy(&peer_addr, identity);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    char passkey_str[7];
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);
    printk("Passkey for %s: %s\n", addr, passkey_str);
    k_poll_signal_raise(&passkey_enter_signal, 0);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char passkey_str[7];
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);
    LOG_DBG("Passkey for %s: %s", addr, passkey_str);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing failed conn: %s, reason %d %s\n", addr, reason,
        bt_security_err_to_str(reason));
}

enum bt_security_err pairing_accept(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat)
{
//     bt_shell_print("Remote pairing features: "
//                "IO: 0x%02x, OOB: %d, AUTH: 0x%02x, Key: %d, "
//                "Init Kdist: 0x%02x, Resp Kdist: 0x%02x",
//                feat->io_capability, feat->oob_data_flag,
//                feat->auth_req, feat->max_enc_key_size,
//                feat->init_key_dist, feat->resp_key_dist);

    LOG_DBG("--- Accepting pairing");
    return BT_SECURITY_ERR_SUCCESS;
}

static void oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
    int err;
    LOG_DBG("-------LESC OOB data requested");
    if (info->type != BT_CONN_OOB_LE_SC) {
        LOG_DBG("OOB data request type not LESC");
        return;
    }
    // remote and local are the same in this test, because central and peripheral are using
    // debug OOB data
    // CONFIG_BT_TESTING=y
    // CONFIG_BT_OOB_DATA_FIXED=y
    // CONFIG_BT_USE_DEBUG_KEYS=y
    bt_le_oob_set_sc_data(conn, &oob_local.le_sc_data, &oob_remote.le_sc_data);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing cancelled: %s", addr);
}

static int init_bt(void)
{
    int err;

    default_conn = NULL;

    k_poll_signal_init(&conn_signal);
//  k_poll_signal_init(&passkey_enter_signal);
    k_poll_signal_init(&gatt_disc_signal);
    k_poll_signal_init(&gatt_read_signal);
    k_poll_signal_init(&gatt_write_signal);
    k_poll_signal_init(&device_found_cb_completed);

    
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }
    LOG_DBG("Bluetooth initialized");

        /* Initialize settings */
#if defined(CONFIG_BT_SETTINGS)
    if (IS_ENABLED(CONFIG_SETTINGS)) {
        LOG_INF("Calling settings_load()");
        err = settings_load();
        if (err < 0) {
            LOG_ERR("Settings load failed (err %d)", err);
            return err;
        }
    }
    LOG_DBG("Settings loaded");
#endif

#if defined(CONFIG_BT_USE_DEBUG_KEYS)
    err = bt_unpair(BT_ID_DEFAULT, NULL);
	if (err) {
		LOG_ERR("Bond remove failed err: %d", err);
	} else {
		LOG_INF("All bond removed");
	}
#endif

    bt_le_oob_set_sc_flag(true);

    err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
    if (err) {
        LOG_ERR("Unpairing failed (err %d)", err);
    }

    central_cb.connected = connected;
    central_cb.disconnected = disconnected;
#if defined(CONFIG_BT_SMP)
    central_cb.security_changed = security_changed;
//  central_cb.identity_resolved = identity_resolved;
#endif

    err = bt_conn_cb_register(&central_cb);
    if (err) {
        LOG_ERR("Failed to register connection callbacks (err %d)", err);
        return -1;
    }

#if defined(CONFIG_BT_SMP)
    central_auth_cb.pairing_confirm = NULL;
//  central_auth_cb.passkey_confirm = auth_passkey_confirm;
//  central_auth_cb.passkey_display = auth_passkey_display;
    central_auth_cb.passkey_confirm = NULL;
    central_auth_cb.passkey_display = NULL;
    central_auth_cb.passkey_entry = NULL;
//  central_auth_cb.oob_data_request = NULL;
    central_auth_cb.oob_data_request = oob_data_request;
    central_auth_cb.pairing_accept = pairing_accept;
    central_auth_cb.cancel = auth_cancel;

    err = bt_conn_auth_cb_register(&central_auth_cb);
    if (err) {
        LOG_ERR("Failed to register authorization callbacks (err %d)", err);
        return -1;
    }

    /***************************************************************/
    /* in peripheral role, we need to register auth callbacks to handle
     */
    central_auth_info_cb.pairing_complete = pairing_complete;
    central_auth_info_cb.pairing_failed = pairing_failed;
    central_auth_info_cb.bond_deleted = NULL;
    err = bt_conn_auth_info_cb_register(&central_auth_info_cb);
    if (err) {
        LOG_ERR("Failed to register authorization info callbacks (err %d)", err);
        return -1;
    }
    /****************************************************************/
#endif
    return 0;
}

int main(void)
{
    int err;
    bool connect;
    uint16_t end_handle;
    uint16_t start_handle;

    /* Initialize Bluetooth and callbacks */
    err = init_bt();
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return -1;
    }

    /* Start scan and connect to our peripheral */
    connect = true;
    err = start_scan(connect);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return -2;
    }

//  await_signal(&passkey_enter_signal);

    /* Update connection security level */
//  err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
//  if (err) {
//      LOG_ERR("Failed to set security (err %d)", err);
//      return -3;
//  }
//  err = bt_conn_auth_passkey_confirm(default_conn);
//  if (err) {
//      LOG_ERR("Security update failed");
//      return -4;
//  }

    /* Locate the primary service */
//  err = gatt_discover_primary_service(default_conn, BT_UUID_CUSTOM_SERVICE, &start_handle,
//                      &end_handle);
    err = gatt_discover_by_uuid(default_conn, &cert_service_uuid.uuid, &start_handle, &end_handle, BT_GATT_DISCOVER_PRIMARY);
    if (err) {
        LOG_ERR("Service not found (err %d)", err);
        return -5;
    }
    err = gatt_discover_by_uuid(default_conn, &central_certificate_uuid.uuid, &start_handle, &end_handle, BT_GATT_DISCOVER_CHARACTERISTIC);
    if (err) {
        LOG_ERR("Central Certificate Characteristic not found (err %d)", err);
        return -5;
    }
    err = gatt_discover_by_uuid(default_conn, &device_certificate_uuid.uuid, &start_handle, &end_handle, BT_GATT_DISCOVER_CHARACTERISTIC);
    if (err) {
        LOG_ERR("Device Certificate Characteristic not found (err %d)", err);
        return -5;
    }
    err = gatt_discover_by_uuid(default_conn, &central_oob_data_uuid.uuid, &start_handle, &end_handle, BT_GATT_DISCOVER_CHARACTERISTIC);
    if (err) {
        LOG_ERR("Central OOB Data Characteristic not found (err %d)", err);
        return -5;
    }
    err = gatt_discover_by_uuid(default_conn, &device_oob_data_uuid.uuid, &start_handle, &end_handle, BT_GATT_DISCOVER_CHARACTERISTIC);
    if (err) {
        LOG_ERR("Device OOB Data Characteristic not found (err %d)", err);
        return -5;
    }
       
#if CERT_EXCHANGE_ENABLE
    /* Read the Device certificate characteristic */
//     err = gatt_read(default_conn, &device_certificate_uuid.uuid, _CENTRAL_CERT_LEN, start_handle, end_handle, _DEV_CERT);
    err = gatt_read(default_conn, &device_certificate_uuid.uuid, sizeof(_DEV_CERT), handle_recv_device_cert, handle_recv_device_cert, _DEV_CERT);
    if (err) {
        LOG_ERR("GATT read failed (err %d)", err);
        return -6;
    }
#if CERT_LOG_ENABLE
    LOG_DBG("Received Device Certificate:");
    LOG_HEXDUMP_DBG(_DEV_CERT, _DEV_CERT_LEN, "Device Certificate:");
#endif
    err = gatt_write(default_conn, &central_certificate_uuid.uuid, _CENTRAL_CERT, _CENTRAL_CERT_LEN, start_handle, end_handle);
    if (err) {
        LOG_ERR("GATT write failed (err %d)", err);
        return -8;
    }
#endif
#if OOB_EXCHANGE_ENABLE
    /* Read the Device certificate characteristic */
    char addr_str[BT_ADDR_LE_STR_LEN];

    uint8_t oob_data_len = sizeof(oob_remote);
    uint8_t oob_data_buf[oob_data_len];

    LOG_DBG("Reading OOB data from peripheral...");
//     err = gatt_read(default_conn, &device_oob_data_uuid.uuid, oob_data_len, start_handle, end_handle, oob_data_buf);
    err = gatt_read(default_conn, &device_oob_data_uuid.uuid, oob_data_len, handle_recv_device_oob, handle_recv_device_oob, oob_data_buf);
    if (err) {
        LOG_ERR("OOB device read failed (err %d)", err);
        return -6;
    }
    memcpy(&oob_remote, oob_data_buf, sizeof(oob_remote));
#if OOB_LOG_ENABLE
    LOG_DBG(">>>>> Received OOB data:");
    bt_addr_le_to_str(&oob_remote.addr, addr_str, sizeof(addr_str));
    LOG_DBG("OOB data from %s:", addr_str);
    LOG_HEXDUMP_DBG(oob_remote.le_sc_data.r, sizeof(oob_remote.le_sc_data.r),
            "Remote OOB Randomizer R:");
    LOG_HEXDUMP_DBG(oob_remote.le_sc_data.c, sizeof(oob_remote.le_sc_data.c),
            "Remote OOB Hash C:");
#endif
    err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
    if (err) {
        LOG_ERR("Error while fetching local OOB data: %d", err);
        return -7;
    }
#if OOB_LOG_ENABLE
    bt_addr_le_to_str(&oob_local.addr, addr_str, sizeof(addr_str));
    LOG_DBG("<<<<< Local OOB data for %s:", addr_str);
    LOG_HEXDUMP_DBG(oob_local.le_sc_data.r, sizeof(oob_local.le_sc_data.r),
            "Local OOB Randomizer R:");
    LOG_HEXDUMP_DBG(oob_local.le_sc_data.c, sizeof(oob_local.le_sc_data.c),
            "Local OOB Hash C:");
#endif
    memcpy(oob_data_buf, &oob_local, sizeof(oob_local));
    LOG_DBG(">>>> Sending OOB data to peripheral...");
    err = gatt_write(default_conn, &central_oob_data_uuid.uuid, oob_data_buf, oob_data_len, start_handle, end_handle);
    if (err) {
        LOG_ERR("OOB central write failed (err %d)", err);
        return -8;
    }
#endif // OOB_EXCHANGE_ENABLE

    k_sleep(K_MSEC(3000)); // wait for a while before disconnecting, to allow the central to read the data

// #ifdef CONFIG_BT_SMP
//     LOG_DBG("Setting security level to L4...");
//     err = bt_conn_set_security(default_conn, BT_SECURITY_L4);
//     if (err) {
//         LOG_ERR("Setting security failed (err %d)", err);
//         return -3;
//     }
// #endif
    k_sleep(K_MSEC(10000)); // wait for a while before disconnecting, to allow the central to read the data

    /* Start a new scan to get and decrypt the Advertising Data */
    err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        LOG_ERR("Failed to disconnect.");
        return -7;
    }

    connect = false;
    err = start_scan(connect);
    if (err) {
        return -2;
    }

    return 0;
}
