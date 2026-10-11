/*
 * Pads over Bluetooth, through ESP-IDF's HID host (esp_hidh) on Bluedroid:
 * DS4 and DS5 over Bluetooth Classic, the Xbox pad over BLE. Also the
 * esp-ps-controller calls, ps_controller_*, over the first PlayStation pad.
 * ESP-IDF only; under stm-forge this file is empty.
 */
#include "gamecontroller_priv.h"

#ifdef ESP_PLATFORM

#if (CONFIG_GAMECONTROLLER_DS4 || CONFIG_GAMECONTROLLER_DS5) && \
    CONFIG_BT_CLASSIC_ENABLED && CONFIG_BT_HID_HOST_ENABLED
#define GC_CLASSIC 1
#else
#define GC_CLASSIC 0
#endif
#if CONFIG_GAMECONTROLLER_XBOX && CONFIG_BT_BLE_ENABLED
#define GC_BLE 1
#else
#define GC_BLE 0
#endif

#if CONFIG_BT_BLUEDROID_ENABLED && (GC_CLASSIC || GC_BLE)

#include <stdio.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_hidh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#if GC_CLASSIC
#include "esp_gap_bt_api.h"
#endif
#if GC_BLE
#include "esp_gap_ble_api.h"
#endif

#include "ds4.h"
#include "ds5.h"
#include "gamecontroller_bt.h"
#include "gamecontroller_log.h"
#include "ps_controller.h"
#include "xbox.h"

#ifndef CONFIG_GAMECONTROLLER_MAX_PADS
#define CONFIG_GAMECONTROLLER_MAX_PADS 1
#endif

#define SONY_VENDOR     0x054c  /* hid-ids.h:1194 */
#define SONY_PS4        0x05c4  /* hid-ids.h:1199-1200 */
#define SONY_PS4_2      0x09cc
#define SONY_PS5        0x0ce6  /* hid-ids.h:1202-1203 */
#define SONY_PS5_2      0x0df2

#define DEFAULT_SCAN_S  20

/* One connected pad. */
struct pad {
    struct gamecontroller_dev dev;
    esp_hidh_dev_t *hid;            /* NULL: the slot is free */
    struct gamecontroller_state state;
    ps_state_t ps;                  /* the esp-ps-controller view */
    struct xbox_parser xbox;
    /* what the outputs last set, so one call does not undo another */
    uint8_t strong, weak, red, green, blue;
    uint8_t ds5_seq;
    bool ds5_lightbar_ready;
};

static struct pad s_pads[CONFIG_GAMECONTROLLER_MAX_PADS];
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_stack_up;
#if GC_CLASSIC
static bool s_scan_playstation;
#endif
#if GC_BLE
static bool s_scan_xbox;
#endif

static const struct gamecontroller_dev s_log_dev = { .name = "bt" };

/* ------------------------------------------------------------ pads ---- */

static struct pad *pad_of(esp_hidh_dev_t *hid)
{
    for (int i = 0; i < CONFIG_GAMECONTROLLER_MAX_PADS; i++)
        if (s_pads[i].hid == hid)
            return &s_pads[i];
    return NULL;
}

unsigned int gamecontroller_bt_count(void)
{
    unsigned int n = 0;

    for (int i = 0; i < CONFIG_GAMECONTROLLER_MAX_PADS; i++)
        if (s_pads[i].hid && s_pads[i].state.connected)
            n++;
    return n;
}

struct gamecontroller_dev *gamecontroller_bt_pad(unsigned int index)
{
    if (index >= CONFIG_GAMECONTROLLER_MAX_PADS || !s_pads[index].hid ||
        !s_pads[index].state.connected)
        return NULL;
    return &s_pads[index].dev;
}

static int op_get_state(struct gamecontroller_dev *gc,
                        struct gamecontroller_state *state)
{
    struct pad *p = gc->priv;

    portENTER_CRITICAL(&s_lock);
    *state = p->state;
    portEXIT_CRITICAL(&s_lock);
    return 0;
}

static int send(struct pad *p, const uint8_t *report, size_t len)
{
    /* esp_hidh puts the report id in front itself (bt_hidh.c:783-790). */
    return gamecontroller_errno(esp_hidh_dev_output_set(p->hid, 0, report[0],
                                                        (uint8_t *)report + 1,
                                                        len - 1));
}

