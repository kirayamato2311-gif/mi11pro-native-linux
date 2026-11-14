// SPDX-License-Identifier: MIT
/* Development reference, not a portable installer. See docs/BUILD.en.md. */
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <time.h>

static int server_boot(void) {
    const char *modules[] = {"pinctrl-sm8350.ko", "arm_smmu.ko", "phy-qcom-qmp-ufs.ko", "ufs-qcom.ko"};
    for (unsigned i=0; i<sizeof(modules)/sizeof(modules[0]); i++) {
        char path[160]; snprintf(path,sizeof(path),"/lib/modules/%s",modules[i]);
        int fd=open(path,O_RDONLY);
        if (fd<0) { perror(path); return -1; }
        int rc=syscall(SYS_finit_module,fd,"",0); close(fd);
        if (rc) { perror(modules[i]); return -1; }
        printf("MARS module ready: %s\n",modules[i]);
    }
    for (int n=0; n<20 && access("/dev/sda34",F_OK); n++) sleep(1);
    FILE *info=fopen("/sys/class/block/sda34/uevent","r");
    char line[256]; int userdata=0;
    if (info) { while(fgets(line,sizeof(line),info)) if(!strcmp(line,"PARTNAME=userdata\n")) userdata=1; fclose(info); }
    if (!userdata) { puts("MARS root partition identity mismatch"); return -1; }
    mkdir("/newroot",0755);
    if(mount("/dev/sda34","/newroot","ext4",MS_NOATIME,NULL)) { perror("root mount"); return -1; }
    if(access("/newroot/etc/mars-root",F_OK)) { puts("MARS root marker absent"); return -1; }
    if (mount("/dev","/newroot/dev",NULL,MS_MOVE,NULL) ||
        mount("/proc","/newroot/proc",NULL,MS_MOVE,NULL) ||
        mount("/sys","/newroot/sys",NULL,MS_MOVE,NULL)) { perror("move root mounts"); return -1; }
    puts("MARS starting Ubuntu systemd from UFS");
    execl("/bin/busybox","busybox","switch_root","-c","/dev/console","/newroot","/sbin/init",(char *)NULL);
    perror("switch_root"); return -1;
}

static void start_ssh(void) {
    mkdir("/dev/pts", 0755); mkdir("/run", 0755); mkdir("/tmp", 01777);
    if (mount("devpts", "/dev/pts", "devpts", 0, "mode=0620,ptmxmode=0666")) perror("devpts");
    pid_t child = fork();
    if (!child) {
        execl("/usr/sbin/dropbear", "dropbear", "-R", "-F", "-s", "-j", "-k",
            "-p", "192.168.77.2:22", (char *)NULL);
        perror("SSH exec"); _exit(127);
    }
    if (child < 0) perror("SSH fork");
    else printf("SSH starting: root@192.168.77.2, public key only\n");
}

static void usb_address(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq req = {0};
    if (fd < 0) { perror("USB socket"); return; }
    snprintf(req.ifr_name, sizeof(req.ifr_name), "usb0");
    struct sockaddr_in *addr = (struct sockaddr_in *)&req.ifr_addr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.77.2", &addr->sin_addr);
    if (ioctl(fd, SIOCSIFADDR, &req)) perror("USB address");
    inet_pton(AF_INET, "255.255.255.0", &addr->sin_addr);
    if (ioctl(fd, SIOCSIFNETMASK, &req)) perror("USB netmask");
    if (ioctl(fd, SIOCGIFFLAGS, &req)) perror("USB get flags");
    else { req.ifr_flags |= IFF_UP; if (ioctl(fd, SIOCSIFFLAGS, &req)) perror("USB up"); }
    close(fd);
    printf("USB IP configured: 192.168.77.2/24 on usb0\n");
}

static int put(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) { printf("USB open %s: %s\n", path, strerror(errno)); return -1; }
    int rc = write(fd, value, strlen(value)) < 0 ? -1 : 0;
    if (rc) printf("USB write %s: %s\n", path, strerror(errno));
    close(fd); return rc;
}
static void usb_test(void) {
    mkdir("/sys/kernel/config", 0755);
    if (mount("configfs", "/sys/kernel/config", "configfs", 0, NULL)) perror("configfs");
    const char *base = "/sys/kernel/config/usb_gadget/mars";
    if (mkdir(base, 0755) || chdir(base)) { perror("USB gadget"); return; }
    put("idVendor", "0x1d6b"); put("idProduct", "0x0104");
    mkdir("strings/0x409", 0755);
    put("strings/0x409/serialnumber", "mars-linux-test");
    put("strings/0x409/manufacturer", "Mars experimental");
    put("strings/0x409/product", "Linux USB NCM");
    if (mkdir("configs/c.1", 0755)) { perror("USB config mkdir"); return; }
    if (mkdir("functions/ncm.usb0", 0755)) { perror("USB NCM mkdir"); return; }
    if (symlink("/sys/kernel/config/usb_gadget/mars/functions/ncm.usb0", "configs/c.1/ncm.usb0")) {
        perror("USB symlink"); return;
    }
    for (int attempt = 0; attempt < 5; attempt++) {
        DIR *dir = opendir("/sys/class/udc");
        struct dirent *entry;
        if (dir) {
            while ((entry = readdir(dir))) if (entry->d_name[0] != '.') {
                printf("USB UDC found: %s\n", entry->d_name);
                int rc = put("UDC", entry->d_name);
                closedir(dir); if (chdir("/")) perror("chdir root");
                printf("USB NCM bind=%d (0 means success)\n", rc);
                if (!rc) usb_address();
                return;
            }
            closedir(dir);
        }
        sleep(1);
    }
    if (chdir("/")) perror("chdir root");
    printf("USB UDC absent: inspect deferred probes below\n");
}

