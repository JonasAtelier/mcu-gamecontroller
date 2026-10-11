#include <stdio.h>

#include "gamecontroller_log.h"

void gamecontroller_log_state(const struct gamecontroller_dev *gc,
			      const struct gamecontroller_state *s)
{
	char battery[8];

	if (s->battery == GAMECONTROLLER_BATTERY_UNKNOWN)
		snprintf(battery, sizeof(battery), "?");
	else
		snprintf(battery, sizeof(battery), "%u%%", (unsigned)s->battery);
	gamecontroller_info(gc, "L %6d %6d  R %6d %6d  LT %5u RT %5u  "
			    "buttons 0x%05lx  battery %s%s%s",
			    s->lx, s->ly, s->rx, s->ry, (unsigned)s->lt,
			    (unsigned)s->rt, (unsigned long)s->buttons, battery,
			    s->charging ? " charging" : "",
			    s->connected ? "" : "  (not connected)");
}