#if CONFIG_GAMECONTROLLER_DS4
static int ds4_send(struct pad *p)
{
    uint8_t out[DS4_OUTPUT_LEN];
    struct ds4_output o = {
        .strong = p->strong, .weak = p->weak,
        .red = p->red, .green = p->green, .blue = p->blue,
    };

    return send(p, out, ds4_build_output(&o, out));
}

static int ds4_rumble(struct gamecontroller_dev *gc, uint8_t strong, uint8_t weak)
{
    struct pad *p = gc->priv;

    p->strong = strong;
    p->weak = weak;
    return ds4_send(p);
}

static int ds4_lightbar(struct gamecontroller_dev *gc, uint8_t r, uint8_t g,
                        uint8_t b)
{
    struct pad *p = gc->priv;

    p->red = r;
    p->green = g;
    p->blue = b;
    return ds4_send(p);
}

static const struct gamecontroller_ops ds4_ops = {
    .get_state = op_get_state,
    .set_rumble = ds4_rumble,
    .set_lightbar = ds4_lightbar,
};
#endif

#if CONFIG_GAMECONTROLLER_DS5
static int ds5_send(struct pad *p, const struct ds5_output *o)
{
    uint8_t out[DS5_OUTPUT_LEN];
    size_t len = ds5_build_output(o, p->ds5_seq, out);

    p->ds5_seq = (uint8_t)((p->ds5_seq + 1) & 0x0F);
    return send(p, out, len);
}

/* The light bar plays a start-up animation until told otherwise
 * (hid-playstation.c:1532-1542): release it before the first light. */
static int ds5_lights_ready(struct pad *p)
{
    struct ds5_output o = { .lightbar_setup = true };
    int err;

    if (p->ds5_lightbar_ready)
        return 0;
    err = ds5_send(p, &o);
    if (!err)
        p->ds5_lightbar_ready = true;
    return err;
}

static int ds5_rumble(struct gamecontroller_dev *gc, uint8_t strong, uint8_t weak)
{
    struct ds5_output o = { .rumble = true, .strong = strong, .weak = weak };

    return ds5_send(gc->priv, &o);
}

static int ds5_lightbar(struct gamecontroller_dev *gc, uint8_t r, uint8_t g,
                        uint8_t b)
{
    struct ds5_output o = { .lightbar = true, .red = r, .green = g, .blue = b };
    int err = ds5_lights_ready(gc->priv);

    return err ? err : ds5_send(gc->priv, &o);
}

static int ds5_player_leds(struct gamecontroller_dev *gc, uint8_t mask)
{
    struct ds5_output o = { .player_leds = true, .player_mask = mask };
    int err = ds5_lights_ready(gc->priv);

    return err ? err : ds5_send(gc->priv, &o);
}

static const struct gamecontroller_ops ds5_ops = {
    .get_state = op_get_state,
    .set_rumble = ds5_rumble,
    .set_lightbar = ds5_lightbar,
    .set_player_leds = ds5_player_leds,
};
#endif

#if GC_BLE
static int xbox_rumble(struct gamecontroller_dev *gc, uint8_t strong, uint8_t weak)
{
    uint8_t out[XBOX_RUMBLE_LEN];

    return send(gc->priv, out, xbox_build_rumble(strong, weak, out));
}

static const struct gamecontroller_ops xbox_ops = {
    .get_state = op_get_state,
    .set_rumble = xbox_rumble,
};
#endif

/* Which pad this is: by USB ids when the stack has them, else by the name
 * a PlayStation pad gives over Classic (esp-ps-controller's names). */
