/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <common/bt_str.h>

extern int mtu_exchange(struct bt_conn *conn);
extern struct bt_conn *conn_connected;

#define LOG_LEVEL CONFIG_APP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define AUTH_SC_FLAG      0x08
/******************************************************************************
 * Hardcoded Certs for testing - replace with actual certs as needed
 * ****************************************************************************/
#define MAX_CERT_SIZE 256
static const uint8_t _DEV_CERT[MAX_CERT_SIZE] = "---BEGIN CERTIFICATE---\n"
						"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzVhE\n"
						"-----END CERTIFICATE-----\n";
static const uint16_t _DEV_CERT_LEN = sizeof(_DEV_CERT);
static uint8_t _CENTRAL_CERT[MAX_CERT_SIZE];
static uint16_t _CENTRAL_CERT_LEN = 0;

/******************************************************************************
 * GATT Characteristic Callbacks
 * ****************************************************************************/
static ssize_t recv_central_certificate(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					const void *buf, uint16_t len, uint16_t offset,
					uint8_t flags);
static ssize_t send_device_certificate(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset);

/*******************************************************************************
 * Bluetooth Connection Callbacks Declaration
 ******************************************************************************/
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void connected(struct bt_conn *conn, uint8_t err);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

static void pairing_complete(struct bt_conn *conn, bool bonded);
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);

/******************************************************************************
 * Bluetooth Connection Authentication Callbacks Declaration
 * ***************************************************************************/
static void auth_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info);
static void auth_cancel(struct bt_conn *conn);
static enum bt_security_err pairing_accept(struct bt_conn *conn, const struct bt_conn_pairing_feat *const feat);
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

/******************************************************************************
 * Custom Services UUIDS for certificate validation and secure data write
 ******************************************************************************/
#define BT_UUID_CERT_SERVICE_VAL                                                                   \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

// Characteristic UUID for writing device certificate
#define BT_UUID_CERT_DEVICE_CERTIFICATE_UUID_VAL                                                   \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

// Characteristic UUID for reading central certificate
#define BN_UUID_CERT_CENTRAL_CERTIFICATE_UUID_VAL                                                  \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

// Characteristic UUID for writing secure data
#define BT_UUID_CERT_SECURE_DATA_UUID_VAL                                                          \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef3)

// 128-bit UUID definitions
static const struct bt_uuid_128 cert_service_uuid = BT_UUID_INIT_128(BT_UUID_CERT_SERVICE_VAL);
static const struct bt_uuid_128 device_certificate_uuid =
	BT_UUID_INIT_128(BT_UUID_CERT_DEVICE_CERTIFICATE_UUID_VAL);
static const struct bt_uuid_128 central_certificate_uuid =
	BT_UUID_INIT_128(BN_UUID_CERT_CENTRAL_CERTIFICATE_UUID_VAL);
static const struct bt_uuid_128 secure_data_uuid =
	BT_UUID_INIT_128(BT_UUID_CERT_SECURE_DATA_UUID_VAL);

// TODO: rename UUIDs device_certificate_uuid is confusing because it's for receiving central cert
// GATT Service Declaration
BT_GATT_SERVICE_DEFINE(cert_service, BT_GATT_PRIMARY_SERVICE(&cert_service_uuid),
		       BT_GATT_CHARACTERISTIC(&device_certificate_uuid.uuid, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE, NULL, recv_central_certificate,
					      NULL),
		       BT_GATT_CHARACTERISTIC(&central_certificate_uuid.uuid, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ, send_device_certificate, NULL,
					      NULL),
#if defined(CONFIG_BT_SMP)
		       BT_GATT_CHARACTERISTIC(&secure_data_uuid.uuid, BT_GATT_CHRC_WRITE,
					      BT_GATT_PERM_WRITE_AUTHEN, NULL, NULL, NULL),
#endif
);

/******************************************************************************
 * Bluetooth Connection Callbacks Implementation
 ******************************************************************************/
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
	} else {
		LOG_INF("Connected with device address: %s", bt_addr_le_str(bt_conn_get_dst(conn)));
	}
