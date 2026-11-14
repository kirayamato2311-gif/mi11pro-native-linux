# Experimental battery limit

The development phone uses a userspace policy to **stop input at 85% and resume at 80%**, checked every 15 seconds while Linux is running. This is input suspend, **not battery bypass**: the phone draws from its battery while input is suspended. The underlying firmware's protections remain necessary. This is not a hardware-enforced threshold for every boot/powered-off state.

## Firmware-specific protocol

The published kernel patch adds a Mars-only auxiliary-device attribute exposed at:

```text
/sys/class/power_supply/qcom-battmgr-bat/device/mars_input_suspend
```

For the tested **OS1.0.11.0.UKAMIXM** stock firmware, the Xiaomi XM request uses GET `0x50`, SET `0x51`, property **44**, and a boolean value. Its ID was confirmed by inspecting the `input_suspend_show`/`input_suspend_store` request construction in the original `qti_battery_charger_main.ko` from the locally extracted vendor filesystem. No proprietary module is included here.

Do not reuse an XM enum number from another Xiaomi source tree: older Venus sources used a different layout, and even the Star source enum is conditional on build options. The stock configuration had `CONFIG_BQ2597X=y` and no dual fuel gauge, but source enum counting alone still did not match the newer binary. The driver currently gates the attribute on `xiaomi,mars`; it **does not automatically identify the firmware version**. Verify the protocol independently before using it on another installation.

An earlier attempt to treat thermal-control property 10 as an FCC microamp setter was wrong for this firmware and triggered a charger-process crash during restoration. It was rolled back. Correcting the constant-current property did not establish an effective charging limit either. Those FCC write interfaces are **not** in the published final patch. Do not send large current values to thermal-level indices.

## Hardware evidence

With USB PC connected at approximately 75% battery:

| State | Firmware status | Battery current |
|---|---|---:|
| Input allowed (`0`) | Charging | about −252 mA |
| Input suspended (`1`) | Discharging | about +292 mA |
| Input restored (`0`) | Charging | about −255 mA |

Negative current was reported while charging on this phone. The test also confirmed that SSH stayed usable. The policy was then tested with a temporary stop threshold at the current capacity, which stopped input, followed by the intended 85/80 configuration, which resumed input because capacity was below 80%. **The battery was not actually taken through a full 85% → 80% cycle during this test.**

## Policy files

- `userspace/mars-charge-limit.sh` → `/usr/local/sbin/mars-charge-limit`
- `userspace/mars-charge-limit.service` and `.timer` → `/etc/systemd/system/`
- `/etc/default/mars-charge-limit`:

```ini
START_PERCENT=80
STOP_PERCENT=85
```

The timer is enabled with `systemctl enable --now mars-charge-limit.timer` only after the kernel attribute and manual stop/resume have been verified. State lives under `/var/lib/mars-charge-limit/`. Failed write/readback creates a `fault` marker and blocks further automatic writes for manual investigation. The script does not set charge current or negotiate Xiaomi 67 W charging.

To stop the policy and allow input again on a verified compatible installation:

```sh
sudo systemctl disable --now mars-charge-limit.timer
echo 0 | sudo tee /sys/class/power_supply/qcom-battmgr-bat/device/mars_input_suspend
```

Stopping the timer alone does not undo a firmware input-suspend setting. Simultaneous PD charging/Ethernet, operation with a wall charger, long-term battery behavior and charger compatibility need further testing. USB PC testing reported an SDP source with a 500 mA limit; a firmware `Fast` label is not proof of 67 W input.
