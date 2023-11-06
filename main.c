#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void change_access(struct udev_device *device, const char *c) {
  char *path = strdup(udev_device_get_syspath(device));
  char out[strlen(path) + strlen("/authorized")];
  strncpy(out, path, sizeof(out));
  strcat(out, "/authorized");
  FILE *auth = fopen(out, "w");
  if (!auth && !udev_device_get_sysattr_value(device, "authorized")) {
    strerror(errno);
    exit(EXIT_FAILURE);
  }
  fprintf(auth, "%s", c);
  fclose(auth);

  return;
}

void log_entry(struct udev_device *device) {

  time_t t = time(NULL);
  struct tm time = *localtime(&t);
  char crr_time[20];
  strftime(crr_time, 100, "%F - %T", &time);

  FILE *f = fopen("./event.log", "a");
  fprintf(f, "->: Time = %s\n", crr_time);
  fprintf(f, "-->: Authorized : %s\n",
          udev_device_get_sysattr_value(device, "authorized"));
  fprintf(f, "-->: Type = %s\n", udev_device_get_devtype(device));
  fprintf(f, "-->: Manufacturer = %s\n",
          udev_device_get_sysattr_value(device, "manufacturer"));
  fprintf(f, "-->: idVendor : %s\n",
          udev_device_get_sysattr_value(device, "idVendor"));
  fprintf(f, "-->: sys_path = %s\n", udev_device_get_syspath(device));
  fprintf(f, "---\n");
  fclose(f);

  return;
}

int ask_access(struct udev_device *device) {
  if (!udev_device_get_sysattr_value(device, "authorized"))
    return -1;

  if (!udev_device_get_sysattr_value(device, "idVendor"))
    return -2;

  // removing access
  change_access(device, "0");

  time_t t = time(NULL);
  struct tm time = *localtime(&t);
  char crr_time[20];
  strftime(crr_time, 100, "%F - %T", &time);

  printf(" ### NEW DEVICE FOUND! ###\n");
  printf("->: Time = %s\n", crr_time);
  printf("-->: Type = %s\n", udev_device_get_devtype(device));
  printf("-->: Manufacturer = %s\n",
         udev_device_get_sysattr_value(device, "manufacturer"));
  printf("-->: idVendor : %s\n",
         udev_device_get_sysattr_value(device, "idVendor"));
  printf("--> Alowing access? (y/n)\n");
  char c;
  while (1) {
    scanf("%c", &c);
    if (c == 'y' || c == 'n') {
      break;
    }
  }
  if (c == 'y') {
    printf(" ### ACCESS GRANTED ###\n");
    change_access(device, "1");
  } else if (c == 'n') {
    printf(" ### ACCESS DENIAD ###\n");
  }
  printf("|\n|\n");

  // loging
  log_entry(device);

  return 1;
}
int main() {
  struct udev *udev = udev_new();
  struct udev_monitor *monitor = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "usb", NULL);
  if (udev_monitor_enable_receiving(monitor) < 0) {
    exit(EXIT_FAILURE);
  }

  while (1) {
    struct udev_device *device = udev_monitor_receive_device(monitor);
    if (device) {
      if (strcmp(udev_device_get_action(device), "add") == 0) {
        ask_access(device);
      }
      /* free device */
      udev_device_unref(device);
    }
  }
  return 0;
}