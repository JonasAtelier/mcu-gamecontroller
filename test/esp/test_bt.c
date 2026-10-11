/*
 * Host-side check of src/gamecontroller_bt.c, the Bluetooth glue, against
 * fakes of the Bluetooth controller, Bluedroid, Classic and BLE GAP and
 * the HID host: bring-up and its failures, scanning and which pads it
 * opens, the pads' reports through to the state and to the
 * esp-ps-controller view, the outputs, and a pad going away. Also the four
 * esp-ps-controller fixes, each with a check that fails without it.
 *
 * The reports are the layout-derived vectors of ../vectors.h; nothing here
 * has run on a pad.
 */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_hidh.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ds4.h"
#include "ds5.h"
#include "gamecontroller_bt.h"
#include "ps_controller.h"
#include "xbox.h"
#include "../vectors.h"

extern TickType_t stub_ticks, stub_tick_step;

/* ------------------------------------------------- the fake stack ---- */

static esp_bt_controller_status_t ctrl_status;
static esp_bluedroid_status_t bd_status;
static esp_err_t fail_bluedroid_init;
static int released;			/* esp_bt_mode_t bits released */
static int enabled_mode;

esp_err_t esp_bt_controller_mem_release(esp_bt_mode_t mode)
{
	released |= mode;
	return ESP_OK;
}

esp_err_t esp_bt_controller_init(esp_bt_controller_config_t *cfg)
{
	(void)cfg;
	ctrl_status = ESP_BT_CONTROLLER_STATUS_INITED;
	return ESP_OK;
}

esp_err_t esp_bt_controller_enable(esp_bt_mode_t mode)
{
	enabled_mode = mode;
	ctrl_status = ESP_BT_CONTROLLER_STATUS_ENABLED;
	return ESP_OK;
}

esp_bt_controller_status_t esp_bt_controller_get_status(void)
{
	return ctrl_status;
}

esp_bluedroid_status_t esp_bluedroid_get_status(void)
{
	return bd_status;
}

esp_err_t esp_bluedroid_init(void)
{
	if (fail_bluedroid_init)
		return fail_bluedroid_init;
	bd_status = ESP_BLUEDROID_STATUS_INITIALIZED;
	return ESP_OK;
}

esp_err_t esp_bluedroid_enable(void)
{
	bd_status = ESP_BLUEDROID_STATUS_ENABLED;
	return ESP_OK;
}

/* Classic GAP */
static esp_bt_gap_cb_t gap_cb;
static int discoveries, discovery_cancels;

/* EIR and advertising data share one format: length, type, data. */
static uint8_t *find_ad(uint8_t *p, size_t n, uint8_t type, uint8_t *len)
{
	size_t i = 0;

	while (i < n && p[i]) {
		if (p[i + 1] == type) {
			*len = (uint8_t)(p[i] - 1);
			return p + i + 2;
		}
		i += 1u + p[i];
	}
	return NULL;
}

uint8_t *esp_bt_gap_resolve_eir_data(uint8_t *eir, esp_bt_eir_type_t type,
				     uint8_t *length)
{
	return find_ad(eir, ESP_BT_GAP_EIR_DATA_LEN, (uint8_t)type, length);
}

esp_err_t esp_bt_gap_start_discovery(esp_bt_inq_mode_t mode, uint8_t len,
				     uint8_t num)
{
	(void)mode;
	(void)num;
	assert(len == 0x30);
	discoveries++;
	return ESP_OK;
}

esp_err_t esp_bt_gap_cancel_discovery(void)
{
	discovery_cancels++;
	return ESP_OK;
}

esp_err_t esp_bt_gap_set_security_param(esp_bt_sp_param_t t, void *v,
					uint8_t len)
{
	assert(t == ESP_BT_SP_IOCAP_MODE && len == 1 &&
	       *(esp_bt_io_cap_t *)v == ESP_BT_IO_CAP_NONE);
	return ESP_OK;
}

esp_err_t esp_bt_gap_set_scan_mode(esp_bt_connection_mode_t c,
				   esp_bt_discovery_mode_t d)
{
	(void)c;
	(void)d;
	return ESP_OK;
}

esp_err_t esp_bt_gap_register_callback(esp_bt_gap_cb_t cb)
{
	gap_cb = cb;
	return ESP_OK;
}

/* BLE GAP */
static esp_gap_ble_cb_t ble_cb;
static int scans, scan_stops, scan_params_set, sec_rsps;
static uint32_t scan_duration;

