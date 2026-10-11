#include <errno.h>

#include "gamecontroller.h"

static bool gamecontroller_bound(const struct gamecontroller_dev *gc)
{
	return gc && gc->ops && gc->ops->get_state;
}

/* Into scratch first, so a failed read leaves *state alone. */
int gamecontroller_get_state(struct gamecontroller_dev *gc,
			     struct gamecontroller_state *state)
{
	struct gamecontroller_state s;
	int ret;

	if (!gamecontroller_bound(gc) || !state)
		return -EINVAL;

	ret = gc->ops->get_state(gc, &s);
	if (ret)
		return ret;
	*state = s;
	return 0;
}

int gamecontroller_set_rumble(struct gamecontroller_dev *gc, uint8_t strong,
			      uint8_t weak)
{
	if (!gamecontroller_bound(gc))
		return -EINVAL;
	if (!gc->ops->set_rumble)
		return -EOPNOTSUPP;

	return gc->ops->set_rumble(gc, strong, weak);
}

int gamecontroller_set_lightbar(struct gamecontroller_dev *gc, uint8_t red,
				uint8_t green, uint8_t blue)
{
	if (!gamecontroller_bound(gc))
		return -EINVAL;
	if (!gc->ops->set_lightbar)
		return -EOPNOTSUPP;

	return gc->ops->set_lightbar(gc, red, green, blue);
}

int gamecontroller_set_player_leds(struct gamecontroller_dev *gc,
				   uint8_t mask)
{
	if (!gamecontroller_bound(gc))
		return -EINVAL;
	if (!gc->ops->set_player_leds)
		return -EOPNOTSUPP;

	return gc->ops->set_player_leds(gc, mask);
}
