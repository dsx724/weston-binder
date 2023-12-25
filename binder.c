/*
 * Copyright © 2018 Tarvi Verro
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include  <libweston/libweston.h>
#include <ctype.h>
#include <libevdev/libevdev.h>
#include <string.h>
#include <weston.h>
#include <unistd.h>
#include <fcntl.h>
#include <libweston/libweston.h>
#include <libweston/version.h>

/**
 * Parses a key combo to keycode (linux/input-event-codes.h KEY_%) and modifier
 * (libweston/compositor.h: enum weston_keyboard_modifier).
 *
 * Returns -1 on failure, 0 on success.
 */
static int
binder_parse_combination(const char *combo, uint32_t *key,
enum weston_keyboard_modifier *mod)
{
	uint32_t k = 0;
	enum weston_keyboard_modifier m = 0;

	/*
	 * The maximum length of a key name is 28:
	 *
	 *	grep -o 'KEY_\S\+' linux/input-event-codes.h | wc -L
	 */
	char keysym[29];
	int code;

	const char *i = combo;
	while (*i) {
		if (*i == '+')
			i++;

		const char *start = i;
		for (; *i && *i != '+'; i++);
		if (i - start > sizeof(keysym) - sizeof("KEY_"))
			return -1;

		strcpy(keysym, "KEY_");
		memcpy(keysym + 4, start, i - start);
		keysym[4 + i - start] = '\0';

		for (int j = 4; j < i - start + 4; j++)
			keysym[j] = toupper(keysym[j]);

		if (!strcmp("CTRL", keysym + 4)) {
			m |= MODIFIER_CTRL;
		} else if (!strcmp("ALT", keysym + 4)) {
			m |= MODIFIER_ALT;
		} else if (!strcmp("SUPER", keysym + 4)) {
			m |= MODIFIER_SUPER;
		} else if ((code = libevdev_event_code_from_name(EV_KEY, keysym))) {
			if (code == -1 || k != 0)
				return -1;
			k = code;
		} else  {
			return -1;
		}
	}

	*key = k;
	*mod = m;
	return 0;
}


struct binder_data {
	char *exec;
	struct weston_compositor *ec;
};

struct binder_process {
	struct wet_process wp;
	struct binder_data *data;
};

static void
binder_callback(struct weston_keyboard *keyboard, const struct timespec *time,
		uint32_t key, void *data)
{
	struct binder_data *bd = (struct binder_data *) data;	
	char *exe = malloc(strlen(bd->exec) + 9);
	strcpy(exe,"/bin/env ");
	strcpy(exe+9,bd->exec);
	struct wl_client *client=wet_client_start(bd->ec,exe);
	if(!client){
		weston_log("Failed starting process %s\n", (char *) bd->exec);
	}
	return;
}

static void
binder_add_bindings(struct weston_compositor *ec)
{
	struct weston_config *config =  wet_get_config(ec);
	struct weston_config_section *s = NULL;
	const char *name = NULL;
	char *exec, *key;

	while (weston_config_next_section(config, &s, &name)) {
		if (strcmp(name, "keybind"))
			continue;

		weston_config_section_get_string(s, "key", &key, NULL);
		weston_config_section_get_string(s, "exec", &exec, NULL);

		if (!key || !exec) {
			free(key);
			free(exec);

			weston_log("Ignoring incomplete keybind section.\n");
			continue;
		}

		uint32_t k;
		enum weston_keyboard_modifier m;
		int code = binder_parse_combination(key, &k, &m);

		if (code == -1) {
			weston_log("Invalid key for keybind: '%s'.", key);
			continue;
		}

		struct binder_data *bd = malloc(sizeof(*bd));
		bd->exec = exec;
		bd->ec = ec;
		weston_log("Adding keybind %s -> %s\n", key, exec);
		weston_compositor_add_key_binding(ec, k, m, binder_callback, bd);
		free(key);
	}
}

WL_EXPORT int
wet_module_init(struct weston_compositor *ec, int *argc, char *argv[])
{
	binder_add_bindings(ec);
	return 0;
}