uint8_t *esp_ble_resolve_adv_data_by_type(uint8_t *adv, uint16_t n,
					  esp_ble_adv_data_type type,
					  uint8_t *length)
{
	return find_ad(adv, n, (uint8_t)type, length);
}

esp_err_t esp_ble_gap_set_scan_params(esp_ble_scan_params_t *p)
{
	assert(p->scan_type == BLE_SCAN_TYPE_ACTIVE);
	scan_params_set++;
	return ESP_OK;
}

esp_err_t esp_ble_gap_start_scanning(uint32_t duration)
{
	scans++;
	scan_duration = duration;
	return ESP_OK;
}

esp_err_t esp_ble_gap_stop_scanning(void)
{
	scan_stops++;
	return ESP_OK;
}

esp_err_t esp_ble_gap_security_rsp(esp_bd_addr_t bda, bool accept)
{
	(void)bda;
	assert(accept);
	sec_rsps++;
	return ESP_OK;
}

esp_err_t esp_ble_gap_set_security_param(esp_ble_sm_param_t t, void *v,
					 uint8_t len)
{
	(void)t;
	(void)v;
	assert(len == 1);
	return ESP_OK;
}

esp_err_t esp_ble_gap_register_callback(esp_gap_ble_cb_t cb)
{
	ble_cb = cb;
	return ESP_OK;
}

/* The HID host, and the pads it opens. */
struct esp_hidh_dev_s {
	const char *name;
	uint16_t vid, pid;
	esp_hid_transport_t transport;
	bool closed;
	uint8_t features[4];
	int nfeatures;
};

static esp_event_handler_t hidh_cb;
static struct esp_hidh_dev_s pads[4];
static int npads;
static esp_hid_transport_t opened_transport;
static uint8_t opened_addr_type;
static uint8_t out_id, out_data[DS5_OUTPUT_LEN];
static size_t out_len;
static int outputs;

esp_err_t esp_hidh_init(const esp_hidh_config_t *config)
{
	hidh_cb = config->callback;
	return ESP_OK;
}

/*
 * Every esp_hidh call may block on the HID host's semaphore (esp_hidh.c:
 * 29-33): none may come while the glue holds its spinlock.
 */
#define NOT_LOCKED() assert(!stub_critical)

esp_hidh_dev_t *esp_hidh_dev_open(esp_bd_addr_t bda, esp_hid_transport_t t,
				  uint8_t addr_type)
{
	NOT_LOCKED();
	(void)bda;
	opened_transport = t;
	opened_addr_type = addr_type;
	assert(npads < 4);
	pads[npads].transport = t;
	return &pads[npads++];
}

esp_err_t esp_hidh_dev_close(esp_hidh_dev_t *dev)
{
	NOT_LOCKED();
	dev->closed = true;
	return ESP_OK;
}

esp_err_t esp_hidh_dev_output_set(esp_hidh_dev_t *dev, size_t map,
				  size_t id, uint8_t *data, size_t len)
{
	NOT_LOCKED();
	assert(!dev->closed && map == 0 && len <= sizeof(out_data));
	out_id = (uint8_t)id;
	memcpy(out_data, data, len);
	out_len = len;
	outputs++;
	return ESP_OK;
}

esp_err_t esp_hidh_dev_feature_get(esp_hidh_dev_t *dev, size_t map,
				   size_t id, size_t max, uint8_t *data,
				   size_t *len)
{
	NOT_LOCKED();
	(void)map;
	(void)max;
	(void)data;
	assert(dev->nfeatures < 4);
	dev->features[dev->nfeatures++] = (uint8_t)id;
	*len = 0;
	return ESP_OK;
}

const char *esp_hidh_dev_name_get(esp_hidh_dev_t *dev)
{
	NOT_LOCKED();
	return dev->name;
}

uint16_t esp_hidh_dev_vendor_id_get(esp_hidh_dev_t *dev)
{
	NOT_LOCKED();
	return dev->vid;
}

uint16_t esp_hidh_dev_product_id_get(esp_hidh_dev_t *dev)
{
	NOT_LOCKED();
	return dev->pid;
}

esp_hid_transport_t esp_hidh_dev_transport_get(esp_hidh_dev_t *dev)
{
	NOT_LOCKED();
	return dev->transport;
}

/* ---- what the other side does while gamecontroller_bt_start() waits ---- */

static void (*on_wait)(void);

void vTaskDelay(TickType_t ticks)
{
	void (*f)(void) = on_wait;

	stub_ticks += ticks;
	on_wait = NULL;
	if (f)
		f();
}

static void hid_event(esp_hidh_event_t id, esp_hidh_event_data_t *e)
{
	hidh_cb(NULL, "ESP_HIDH_EVENTS", id, e);
}