static void memory_test(void) {
    const size_t bytes = 768UL * 1024 * 1024;
    volatile uint64_t *ram = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ram == MAP_FAILED) { perror("RAMTEST mmap"); return; }
    const uint64_t patterns[] = {0xaaaaaaaaaaaaaaaaULL, 0x5555555555555555ULL,
        0x0000000000000000ULL, 0xffffffffffffffffULL};
    size_t count = bytes / sizeof(*ram);
    for (unsigned pass = 0; pass < 4; pass++) {
        printf("RAMTEST 768MiB pass %u write\n", pass + 1);
        for (size_t i = 0; i < count; i++) ram[i] = patterns[pass] ^ (uint64_t)i;
        size_t errors = 0;
        for (size_t i = 0; i < count; i++) {
            uint64_t expected = patterns[pass] ^ (uint64_t)i;
            uint64_t actual = ram[i];
            if (actual != expected) {
                if (errors < 4) printf("RAMTEST mismatch offset=%zu expected=%llx actual=%llx\n",
                    i * sizeof(*ram), (unsigned long long)expected, (unsigned long long)actual);
                errors++;
            }
        }
        printf("RAMTEST pass %u errors=%zu\n", pass + 1, errors);
        if (errors) { printf("RAMTEST FAILED\n"); munmap((void *)ram, bytes); return; }
    }
    munmap((void *)ram, bytes);
    printf("RAMTEST PASS: 768MiB, four patterns; not a full physical RAM test\n");
}

static void show(const char *path) {
    char buffer[1024];
    FILE *file = fopen(path, "r");
    printf("\n--- %s ---\n", path);
    if (!file) { printf("%s\n", strerror(errno)); return; }
    while (fgets(buffer, sizeof(buffer), file)) fputs(buffer, stdout);
    fclose(file);
}

static void usb_status(void) {
    show("/sys/class/udc/a600000.usb/state");
    show("/sys/class/udc/a600000.usb/current_speed");
    show("/sys/kernel/debug/usb/a600000.usb/link_state");
    FILE *file = fopen("/sys/kernel/debug/usb/a600000.usb/regdump", "r");
    char line[256];
    if (file) {
        while (fgets(line, sizeof(line), file))
            if (strstr(line, "DCTL") || strstr(line, "DSTS") ||
                strstr(line, "DCFG") || strstr(line, "GUSB2PHYCFG")) fputs(line, stdout);
        fclose(file);
    }
}

int main(void) {
    struct utsname kernel;
    mkdir("/dev", 0755); mkdir("/proc", 0555); mkdir("/sys", 0555);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    int console = open("/dev/console", O_RDWR);
    if (console >= 0) { dup2(console, 0); dup2(console, 1); dup2(console, 2); if (console > 2) close(console); }
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("MARS LINUX INITRAMFS: userspace reached; storage untouched\n");
    if (mount("proc", "/proc", "proc", 0, NULL)) perror("mount proc");
    if (mount("sysfs", "/sys", "sysfs", 0, NULL)) perror("mount sysfs");
    if (!uname(&kernel)) printf("Kernel: %s %s %s\n", kernel.sysname, kernel.release, kernel.machine);
    show("/sys/devices/system/cpu/online");
    if (access("/usb-test", F_OK) != 0) {
        show("/proc/meminfo");
        show("/proc/partitions");
        show("/proc/cmdline");
    }
    if (access("/usb-test", F_OK) == 0) {
        if (mount("debugfs", "/sys/kernel/debug", "debugfs", 0, NULL)) perror("debugfs");
        usb_test();
        if (access("/server-boot",F_OK)==0) {
            if(server_boot()) puts("MARS server boot failed; diagnostic fallback");
        }
        start_ssh();
        show("/sys/kernel/debug/devices_deferred");
        for (;;) { usb_status(); sleep(15); }
    } else memory_test();
    for (;;) { printf("MARS diagnostic init alive. No disk filesystem mounted.\n"); sleep(30); }
}
