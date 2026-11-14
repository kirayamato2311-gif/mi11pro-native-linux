# Managed USB experiment

On 2026-11-14, two consecutive PC → Onten Ethernet → PC cycles passed without restarting the test phone. SSH worked over LAN and returned over the USB network gadget. A normal reboot with the PC cable connected also restored the service, native display, Power key and USB SSH after installation on the development phone.

This workaround uses `kernel/devicetree/mars-native-managed-usb-test.dts` and `userspace/mars-managed-usb.sh` with the example service. The tree retains ADSP/PMIC but deliberately leaves the UCSI connector graph disconnected from DWC3, avoiding competing automatic role changes. It sets OTG with peripheral as the default role and adds the conditional `mars,managed-usb-test` marker.

The service waits for a stable UCSI partner/data role for about three seconds. Before switching to host it unbinds the USB gadget. When returning from host to device, it **unbinds/rebinds only the DWC3 platform controller** before binding the existing gadget and restoring its USB network address. A simple DRD mode switch had completed in software but failed to restore PC enumeration; the full controller reprobe succeeded in the two tested cycles. The exact underlying hardware/driver cause has not been isolated.

Dependencies and limits:

- The existing configfs gadget must already be configured as `mars`; this script does not create it.
- It expects the tested DWC3 address, debugfs interface, UCSI port, drivers and userspace layout. Do not apply to other phones unchanged.
- It restores the test USB subnet `192.168.77.0/24` (phone `.2`). The PC needs its own compatible address; on the development Windows host `.1` had to be restored after re-enumeration.
- Switching tears down the network transport; existing SSH sessions are not preserved.
- A transition limit stops the test if roles change more than three times in 30 seconds. Automatic service restart is disabled to avoid a reset loop.
- Kernel follow logging is opt-in via `/etc/mars-managed-usb-debug`; ordinary transition logs remain available.
- USB PC telemetry showed charging from an SDP source with a 500 mA limit. The firmware's `Fast` charge-type label is **not** proof of 67 W charging.
- Cold boot, suspend, long-term hotplug reliability, all charger/hub combinations and simultaneous PD sink + Ethernet remain unqualified. Charge thresholds were subsequently implemented separately; see `CHARGING.en.md`.

Keep the separate fixed-host and fixed-peripheral trees as recovery/reference configurations. No boot binaries, root filesystem provisioning or flashing automation are distributed here.