static void open_event(esp_hidh_dev_t *dev)
{
	esp_hidh_event_data_t e = { .open = { .dev = dev, .status = ESP_OK } };

	hid_event(ESP_HIDH_OPEN_EVENT, &e);
}

static void input_event(esp_hidh_dev_t *dev, uint8_t id, const uint8_t *d,
			size_t n)
{
	esp_hidh_event_data_t e = { .input = {
		.dev = dev, .report_id = id, .length = (uint16_t)n,
		.data = (uint8_t *)d,
	} };

	hid_event(ESP_HIDH_INPUT_EVENT, &e);
}

static void close_event(esp_hidh_dev_t *dev)
{
	esp_hidh_event_data_t e = { .close = { .dev = dev } };

	hid_event(ESP_HIDH_CLOSE_EVENT, &e);
}

/* A Classic inquiry result carrying a name in its EIR. */
static void discovered(const char *name)
{
	uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN] = { 0 };
	size_t n = strlen(name);
	esp_bt_gap_dev_prop_t prop = {
		.type = ESP_BT_GAP_DEV_PROP_EIR, .len = (int)(n + 2), .val = eir,
	};
	esp_bt_gap_cb_param_t p = { .disc_res = { .num_prop = 1, .prop = &prop } };

	eir[0] = (uint8_t)(n + 1);
	eir[1] = ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME;
	memcpy(eir + 2, name, n);
	gap_cb(ESP_BT_GAP_DISC_RES_EVT, &p);
}

/* A BLE advertiser, with an appearance or without. */
static void advertised(uint16_t appearance)
{
	esp_ble_gap_cb_param_t p = { .scan_rst = {
		.search_evt = ESP_GAP_SEARCH_INQ_RES_EVT,
		.ble_addr_type = BLE_ADDR_TYPE_RANDOM,
		.adv_data_len = 4,
	} };

	p.scan_rst.ble_adv[0] = 3;
	p.scan_rst.ble_adv[1] = appearance ? ESP_BLE_AD_TYPE_APPEARANCE : 0x01;
	p.scan_rst.ble_adv[2] = (uint8_t)appearance;
	p.scan_rst.ble_adv[3] = (uint8_t)(appearance >> 8);
	ble_cb(ESP_GAP_BLE_SCAN_RESULT_EVT, &p);
}

/* ------------------------------------------------------------- cases ---- */

static void ps4_appears(void)
{
	int before = npads;

	/* esp-ps-controller opened anything whose name held "Wireless
	 * Controller" -- an Xbox pad too. */
	discovered("Xbox Wireless Controller");
	assert(npads == before);
	discovered("Wireless Controller");
	assert(npads == before + 1 && opened_transport == ESP_HID_TRANSPORT_BT);
	pads[before].name = "Wireless Controller";
	pads[before].vid = 0x054c;
	pads[before].pid = 0x05c4;
	open_event(&pads[before]);
}

/* Bring-up fails: the error comes back, nothing aborts (esp-ps-controller
 * wrapped every step in ESP_ERROR_CHECK). */
static void test_bringup_failure(void)
{
	gamecontroller_bt_config_t cfg = { .scan_seconds = 1, .playstation = true };

	fail_bluedroid_init = ESP_FAIL;
	assert(gamecontroller_bt_start(&cfg) == ESP_FAIL);
	assert(discoveries == 0);
	fail_bluedroid_init = ESP_OK;
}

static void test_arguments(void)
{
	gamecontroller_bt_config_t cfg = { 0 };

	assert(gamecontroller_bt_start(NULL) == ESP_ERR_INVALID_ARG);
	assert(gamecontroller_bt_start(&cfg) == ESP_ERR_INVALID_ARG);
	assert(gamecontroller_bt_pad(0) == NULL && gamecontroller_bt_pad(9) == NULL);
	assert(gamecontroller_bt_count() == 0);
}