#if defined(CONFIG_BT_SMP)
// 	bt_le_oob_set_sc_flag(true); // enable LESC OOB for this connection
	/* Update connection security level */
	k_sleep(K_SECONDS(2));
	LOG_DBG("Setting security level to L4 (LESC MITM)");
	// add delay to avoid "Command Disallowed" error
	err = bt_conn_set_security(conn, BT_SECURITY_L4);
	if (err) {
		LOG_ERR("Failed to set security (err %d)", err);
	} else {
		LOG_DBG("Security set to L4 (LESC MITM) in progress");
	}
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr, level);
	} else {
		LOG_DBG("Security failed: %s level %u err %d %s", addr, level, err,
			bt_security_err_to_str(err));
	}
}

/*******************************************************************************
 * GATT Characteristic Callbacks Implementation
 * ****************************************************************************/
static ssize_t recv_central_certificate(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					const void *buf, uint16_t len, uint16_t offset,
					uint8_t flags)
{
	if ((offset + len) > MAX_CERT_SIZE) {
		LOG_ERR("Central certificate size exceeds maximum limit");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	memcpy(_CENTRAL_CERT + offset, buf, len);
	_CENTRAL_CERT_LEN += len;
	LOG_DBG("Received central certificate chunk: offset=%d, len=%d", offset, len);
	LOG_HEXDUMP_DBG(_CENTRAL_CERT, _CENTRAL_CERT_LEN, "Central Certificate Data:");
	return len;
}

static ssize_t send_device_certificate(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	if (offset > _DEV_CERT_LEN) {
		return 0;
	}
	uint16_t remaining = _DEV_CERT_LEN - offset;
	len = (len > remaining) ? remaining : len;
	memcpy(buf, _DEV_CERT + offset, len);
	LOG_DBG("Wrote device certificate chunk: offset=%d, len=%d", offset, len);
	LOG_HEXDUMP_DBG(_DEV_CERT, _DEV_CERT_LEN, "Device Certificate Data:");
	return len;
}

/*****************************************************************************
 * Bluetooth Advertising Data
 * **************************************************************************/
// advertising data
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CERT_SERVICE_VAL),
};

// scan response data
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

// MTU update callback
static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
	LOG_DBG("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
}
//
static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = mtu_updated
};

/*****************************************************************************
 * Bluetooth Connection Authentication Info Callbacks
 * ***************************************************************************/
#if defined(CONFIG_BT_SMP)
static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {.pairing_complete = pairing_complete,
							       .pairing_failed = pairing_failed};
#endif
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

/******************************************************************************
 * Bluetooth Connection Authentication Callbacks
 * ***************************************************************************/
#if defined(CONFIG_BT_SMP)
static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.pairing_accept = pairing_accept,
//     .passkey_display = auth_passkey_display,
	.oob_data_request = auth_oob_data_request,
};
#endif