static bool identify(esp_hidh_dev_t *hid, enum gamecontroller_kind *kind)
{
    uint16_t vid = esp_hidh_dev_vendor_id_get(hid);
    uint16_t pid = esp_hidh_dev_product_id_get(hid);
    const char *name = esp_hidh_dev_name_get(hid);

#if GC_BLE
    if (esp_hidh_dev_transport_get(hid) == ESP_HID_TRANSPORT_BLE) {
        *kind = GAMECONTROLLER_XBOX;
        return xbox_is_ble_pad(vid, pid);
    }
#endif
    if (vid == SONY_VENDOR && (pid == SONY_PS4 || pid == SONY_PS4_2)) {
        *kind = GAMECONTROLLER_DS4;
    } else if (vid == SONY_VENDOR && (pid == SONY_PS5 || pid == SONY_PS5_2)) {
        *kind = GAMECONTROLLER_DS5;
    } else if (name && !strcmp(name, "DualSense Wireless Controller")) {
        *kind = GAMECONTROLLER_DS5;
    } else if (name && !strcmp(name, "Wireless Controller")) {
        *kind = GAMECONTROLLER_DS4;
    } else {
        return false;
    }
#if !CONFIG_GAMECONTROLLER_DS4
    if (*kind == GAMECONTROLLER_DS4) return false;
#endif
#if !CONFIG_GAMECONTROLLER_DS5
    if (*kind == GAMECONTROLLER_DS5) return false;
#endif
    return true;
}

static void on_open(esp_hidh_dev_t *hid)
{
    enum gamecontroller_kind kind;
    struct pad *p;
#if CONFIG_GAMECONTROLLER_DS4 || CONFIG_GAMECONTROLLER_DS5
    uint8_t buf[64];
    size_t len = 0;
#endif

    if (!identify(hid, &kind)) {
        gamecontroller_warn(&s_log_dev, "not a pad this build knows; closing");
        esp_hidh_dev_close(hid);
        return;
    }
    p = pad_of(NULL);
    if (!p) {
        gamecontroller_warn(&s_log_dev, "all %d slots taken; closing",
                CONFIG_GAMECONTROLLER_MAX_PADS);
        esp_hidh_dev_close(hid);
        return;
    }

    const char *name = NULL;
    const struct gamecontroller_ops *ops = NULL;

    switch (kind) {
#if CONFIG_GAMECONTROLLER_DS4
    case GAMECONTROLLER_DS4:
        name = "ds4";
        ops = &ds4_ops;
        break;
#endif
#if CONFIG_GAMECONTROLLER_DS5
    case GAMECONTROLLER_DS5:
        name = "ds5";
        ops = &ds5_ops;
        break;
#endif
#if GC_BLE
    case GAMECONTROLLER_XBOX:
        name = "xbox";
        ops = &xbox_ops;
        break;
#endif
    default:
        esp_hidh_dev_close(hid);
        return;
    }

    /* Asked before the lock: esp_hidh takes a semaphore and may block
     * (esp_hidh.c:29-33, 456-463), which a spinlock forbids. */
    uint16_t pid = kind == GAMECONTROLLER_XBOX ? esp_hidh_dev_product_id_get(hid) : 0;

    /* The slot is cleared and set up under the lock, so op_get_state()
     * never copies a half-cleared one. Nothing in here may block. */
    portENTER_CRITICAL(&s_lock);
    memset(p, 0, sizeof(*p));
    p->dev.kind = kind;
    p->dev.priv = p;
    p->dev.name = name;
    p->dev.ops = ops;
    p->state.battery = GAMECONTROLLER_BATTERY_UNKNOWN;
#if GC_BLE
    if (kind == GAMECONTROLLER_XBOX)
        xbox_parser_init(&p->xbox, pid);
#else
    (void)pid;
#endif
    portEXIT_CRITICAL(&s_lock);

    /* The reads that turn on full reports (ds4_proto.h, ds5_proto.h);
     * outside the lock, as they wait on the radio. */
#if CONFIG_GAMECONTROLLER_DS4
    if (kind == GAMECONTROLLER_DS4) {
        esp_hidh_dev_feature_get(hid, 0, 0x02, sizeof(buf), buf, &len);
        esp_hidh_dev_feature_get(hid, 0, 0x05, sizeof(buf), buf, &len);
    }
#endif
#if CONFIG_GAMECONTROLLER_DS5
    if (kind == GAMECONTROLLER_DS5)
        esp_hidh_dev_feature_get(hid, 0, 0x05, sizeof(buf), buf, &len);
#endif

    portENTER_CRITICAL(&s_lock);
    p->hid = hid;
    p->state.connected = true;
    portEXIT_CRITICAL(&s_lock);
    {
        const char *bt_name = esp_hidh_dev_name_get(hid);

        gamecontroller_info(&p->dev, "connected: %s",
                            bt_name ? bt_name : "(no name)");
    }
}