static void test_playstation(void)
{
	gamecontroller_bt_config_t cfg = { .scan_seconds = 5, .playstation = true };
	struct gamecontroller_state s;
	struct gamecontroller_dev *gc;
	esp_hidh_dev_t *dev = &pads[npads];
	uint8_t bad[sizeof(ds4_full)];
	ps_state_t ps, want = { 0 };
	int entries;

	on_wait = ps4_appears;
	assert(gamecontroller_bt_start(&cfg) == ESP_OK);
	/* Both stacks up together: nothing released, dual mode (U2). */
	assert(released == 0 && enabled_mode == ESP_BT_MODE_BTDM);
	assert(discoveries == 1 && discovery_cancels >= 1);
	/* The reads that switch a DS4 to full reports. */
	assert(dev->nfeatures == 2 && dev->features[0] == 0x02 &&
	       dev->features[1] == 0x05);

	gc = gamecontroller_bt_pad(0);
	assert(gc && gc->kind == GAMECONTROLLER_DS4 && !strcmp(gc->name, "ds4"));
	assert(gamecontroller_bt_count() == 1 && ps_controller_connected());
	assert(ps_controller_dev() == gc);

	/* A report: the state and the esp-ps-controller view. */
	input_event(dev, 0x11, ds4_full, sizeof(ds4_full));
	assert(gamecontroller_get_state(gc, &s) == 0);
	assert(s.connected && s.lx == -32768 && s.battery == 55 && s.seq == 1);
	assert(ps_parse(0x11, ds4_full, sizeof(ds4_full), &want));
	ps_controller_get(&ps);
	assert(!memcmp(&ps, &want, sizeof(ps)) && ps.lx == -128 && ps.cross);

	/* A bad CRC moves neither. */
	memcpy(bad, ds4_full, sizeof(bad));
	bad[2] = 0x80;
	input_event(dev, 0x11, bad, sizeof(bad));
	assert(gamecontroller_get_state(gc, &s) == 0 && s.lx == -32768 && s.seq == 1);
	ps_controller_get(&ps);
	assert(ps.lx == -128);

	/* Outputs: light bar, then rumble -- one report with both, as
	 * hid-playstation sends them. */
	assert(gamecontroller_set_lightbar(gc, 1, 2, 3) == 0);
	assert(gamecontroller_set_rumble(gc, 0xC8, 0x32) == 0);
	assert(out_id == 0x11 && out_len == DS4_OUTPUT_LEN - 1);
	assert(!memcmp(out_data, ds4_out + 1, out_len));
	assert(gamecontroller_set_player_leds(gc, 1) == -EOPNOTSUPP);

	/* Gone: cleared under the lock (U4), the slot free. */
	entries = stub_critical_entries;
	close_event(dev);
	assert(stub_critical_entries > entries);
	assert(gamecontroller_bt_pad(0) == NULL && !ps_controller_connected());
	assert(ps_controller_dev() == NULL);
	ps_controller_get(&ps);
	memset(&want, 0, sizeof(want));
	assert(!memcmp(&ps, &want, sizeof(ps)));
	assert(gamecontroller_get_state(gc, &s) == 0 && !s.connected && s.lx == 0);
}

static void ds5_appears(void)
{
	discovered("DualSense Wireless Controller");
	/* No USB ids from the stack this time: the name decides. */
	pads[npads - 1].name = "DualSense Wireless Controller";
	open_event(&pads[npads - 1]);
}

static void test_dualsense(void)
{
	gamecontroller_bt_config_t cfg = { .scan_seconds = 5, .playstation = true };
	struct gamecontroller_dev *gc;
	esp_hidh_dev_t *dev = &pads[npads];
	uint8_t want[DS5_OUTPUT_LEN];
	struct ds5_output lb = { .lightbar = true, .red = 4, .green = 5, .blue = 6 };
	int before;

	on_wait = ds5_appears;
	assert(gamecontroller_bt_start(&cfg) == ESP_OK);
	assert(dev->nfeatures == 1 && dev->features[0] == 0x05);
	gc = gamecontroller_bt_pad(0);
	assert(gc && gc->kind == GAMECONTROLLER_DS5);

	input_event(dev, 0x31, ds5_full, sizeof(ds5_full));
	assert(ps_controller_connected());

	/* The first light releases the start-up animation, seq 0, then sets
	 * the colour, seq 1; later ones only set it. */
	before = outputs;
	assert(gamecontroller_set_lightbar(gc, 4, 5, 6) == 0);
	assert(outputs == before + 2);
	ds5_build_output(&lb, 1, want);
	assert(out_id == 0x31 && !memcmp(out_data, want + 1, out_len));
	assert(gamecontroller_set_player_leds(gc, 0x04) == 0);
	assert(outputs == before + 3);
	assert(out_data[46 - 1] == 0x04 && (out_data[0] >> 4) == 2);
	close_event(dev);
}

