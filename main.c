/* Copyright 2019-2020 Corey Hinshaw

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */


#include <bsd/libutil.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "version.h"
#ifdef ELOGIND
#include <elogind/sd-bus.h>
#else
#include <systemd/sd-bus.h>
#endif
#ifdef WAYLAND
#include <wayland-client.h>
#include "idle-inhibit-unstable-v1-client-protocol.h"
#endif

#define PROGNAME "caffeinated"

static char daemonize = 0;
static char *pid_path = NULL;
static struct pidfh *pfh = NULL;
static int lock_fd = -1;
static struct sd_bus *bus = NULL;

#ifdef WAYLAND
static struct wl_compositor *compositor = NULL;
static struct wl_surface *surface = NULL;
static struct wl_display *display = NULL;
static struct zwp_idle_inhibit_manager_v1 *wl_idle_inhibit_manager = NULL;
static struct zwp_idle_inhibitor_v1 *wl_idle_inhibitor = NULL;

static void handle_wl_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name,
			&wl_compositor_interface, 1);
	} else if (strcmp(interface, zwp_idle_inhibit_manager_v1_interface.name) == 0) {
		wl_idle_inhibit_manager = wl_registry_bind(registry, name,
			&zwp_idle_inhibit_manager_v1_interface, 1);
	}
}

static void handle_wl_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	// No op
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_wl_global,
	.global_remove = handle_wl_global_remove,
};

static void init_wl_surface(void) {
	display = wl_display_connect(NULL);
	if (display == NULL) {
		errx(EXIT_FAILURE, "failed to create wl_display");
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);

	if (wl_idle_inhibit_manager == NULL || compositor == NULL) {
		errx(EXIT_FAILURE, "no wl_idle_inhibit_manager or wl_compositor support");
	}

	surface = wl_compositor_create_surface(compositor);
}
#endif

static void acquire_lock(void) {
	sd_bus_message *msg = NULL;
	sd_bus_error error = SD_BUS_ERROR_NULL;

	int ret = sd_bus_call_method(bus, "org.freedesktop.login1",
			"/org/freedesktop/login1", "org.freedesktop.login1.Manager",
			"Inhibit", &error, &msg, "ssss",
			"idle", PROGNAME, "Prevent system idle", "block");
	if (ret < 0) {
		warnx("Failed to send idle inhibit signal: %s", error.message);
		goto cleanup;
	}

	ret = sd_bus_message_read(msg, "h", &lock_fd);
	if (ret < 0) {
		warnx("Failed to parse D-Bus response for idle inhibit");
		goto cleanup;
	}

	lock_fd = fcntl(lock_fd, F_DUPFD_CLOEXEC, 3);
	if (lock_fd < 0) {
		ret = -errno;
		warn("Failed to copy lock file descriptor");
	}

#ifdef WAYLAND
	if (wl_idle_inhibitor == NULL) {
		wl_idle_inhibitor =
			zwp_idle_inhibit_manager_v1_create_inhibitor(wl_idle_inhibit_manager, surface);
		wl_display_roundtrip(display);
	}
#endif

cleanup:
	sd_bus_error_free(&error);
	sd_bus_message_unref(msg);
	if (ret < 0) {
		exit(-ret);
	}
}

static void release_lock(void) {
	if (lock_fd >= 0) {
		close(lock_fd);
	}
	lock_fd = -1;

#ifdef WAYLAND
	if (wl_idle_inhibitor) {
		zwp_idle_inhibitor_v1_destroy(wl_idle_inhibitor);
		wl_idle_inhibitor = NULL;
		wl_display_roundtrip(display);
	}
#endif
}

static void connect_to_bus(void) {
	int ret = sd_bus_default_system(&bus);
	if (sd_bus_default_system(&bus) < 0) {
		errx(-ret, "Failed to open D-Bus connection");
	}
}

static void cleanup(void) {
	release_lock();
	pidfile_remove(pfh);

#ifdef WAYLAND
	wl_surface_destroy(surface);
#endif
}

void handle_signal(int sig) {
	switch (sig) {
		case SIGUSR1:
			if (lock_fd >= 0) {
				release_lock();
			} else {
				acquire_lock();
			}
			break;
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			cleanup();
			signal(sig, SIG_DFL);
			raise(sig);
			break;
	}
}

static void print_version(void) {
	printf("%s %s\n", PROGNAME, VERSION);
}

static void print_help(void) {
	printf("Usage: %s [OPTIONS]\n\
Prevents system idle.\n\
\n\
Options:\n\
    -h          print this help message\n\
    -v          print program version information\n\
    -d          run as background daemon\n\
    -p PATH     create pidfile at PATH\n\
", PROGNAME);
}

static void set_pid_path(void) {
	char *dir = getenv("XDG_RUNTIME_DIR");

	if (geteuid() == 0) {
		pid_path = "/var/run/" PROGNAME ".pid";
	} else if (dir != NULL) {
		pid_path = malloc(strlen(dir) + strlen("/" PROGNAME ".pid") + 1);
    strcpy(pid_path, dir);
    strcat(pid_path, "/" PROGNAME ".pid");
	}
}

static void parse_args(int argc, char *argv[]) {
	char c;
	while ((c = getopt(argc, argv, ":hvdp:")) != -1) {
		switch (c) {
			case 'h':
				print_help();
				exit(EXIT_SUCCESS);
			case 'v':
				print_version();
				exit(EXIT_SUCCESS);
			case 'd':
				daemonize = 1;
				break;
			case 'p':
				pid_path = optarg;
				break;
			case '?':
				errx(EXIT_FAILURE, "Unknown option `-%c'.", optopt);
			case ':':
				errx(EXIT_FAILURE, "Option -%c requires an argument.", optopt);
		}
	}
}

int main(int argc, char *argv[]) {
	set_pid_path();
	parse_args(argc, argv);

	if (pid_path != NULL) {
		pfh = pidfile_open(pid_path, 0644, NULL);
		if (pfh == NULL) {
			if (errno == EEXIST) {
				errx(EXIT_FAILURE, "process is already running");
			}
			warn("Cannot open or create pidfile");
		}
	}

	if (daemonize && daemon(1, 0) < 0) {
		warn("Cannot daemonize");
		pidfile_remove(pfh);
		exit(EXIT_FAILURE);
	}

	atexit(cleanup);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGHUP, handle_signal);
	signal(SIGUSR1, handle_signal);

	pidfile_write(pfh);
	connect_to_bus();
#ifdef WAYLAND
	init_wl_surface();
#endif
	acquire_lock();

	for (;;) {
		pause();
	}

	return EXIT_SUCCESS;
}