static enum bt_security_err pairing_accept(struct bt_conn *conn,
					   const struct bt_conn_pairing_feat *const feat)
{
// 	if (feat->oob_data_flag && (!(feat->auth_req & AUTH_SC_FLAG))) {
// 		bt_le_oob_set_legacy_flag(true);
// 	}
// 	bt_le_oob_set_sc_flag(true);

	LOG_DBG("Pairing accepted");
	return BT_SECURITY_ERR_SUCCESS;
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_DBG("Pairing cancelled: %s", addr);
}
//
static void auth_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *info)
{
	int err;
	if (info->type != BT_CONN_OOB_LE_SC) {
		LOG_DBG("OOB data request type not LESC");
		return;
	}
	LOG_DBG("LESC OOB data requested");
	//     lesc_oob_data_set(conn, info);
	// I want to hardcode the OOB data for testing
	// Hardcoded OOB data for testing - replace with actual OOB data as needed
	// from the RPI
	// └─$ sudo btmgmt
	// [mgmt]# local-oob
	// [mgmt]# Hash C from P-192: 04be9ec3a936c1cb5d911fb1a3e675bb
	// [mgmt]# Randomizer R with P-192: 84c4f5186b998446764a200d3fed7678
	// [mgmt]# Hash C from P-256: e51586bedca1f6ea22cb13d90f61d3d6
	// [mgmt]# Randomizer R with P-256: f57e8c90c9e11d26899397f6599f4ae1
	//
	//
	//     oob_remote.r = (const uint8_t []){0xf5, 0x7e, 0x8c, 0x90, 0xc9, 0xe1, 0x1d, 0x26,
	//                           0x89, 0x93, 0x97, 0xf6, 0x59, 0x9f, 0x4a, 0xe1};
	//     oob_remote.c = (const uint8_t []){0xe5, 0x15, 0x86, 0xbe, 0xdc, 0xa1, 0xf6, 0xea,
	//                           0x22, 0xcb, 0x13, 0xd9, 0x0f, 0x61, 0xd3, 0xd6};
	//
	struct bt_le_oob oob_local;
	struct bt_le_oob_sc_data *oob_data_local = &oob_local.le_sc_data;
	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		LOG_ERR("Error while fetching local OOB data: %d", err);
	}
	//
	//     LOG_HEXDUMP_DBG(oob_local.r, sizeof(oob_local.r), "Local OOB Randomizer R:");
	//     LOG_HEXDUMP_DBG(oob_local.c, sizeof(oob_local.c), "Local OOB Hash C:");
	LOG_HEXDUMP_DBG(oob_data_local->r, sizeof(oob_data_local->r),
			"Local LE SC OOB Randomizer R:");
	LOG_HEXDUMP_DBG(oob_data_local->c, sizeof(oob_data_local->c), "Local LE SC OOB Hash C:");
	//
	// zephyr/subsys/bluetooth/host/smp.c
	//     uint8_t rand_num[] = {
	//                     0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	//                                 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	//                                         };
	// memcpy(le_sc_oob->r, rand_num, sizeof(le_sc_oob->r));
	//
	// err = bt_crypto_f4(sc_public_key, sc_public_key, le_sc_oob->r, 0,
	//           le_sc_oob->c);
	//              if (err) {
	//                      return err;
	//                          }
	//
	/* based on Core Specification 4.2 Vol 3. Part H 2.3.5.6.1 */
	// static const uint8_t debug_private_key_be[BT_PRIV_KEY_LEN] = {
	//   0x3f, 0x49, 0xf6, 0xd4, 0xa3, 0xc5, 0x5f, 0x38,
	//   0x74, 0xc9, 0xb3, 0xe3, 0xd2, 0x10, 0x3f, 0x50,
	//   0x4a, 0xff, 0x60, 0x7b, 0xeb, 0x40, 0xb7, 0x99,
	//   0x58, 0x99, 0xb8, 0xa6, 0xcd, 0x3c, 0x1a, 0xbd,
	// };
	//
	// static const uint8_t debug_public_key[BT_PUB_KEY_LEN] = {
	//   /* X */
	//   0xe6, 0x9d, 0x35, 0x0e, 0x48, 0x01, 0x03, 0xcc,
	//   0xdb, 0xfd, 0xf4, 0xac, 0x11, 0x91, 0xf4, 0xef,
	//   0xb9, 0xa5, 0xf9, 0xe9, 0xa7, 0x83, 0x2c, 0x5e,
	//   0x2c, 0xbe, 0x97, 0xf2, 0xd2, 0x03, 0xb0, 0x20,
	//   /* Y */
	//   0x8b, 0xd2, 0x89, 0x15, 0xd0, 0x8e, 0x1c, 0x74,
	//   0x24, 0x30, 0xed, 0x8f, 0xc2, 0x45, 0x63, 0x76,
	//   0x5c, 0x15, 0x52, 0x5a, 0xbf, 0x9a, 0x32, 0x63,
	//   0x6d, 0xeb, 0x2a, 0x65, 0x49, 0x9c, 0x80, 0xdc
	// };
	//
	//

	// LE SC Confirmation Value: dbf0a7c75a42e2580c0621c77d57fc5b
	// LE SC Random Value: 13e41edc4edc2fd11289f9b1e2bebba0

	struct bt_le_oob_sc_data oob_remote;