static void xbox_appears(void)
{
	esp_ble_gap_cb_param_t none = { 0 };

	ble_cb(ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT, &none);
	assert(scans == 1 && scan_duration == 5);
	advertised(0x03C1);		/* a keyboard: left alone */
	advertised(0);
	assert(opened_transport != ESP_HID_TRANSPORT_BLE);
	advertised(ESP_BLE_APPEARANCE_HID_GAMEPAD);
	assert(opened_transport == ESP_HID_TRANSPORT_BLE &&
	       opened_addr_type == BLE_ADDR_TYPE_RANDOM);
	ble_cb(ESP_GAP_BLE_SEC_REQ_EVT, &none);
	assert(sec_rsps == 1);
	pads[npads - 1].vid = 0x045e;
	pads[npads - 1].pid = 0x0b13;
	open_event(&pads[npads - 1]);
}

static void test_xbox(void)
{
	gamecontroller_bt_config_t cfg = { .scan_seconds = 5, .xbox = true };
	struct gamecontroller_state s;
	struct gamecontroller_dev *gc;
	esp_hidh_dev_t *dev = &pads[npads];
	uint8_t state[16] = { 0, 0x80, 0, 0x80, 0, 0x80, 0, 0x80,
			      0, 0, 0, 0, 0, 0x01, 0, 0x01 };
	uint8_t rumble[XBOX_RUMBLE_LEN];

	on_wait = xbox_appears;
	assert(gamecontroller_bt_start(&cfg) == ESP_OK);
	assert(scan_params_set == 1 && scan_stops >= 1);
	gc = gamecontroller_bt_pad(0);
	assert(gc && gc->kind == GAMECONTROLLER_XBOX);
	assert(!ps_controller_connected());

	input_event(dev, 0x01, state, sizeof(state));
	assert(gamecontroller_get_state(gc, &s) == 0);
	assert(s.buttons == (GAMECONTROLLER_BTN_SOUTH | GAMECONTROLLER_BTN_CAPTURE));

	assert(gamecontroller_set_rumble(gc, 255, 0) == 0);
	xbox_build_rumble(255, 0, rumble);
	assert(out_id == 0x03 && out_len == 8 && !memcmp(out_data, rumble + 1, 8));
	assert(gamecontroller_set_lightbar(gc, 1, 1, 1) == -EOPNOTSUPP);
	close_event(dev);
}

/* A BLE gamepad that is not an Xbox pad, and a pad with every slot full,
 * are closed again. */
static void test_refused(void)
{
	esp_hidh_dev_t *other = &pads[0], *a = &pads[1], *b = &pads[2];
	esp_hidh_dev_t *c = &pads[3];

	memset(pads, 0, sizeof(pads));
	npads = 4;
	other->transport = ESP_HID_TRANSPORT_BLE;
	other->vid = 0x1234;
	open_event(other);
	assert(other->closed && gamecontroller_bt_count() == 0);

	a->name = b->name = c->name = "Wireless Controller";
	open_event(a);
	open_event(b);
	assert(gamecontroller_bt_count() == 2);
	open_event(c);			/* CONFIG_GAMECONTROLLER_MAX_PADS = 2 */
	assert(c->closed && gamecontroller_bt_count() == 2);
	close_event(a);
	close_event(b);
	assert(gamecontroller_bt_count() == 0);
}

/* Nothing in pairing mode: ESP_ERR_TIMEOUT once the time is up, on the
 * clock. */
static void test_timeout(void)
{
	gamecontroller_bt_config_t cfg = { .scan_seconds = 2, .playstation = true };
	TickType_t start;

	stub_tick_step = 0;
	start = stub_ticks;
	assert(gamecontroller_bt_start(&cfg) == ESP_ERR_TIMEOUT);
	assert(stub_ticks - start >= 2 * 100);	/* 100 Hz tick */
	stub_tick_step = 1;
}

static void mac_opened(void)
{
	pads[0].name = "Wireless Controller";
	open_event(&pads[0]);
}

static void test_ps_controller_api(void)
{
	memset(pads, 0, sizeof(pads));
	npads = 0;
	assert(ps_controller_start_mac("not a mac") == ESP_ERR_INVALID_ARG);
	assert(ps_controller_start_mac(NULL) == ESP_ERR_INVALID_ARG);
	on_wait = mac_opened;
	assert(ps_controller_start_mac("aa:bb:cc:dd:ee:ff") == ESP_OK);
	assert(opened_transport == ESP_HID_TRANSPORT_BT && ps_controller_connected());
	close_event(&pads[0]);
	assert(!ps_controller_connected());
}

int main(void)
{
	test_arguments();
	test_bringup_failure();
	test_playstation();
	test_dualsense();
	test_xbox();
	test_refused();
	test_timeout();
	test_ps_controller_api();
	printf("all Bluetooth glue checks passed\n");
	return 0;
}