static void on_input(esp_hidh_dev_t *hid, uint16_t report_id,
                     const uint8_t *data, size_t len)
{
    struct pad *p = pad_of(hid);
    struct gamecontroller_state s;
    ps_state_t ps;
    bool ok = false;

    if (!p || !hid)
        return;

    portENTER_CRITICAL(&s_lock);
    s = p->state;
    ps = p->ps;
    portEXIT_CRITICAL(&s_lock);

    switch (p->dev.kind) {
#if CONFIG_GAMECONTROLLER_DS4
    case GAMECONTROLLER_DS4:
        ok = ds4_parse((uint8_t)report_id, data, len, &s);
        break;
#endif
#if CONFIG_GAMECONTROLLER_DS5
    case GAMECONTROLLER_DS5:
        ok = ds5_parse((uint8_t)report_id, data, len, &s);
        break;
#endif
#if GC_BLE
    case GAMECONTROLLER_XBOX:
        ok = xbox_parse(&p->xbox, (uint8_t)report_id, data, len, &s);
        break;
#endif
    default:
        break;
    }
    if (!ok)
        return;
    /* The esp-ps-controller view, from the same report: only one this
     * library took, so a report with a bad CRC moves neither. */
    if (p->dev.kind != GAMECONTROLLER_XBOX)
        ps_parse((uint8_t)report_id, data, len, &ps);

    portENTER_CRITICAL(&s_lock);
    p->state = s;
    p->ps = ps;
    portEXIT_CRITICAL(&s_lock);
}

/* The pad is gone: its state reads as nothing pressed and not connected.
 * Cleared under the lock, which esp-ps-controller did not take here. */
static void on_close(esp_hidh_dev_t *hid)
{
    struct pad *p = pad_of(hid);

    if (!p || !hid)
        return;
    gamecontroller_info(&p->dev, "disconnected");
    portENTER_CRITICAL(&s_lock);
    memset(&p->state, 0, sizeof(p->state));
    memset(&p->ps, 0, sizeof(p->ps));
    p->state.battery = GAMECONTROLLER_BATTERY_UNKNOWN;
    p->hid = NULL;
    portEXIT_CRITICAL(&s_lock);
}

static void hidh_cb(void *arg, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_data_t *e = event_data;

    (void)arg;
    (void)base;
    switch ((esp_hidh_event_t)id) {
    case ESP_HIDH_OPEN_EVENT:
        if (e->open.status != ESP_OK) {
            gamecontroller_warn(&s_log_dev, "open failed: %s", esp_err_to_name(e->open.status));
            break;
        }
        on_open(e->open.dev);
        break;
    case ESP_HIDH_INPUT_EVENT:
        on_input(e->input.dev, e->input.report_id, e->input.data, e->input.length);
        break;
    case ESP_HIDH_CLOSE_EVENT:
        on_close(e->close.dev);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------ scan ---- */

#if GC_CLASSIC
/* A PlayStation pad in pairing mode by its whole name -- esp-ps-controller
 * matched "Wireless Controller" anywhere in it, which "Xbox Wireless
 * Controller" also contains. */
static bool eir_is_playstation(uint8_t *eir)
{
    uint8_t len = 0;
    uint8_t *name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
    char buf[ESP_BT_GAP_EIR_DATA_LEN + 1];

    if (!name)
        name = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
    if (!name || len == 0)
        return false;
    if (len > ESP_BT_GAP_EIR_DATA_LEN)
        len = ESP_BT_GAP_EIR_DATA_LEN;
    memcpy(buf, name, len);
    buf[len] = '\0';
    return !strcmp(buf, "Wireless Controller") ||
           !strcmp(buf, "DualSense Wireless Controller");
}

static void gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    if (event != ESP_BT_GAP_DISC_RES_EVT || !s_scan_playstation)
        return;

    for (int i = 0; i < param->disc_res.num_prop; i++) {
        if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR &&
            eir_is_playstation((uint8_t *)param->disc_res.prop[i].val)) {
            s_scan_playstation = false;
            esp_bt_gap_cancel_discovery();
            esp_hidh_dev_open(param->disc_res.bda, ESP_HID_TRANSPORT_BT, 0);
            return;
        }
    }
}
#endif

