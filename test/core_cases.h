/*
 * The core's cases -- the state read, the optional outputs, the log line --
 * shared by esp/test_core.c and stm/test_core.c. Each includes this after
 * providing:
 *
 *   LOG_PREFIX             what the platform's log puts before
 *                          "gamecontroller: "
 *   stub_log_last          the last log line
 */

static int fake_calls;
static int fake_ret;			/* fail the next op with this */
static uint8_t fake_a, fake_b, fake_c;

static int fake_get_state(struct gamecontroller_dev *gc,
			  struct gamecontroller_state *s)
{
	(void)gc;
	fake_calls++;
	if (fake_ret) {
		s->lx = 1234;	/* the op scribbles; the caller must not see it */
		return fake_ret;
	}
	memset(s, 0, sizeof(*s));
	s->lx = -5;
	s->rt = 65535;
	s->buttons = GAMECONTROLLER_BTN_SOUTH | GAMECONTROLLER_BTN_DPAD_UP;
	s->battery = 55;
	s->connected = true;
	return 0;
}

static int fake_rumble(struct gamecontroller_dev *gc, uint8_t strong,
		       uint8_t weak)
{
	(void)gc;
	fake_calls++;
	fake_a = strong;
	fake_b = weak;
	return fake_ret;
}

static int fake_lightbar(struct gamecontroller_dev *gc, uint8_t r, uint8_t g,
			 uint8_t b)
{
	(void)gc;
	fake_calls++;
	fake_a = r;
	fake_b = g;
	fake_c = b;
	return fake_ret;
}

static int fake_player_leds(struct gamecontroller_dev *gc, uint8_t mask)
{
	(void)gc;
	fake_calls++;
	fake_a = mask;
	return fake_ret;
}

static const struct gamecontroller_ops fake_ops = {
	.get_state = fake_get_state,
	.set_rumble = fake_rumble,
	.set_lightbar = fake_lightbar,
	.set_player_leds = fake_player_leds,
};

static const struct gamecontroller_ops fake_ops_input_only = {
	.get_state = fake_get_state,
};

static struct gamecontroller_dev fake_dev(void)
{
	struct gamecontroller_dev gc = {
		.name = "fake",
		.ops = &fake_ops,
		.kind = GAMECONTROLLER_DS5,
	};

	fake_calls = 0;
	fake_ret = 0;
	return gc;
}

static void test_state(void)
{
	struct gamecontroller_dev gc = fake_dev();
	struct gamecontroller_state s = { .lx = 99 };

	assert(gamecontroller_get_state(&gc, &s) == 0);
	assert(s.lx == -5 && s.rt == 65535 && s.connected);
	assert(gamecontroller_pressed(&s, GAMECONTROLLER_BTN_SOUTH));
	assert(!gamecontroller_pressed(&s, GAMECONTROLLER_BTN_EAST));

	/* A failed read leaves the caller's state alone. */
	fake_ret = -EIO;
	s.lx = 99;
	assert(gamecontroller_get_state(&gc, &s) == -EIO && s.lx == 99);
}

static void test_outputs(void)
{
	struct gamecontroller_dev gc = fake_dev();

	assert(gamecontroller_set_rumble(&gc, 200, 50) == 0);
	assert(fake_a == 200 && fake_b == 50);
	assert(gamecontroller_set_lightbar(&gc, 1, 2, 3) == 0);
	assert(fake_a == 1 && fake_b == 2 && fake_c == 3);
	assert(gamecontroller_set_player_leds(&gc, 0x04) == 0 && fake_a == 0x04);
	fake_ret = -ETIMEDOUT;
	assert(gamecontroller_set_rumble(&gc, 1, 1) == -ETIMEDOUT);

	/* A pad without an output says so, and nothing is called. */
	gc.ops = &fake_ops_input_only;
	fake_calls = 0;
	assert(gamecontroller_set_rumble(&gc, 1, 1) == -EOPNOTSUPP);
	assert(gamecontroller_set_lightbar(&gc, 1, 1, 1) == -EOPNOTSUPP);
	assert(gamecontroller_set_player_leds(&gc, 1) == -EOPNOTSUPP);
	assert(fake_calls == 0);
}

static void test_unbound(void)
{
	struct gamecontroller_dev gc = fake_dev();
	struct gamecontroller_ops no_state = fake_ops;
	struct gamecontroller_state s;

	assert(gamecontroller_get_state(NULL, &s) == -EINVAL);
	assert(gamecontroller_get_state(&gc, NULL) == -EINVAL);
	assert(gamecontroller_set_rumble(NULL, 1, 1) == -EINVAL);
	gc.ops = NULL;
	assert(gamecontroller_set_lightbar(&gc, 1, 1, 1) == -EINVAL);
	/* get_state is the one op every pad must have. */
	no_state.get_state = NULL;
	gc.ops = &no_state;
	assert(gamecontroller_set_player_leds(&gc, 1) == -EINVAL);
	assert(fake_calls == 0);
}

static void test_log(void)
{
	struct gamecontroller_dev gc = fake_dev();
	struct gamecontroller_state s;

	gamecontroller_err(&gc, "no answer (%d)", -110);
	assert(!strcmp(stub_log_last,
		       LOG_PREFIX("E") "gamecontroller: fake: no answer (-110)"));

	fake_get_state(&gc, &s);
	gamecontroller_log_state(&gc, &s);
	assert(!strcmp(stub_log_last, LOG_PREFIX("I")
		       "gamecontroller: fake: L     -5      0  R      0      0  "
		       "LT     0 RT 65535  buttons 0x02001  battery 55%"));
	s.battery = GAMECONTROLLER_BATTERY_UNKNOWN;
	s.charging = true;
	s.connected = false;
	gamecontroller_log_state(&gc, &s);
	assert(strstr(stub_log_last, "battery ? charging  (not connected)"));
}

static void run_core_cases(void)
{
	test_state();
	test_outputs();
	test_unbound();
	test_log();
}