// 	const uint8_t rpi_c[16] = {0xdb, 0xf0, 0xa7, 0xc7, 0x5a, 0x42, 0xe2, 0x58,
// 				   0x0c, 0x06, 0x21, 0xc7, 0x7d, 0x57, 0xfc, 0x5b};
// 	memcpy(oob_remote.c, rpi_c, sizeof(oob_remote.c));
// 	const uint8_t rpi_r[16] = {0x13, 0xe4, 0x1e, 0xdc, 0x4e, 0xdc, 0x2f, 0xd1,
// 				   0x12, 0x89, 0xf9, 0xb1, 0xe2, 0xbe, 0xbb, 0xa0};
// 	memcpy(oob_remote.r, rpi_r, sizeof(oob_remote.r));


// [00:00:00.647,644] <dbg> app: main: Local OOB Randomizer R:
// 01 02 03 04 05 06 07 08  01 02 03 04 05 06 07 08 |........ ........
// [00:00:00.647,674] <dbg> app: main: Local OOB Hash C:
// c9 49 12 c1 22 3b 68 cd  d3 1a f4 6e ff 72 44 f8 |.I..";h. ...n.rD.

	const uint8_t hardcoded_c[16] = {0xc9, 0x49, 0x12, 0xc1, 0x22, 0x3b, 0x68, 0xcd,
					  0xd3, 0x1a, 0xf4, 0x6e, 0xff, 0x72, 0x44, 0xf8};
	memcpy(oob_remote.c, hardcoded_c, sizeof(oob_remote.c));
	const uint8_t hardcoded_r[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
					  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	memcpy(oob_remote.r, hardcoded_r, sizeof(oob_remote.r));

	// 	//     bt_le_oob_set_sc_data(conn, oob_data_local , NULL);
	// 	bt_le_oob_set_sc_data(conn, oob_data_local, &oob_remote);
	// 	//     bt_le_oob_set_sc_data(conn, &(oob_local.le_sc_data) , &oob_remote);
	// In this test, we are using only the local OOB data for both sides, because
	// central and peripheral are both using the same hardcoded OOB data
	bt_le_oob_set_sc_data(conn, oob_data_local, &oob_remote);
// 	bt_le_oob_set_sc_data(conn, oob_data_local, oob_data_local);
}
//

int main(void)
{
	int err;

#if defined(CONFIG_BT_SMP)
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		LOG_DBG("Failed to register authorization callbacks.");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		LOG_DBG("Failed to register authorization info callbacks.");
		return 0;
	}
#endif

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

#if defined(CONFIG_BT_SETTINGS)
	/* Initialize settings */
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_INF("Calling settings_load()");
		err = settings_load();
		if (err < 0) {
			LOG_ERR("Settings load failed (err %d)", err);
			return err;
		}
	}
#endif

	bt_le_oob_set_sc_flag(true);

	LOG_INF("Starting GATT write with certificate validation");
    bt_gatt_cb_register(&gatt_callbacks);
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	// this is a test to see the OOB data generated (required configs in prj.conf)
	// CONFIG_BT_TESTING=y
	// CONFIG_BT_OOB_DATA_FIXED=y (teh local OOB hash C is fixed)
	// CONFIG_BT_USE_DEBUG_KEYS=y (to use fixed debug keys, otherwise new keys are generated
	// each time)

	struct bt_le_oob oob_local;

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		LOG_ERR("Error while fetching local OOB data: %d", err);
	}

	LOG_HEXDUMP_DBG(oob_local.le_sc_data.r, sizeof(oob_local.le_sc_data.r),
			"Local OOB Randomizer R:");
	LOG_HEXDUMP_DBG(oob_local.le_sc_data.c, sizeof(oob_local.le_sc_data.c),
			"Local OOB Hash C:");
	//     [00:00:00.006,286] <dbg> app: main: Local OOB Randomizer R:
	//                               01 e1 36 be 8e 23 f4 01  02 03 04 05 06 07 08 01 |..6..#..
	//                               ........
	//     [00:00:00.006,317] <dbg> app: main: Local OOB Hash C:
	//                               02 03 04 05 06 07 08 c9  49 12 c1 22 3b 68 cd d3 |........
	//                               I..";h..
	/// END TEST

	LOG_INF("Waiting for connections...");

	for (;;) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