#if GC_BLE
static esp_ble_scan_params_t s_scan_params = {
    .scan_type          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_ENABLE,
};
static uint32_t s_ble_scan_s;

/* A gamepad by its advertised appearance; whether it is an Xbox pad is
 * settled by its ids once open (xbox_is_ble_pad). */
static bool adv_is_gamepad(struct ble_scan_result_evt_param *r)
{
    uint8_t len = 0;
    uint8_t *a = esp_ble_resolve_adv_data_by_type(r->ble_adv,
                                                  r->adv_data_len + r->scan_rsp_len,
                                                  ESP_BLE_AD_TYPE_APPEARANCE, &len);

    return a && len >= 2 && (a[0] | a[1] << 8) == ESP_BLE_APPEARANCE_HID_GAMEPAD;
}

static void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        esp_ble_gap_start_scanning(s_ble_scan_s);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT &&
            s_scan_xbox && adv_is_gamepad(&param->scan_rst)) {
            s_scan_xbox = false;
            esp_ble_gap_stop_scanning();
            esp_hidh_dev_open(param->scan_rst.bda, ESP_HID_TRANSPORT_BLE,
                              param->scan_rst.ble_addr_type);
        }
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* Just Works: the pad has no display or keys. */
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    default:
        break;
    }
}
#endif

/* ----------------------------------------------------------- stack ---- */

#define TRY(expr) do { esp_err_t _e = (expr); if (_e != ESP_OK) return _e; } while (0)

/*
 * Controller, Bluedroid, GAP, HID host -- each only if not up yet, and every
 * failure returned rather than aborting, as esp-ps-controller's
 * ESP_ERROR_CHECK did. Memory is released only for a mode this build can
 * never use: esp-ps-controller always released the BLE memory, after which
 * no Xbox pad could be added.
 */
static esp_err_t start_stack(void)
{
    if (s_stack_up)
        return ESP_OK;

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

#if !GC_BLE
        TRY(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
        cfg.mode = ESP_BT_MODE_CLASSIC_BT;
#elif !GC_CLASSIC && SOC_BT_CLASSIC_SUPPORTED
        TRY(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
        cfg.mode = ESP_BT_MODE_BLE;
#endif
        TRY(esp_bt_controller_init(&cfg));
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
#if GC_CLASSIC && GC_BLE
        TRY(esp_bt_controller_enable(ESP_BT_MODE_BTDM));
#elif GC_CLASSIC
        TRY(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));
#else
        TRY(esp_bt_controller_enable(ESP_BT_MODE_BLE));
#endif
    }
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED)
        TRY(esp_bluedroid_init());
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED)
        TRY(esp_bluedroid_enable());

#if GC_CLASSIC
    {
        esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;   /* Just Works */

        TRY(esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap)));
        TRY(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
        TRY(esp_bt_gap_register_callback(gap_cb));
    }
#endif
#if GC_BLE
    {
        esp_ble_auth_req_t auth = ESP_LE_AUTH_BOND;
        esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
        uint8_t keys = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

        TRY(esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, 1));
        TRY(esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, 1));
        TRY(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &keys, 1));
        TRY(esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &keys, 1));
        TRY(esp_ble_gap_register_callback(ble_gap_cb));
    }
#endif

    {
        esp_hidh_config_t hcfg = {
            .callback = hidh_cb,
            .event_stack_size = 4096,
            .callback_arg = NULL,
        };

        TRY(esp_hidh_init(&hcfg));
    }
    s_stack_up = true;
    return ESP_OK;
}

static bool any_connected(bool playstation, bool xbox)
{
    for (int i = 0; i < CONFIG_GAMECONTROLLER_MAX_PADS; i++) {
        struct pad *p = &s_pads[i];

        if (!p->hid || !p->state.connected)
            continue;
        if (xbox && p->dev.kind == GAMECONTROLLER_XBOX)
            return true;
        if (playstation && p->dev.kind != GAMECONTROLLER_XBOX)
            return true;
    }
    return false;
}

