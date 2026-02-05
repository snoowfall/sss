#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <poll.h>
#include <stdio.h>

int open_restricted(const char *path, int flags, void *user_data) {
    (void)user_data; // cuz micro starts nagging about unused flags :sob:
    return open(path, flags);
}

void close_restricted(int fd, void *user_data) {
    (void)user_data;
    close(fd);
}

int main() {
    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "udev init failed :(\n");
        return 1;
    }

    struct libinput_interface li_iface = {
        .open_restricted = open_restricted,
        .close_restricted = close_restricted
    };

    struct libinput *li = libinput_udev_create_context(&li_iface, NULL, udev);
    if (!li) {
        fprintf(stderr, "failed to make li context\n");
        udev_unref(udev);
        return 1;
    }

    libinput_udev_assign_seat(li, "seat0");

    system("hyprctl dispatch dpms off");

    int fd = libinput_get_fd(li);
    struct pollfd fds = { .fd = fd, .events = POLLIN };

    while (1) {
        poll(&fds, 1, -1);
        libinput_dispatch(li);

        struct libinput_event *event;
        while ((event = libinput_get_event(li)) != NULL) {
            if (libinput_event_get_type(event) == LIBINPUT_EVENT_KEYBOARD_KEY) {
                struct libinput_event_keyboard *kb = libinput_event_get_keyboard_event(event);
                if (libinput_event_keyboard_get_key_state(kb) == LIBINPUT_KEY_STATE_PRESSED) {
                    system("hyprctl dispatch dpms on");
                    libinput_event_destroy(event);
                    goto cleanup;
                }
            }
            libinput_event_destroy(event);
        }
    }

cleanup:
    libinput_unref(li);
    udev_unref(udev);
    return 0;
}
