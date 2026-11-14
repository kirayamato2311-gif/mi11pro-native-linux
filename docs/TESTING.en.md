# Hardware testing and reports

Start with source review. This initial repository has no supported installation procedure or verified downloadable boot image. Do not test on a primary phone or on a model other than `mars`.

Before booting a self-built payload, keep your own verified stock/recovery backups and a confirmed fastboot recovery method. Bootloader unlocking and storage provisioning may erase data. Do not post those backups publicly.

Report each component separately:

1. **Identity:** device codename, RAM size, stock firmware version and panel variant when known. Redact serial/IMEI/MAC/account identifiers.
2. **Build:** this repository commit, upstream versions, configuration and patch differences, image hash, USB mode and userspace version.
3. **Boot:** temporary or permanent boot, reached stage, console output, recovery behavior. Do not assume A/B metadata and partition numbering match another device.
4. **Display/Power:** moving console output, native framebuffer name, one short press off/on, server connectivity while the display is blank. Avoid repeated long holds as a normal display control.
5. **Network:** adapter chipset identity and driver, negotiated link, local wired TCP throughput, errors/drops, internet throughput and Tailscale direct/relay results as separate measurements.
6. **Power/thermal:** charger/hub model, battery percentage trend, charging status, battery temperature and SoC temperatures, workload duration, CPU clocks if available. Stop if temperature is abnormal or the device resets. Do not infer charging from a lamp alone.

For Windows/other OS work, start a discussion/issue identifying the intended boot path and missing components. Report ACPI, storage, display and USB driver work individually. Do not describe Windows as supported just because its loader starts. Do not upload Windows images, proprietary driver bundles or third-party firmware without appropriate redistribution rights.

Use the bilingual issue template. Redact logs before uploading; device trees and boot logs can contain identifiers. Keep any changed image hashes and recovery notes locally as well as in the report.