/* Until a pad of a kind asked for is connected, or seconds have passed --
 * counted on the clock, not in polls. */
static esp_err_t wait_connected(uint32_t seconds, bool playstation, bool xbox)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t limit = pdMS_TO_TICKS(seconds * 1000u) + 1;

    while (!any_connected(playstation, xbox)) {
        if ((TickType_t)(xTaskGetTickCount() - start) > limit)
            return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(50) ? pdMS_TO_TICKS(50) : 1);
    }
    return ESP_OK;
}

esp_err_t gamecontroller_bt_start(const gamecontroller_bt_config_t *cfg)
{
    uint32_t seconds;
    esp_err_t err;

    if (!cfg || (!cfg->playstation && !cfg->xbox))
        return ESP_ERR_INVALID_ARG;
    if ((cfg->playstation && !GC_CLASSIC) || (cfg->xbox && !GC_BLE))
        return ESP_ERR_NOT_SUPPORTED;
    seconds = cfg->scan_seconds ? cfg->scan_seconds : DEFAULT_SCAN_S;

    TRY(start_stack());

#if GC_CLASSIC
    if (cfg->playstation) {
        s_scan_playstation = true;
        /* inquiry length in 1.28 s units; 0x30 is the maximum */
        TRY(esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 0x30, 0));
    }
#endif
#if GC_BLE
    if (cfg->xbox) {
        s_scan_xbox = true;
        s_ble_scan_s = seconds;
        TRY(esp_ble_gap_set_scan_params(&s_scan_params));
    }
#endif

    err = wait_connected(seconds, cfg->playstation, cfg->xbox);

#if GC_CLASSIC
    if (cfg->playstation) {
        s_scan_playstation = false;
        esp_bt_gap_cancel_discovery();
    }
#endif
#if GC_BLE
    if (cfg->xbox) {
        s_scan_xbox = false;
        esp_ble_gap_stop_scanning();
    }
#endif
    return err;
}

/* -------------------------------------------- esp-ps-controller API ---- */

#if GC_CLASSIC
static struct pad *first_playstation(void)
{
    for (int i = 0; i < CONFIG_GAMECONTROLLER_MAX_PADS; i++)
        if (s_pads[i].hid && s_pads[i].state.connected &&
            s_pads[i].dev.kind != GAMECONTROLLER_XBOX)
            return &s_pads[i];
    return NULL;
}

esp_err_t ps_controller_start(uint32_t scan_seconds)
{
    gamecontroller_bt_config_t cfg = {
        .scan_seconds = scan_seconds,
        .playstation = true,
    };

    /* esp-ps-controller waited scan_seconds, 0 included. */
    if (scan_seconds == 0) {
        esp_err_t err = start_stack();

        if (err != ESP_OK)
            return err;
        return any_connected(true, false) ? ESP_OK : ESP_ERR_TIMEOUT;
    }
    return gamecontroller_bt_start(&cfg);
}

esp_err_t ps_controller_start_mac(const char *mac)
{
    unsigned v[6];
    esp_bd_addr_t bda;

    if (!mac || sscanf(mac, "%x:%x:%x:%x:%x:%x", &v[0], &v[1], &v[2], &v[3],
                       &v[4], &v[5]) != 6)
        return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < 6; i++)
        bda[i] = (uint8_t)v[i];

    TRY(start_stack());
    if (esp_hidh_dev_open(bda, ESP_HID_TRANSPORT_BT, 0) == NULL)
        return ESP_FAIL;
    return wait_connected(10, true, false);
}

bool ps_controller_connected(void)
{
    return first_playstation() != NULL;
}

void ps_controller_get(ps_state_t *out)
{
    struct pad *p;

    portENTER_CRITICAL(&s_lock);
    p = first_playstation();
    if (p)
        *out = p->ps;
    else
        memset(out, 0, sizeof(*out));
    portEXIT_CRITICAL(&s_lock);
}

struct gamecontroller_dev *ps_controller_dev(void)
{
    struct pad *p = first_playstation();

    return p ? &p->dev : NULL;
}
#endif /* GC_CLASSIC */

#endif /* CONFIG_BT_BLUEDROID_ENABLED && (GC_CLASSIC || GC_BLE) */
#endif /* ESP_PLATFORM */
