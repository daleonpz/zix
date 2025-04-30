/*
 * Copyright (c) 2024 Circuit Dojo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
// #include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otau_smp_sample);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

static void start_adv(void);

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* custom service */
#define BT_UUID_CUSTOM_SERVICE_VAL \
        BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

static const struct bt_uuid_128 primary_service_uuid = BT_UUID_INIT_128(
            BT_UUID_CUSTOM_SERVICE_VAL);

static const struct bt_uuid_128 read_characteristic_uuid = BT_UUID_INIT_128(
            BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

static const struct bt_uuid_128 write_characteristic_uuid = BT_UUID_INIT_128(
            BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2));

static int signed_value;
static struct bt_le_adv_param adv_param;
static int bond_count;

static ssize_t read_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
        void *buf, uint16_t len, uint16_t offset)
{
    int *value = &signed_value;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
            sizeof(signed_value));
}

static ssize_t write_signed(struct bt_conn *conn, const struct bt_gatt_attr *attr,
        const void *buf, uint16_t len, uint16_t offset,
        uint8_t flags)
{
    int *value = &signed_value;

    if (offset + len > sizeof(signed_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    memcpy(value + offset, buf, len);

    LOG_DBG("Write signed value: %d", *value);

    return len;
}

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(primary_service,
        BT_GATT_PRIMARY_SERVICE(&primary_service_uuid),
        BT_GATT_CHARACTERISTIC(&read_characteristic_uuid.uuid,
            BT_GATT_CHRC_READ,
            BT_GATT_PERM_READ_AUTHEN,
            read_signed, NULL, NULL),
        BT_GATT_CHARACTERISTIC(&write_characteristic_uuid.uuid,
            BT_GATT_CHRC_WRITE,
            BT_GATT_PERM_WRITE_AUTHEN,
            NULL, write_signed, NULL),
        );

/* Register advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//     BT_DATA_BYTES(BT_DATA_UUID128_ALL, SMP_BT_SVC_UUID_VAL),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};


/**********************************************
 * Callbacks BLE 
 *********************************************/
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Pairing failed conn: %s, reason %d %s", addr, reason,
            bt_security_err_to_str(reason));
}

static void pairing_confirm(struct bt_conn *conn)
{
    int err = -1;

    err = bt_conn_auth_pairing_confirm(conn);
    if (err) {
        LOG_ERR("Pairing confirm failed (err %d)", err);
        return;
    }

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing confirmed: %s", addr);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                         enum bt_security_err err)
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

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
    } else {
        LOG_INF("Connected");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
    start_adv();
}

static void start_adv(void)
{
    int err;

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }

    LOG_INF("Advertising successfully started");
}


static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
    .pairing_confirm = pairing_confirm,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};


// BT_GATT_SERVICE_DEFINE(smp_service,
//     BT_GATT_PRIMARY_SERVICE(SMP_BT_SVC_UUID),
//     BT_GATT_CHARACTERISTIC(SMP_BT_CHR_UUID,
//                            BT_GATT_CHRC_WRITE | BT_GATT_CHRC_READ,
//                            BT_GATT_PERM_WRITE_AUTHEN | BT_GATT_PERM_READ_AUTHEN,
//                            NULL, NULL, NULL),
// );

int main(void)
{
    int ret;
    int err;

    LOG_DBG("Board: %s", CONFIG_BOARD);
    LOG_DBG(">>> Revision: %s", VERSION_STRING);
    LOG_DBG("Build time: " __DATE__ " " __TIME__ "");

    if (!gpio_is_ready_dt(&led))
    {
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0)
    {
        return -ENODEV;
    }
    
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

    /* Enable Bluetooth */
    ret = bt_enable(NULL);
    if (ret < 0)
    {
        LOG_ERR("Bluetooth init failed (err %d)", ret);
        return ret;
    }

    /* Initialize settings */
    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        ret = settings_load();
        if (ret < 0)
        {
            LOG_ERR("Settings load failed (err %d)", ret);
            return ret;
        }
    }

    // I could add a button to reset the bond
    err= bt_unpair(BT_ID_DEFAULT,BT_ADDR_LE_ANY);
    if (err) {
        LOG_INF("Cannot delete bond (err: %d)", err);
    } else  {
        LOG_INF("Bond deleted succesfully");
    }

    /* Start advertising */
    start_adv();
    while (1)
    {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0)
        {
            return -ENODEV;
        }
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
