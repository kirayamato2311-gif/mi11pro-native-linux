// SPDX-License-Identifier: MIT
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

static int blank(int off) {
    int fd = open("/sys/class/graphics/fb0/blank", O_WRONLY);
    if (fd < 0) { perror("framebuffer blank"); return -1; }
    int ok = write(fd, off ? "4\n" : "0\n", 2) == 2;
    close(fd);
    if (!ok) { perror("blank write"); return -1; }
    printf("Display request: %s\n", off ? "off" : "on");
    fflush(stdout);
    return 0;
}

static int power_device(void) {
    glob_t paths = {0};
    int found = -1;
    if (glob("/dev/input/event*", 0, NULL, &paths)) return -1;
    for (size_t i = 0; i < paths.gl_pathc; i++) {
        unsigned char bits[(KEY_MAX + 8) / 8] = {0};
        int fd = open(paths.gl_pathv[i], O_RDONLY | O_CLOEXEC);
        if (fd < 0) continue;
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) >= 0 &&
            (bits[KEY_POWER / 8] & (1U << (KEY_POWER % 8)))) {
            if (ioctl(fd, EVIOCGRAB, 1) < 0) {
                perror("Power key exclusive input");
                close(fd);
                continue;
            }
            printf("Power key input: %s\n", paths.gl_pathv[i]);
            fflush(stdout);
            found = fd;
            break;
        }
        close(fd);
    }
    globfree(&paths);
    return found;
}

static int current_blank(int fallback) {
    FILE *file = fopen("/sys/class/graphics/fb0/blank", "r");
    int value;
    if (!file) return fallback;
    int found = fscanf(file, "%d", &value) == 1;
    fclose(file);
    return found ? value != 0 : fallback;
}

static uint64_t monotonic_ms(void) {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time)) return 0;
    return (uint64_t)time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--blank")) return blank(1) != 0;
    if (argc == 2 && !strcmp(argv[1], "--unblank")) return blank(0) != 0;
    if (argc != 1) { fprintf(stderr, "Usage: mars-screen-key [--blank|--unblank]\n"); return 2; }
    int off = 0;
    if (blank(0)) return 1;
    for (;;) {
        int fd = power_device();
        if (fd < 0) { sleep(2); continue; }
        struct input_event event;
        int pressed = 0;
        uint64_t last_release = 0;
        ssize_t count;
        while ((count = read(fd, &event, sizeof(event))) > 0) {
            if (count != sizeof(event)) continue;
            if (event.type == EV_SYN && event.code == SYN_DROPPED) pressed = 0;
            if (event.type == EV_KEY && event.code == KEY_POWER) {
                if (event.value == 1) pressed = 1;
                if (event.value == 0 && pressed) {
                    uint64_t now = monotonic_ms();
                    pressed = 0;
                    if (last_release && now - last_release < 350) continue;
                    last_release = now;
                    off = current_blank(off);
                    if (!blank(!off)) off = !off;
                }
            }
        }
        if (count < 0 && errno != EINTR) perror("input read");
        close(fd);
        sleep(1);
    }
}
