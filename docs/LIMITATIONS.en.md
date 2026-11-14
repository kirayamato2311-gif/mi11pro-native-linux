# Known limitations

This is an inspected development snapshot, not a security-hardened distribution.

- Only `mars` with 8 GB RAM and the tested K2 panel configuration has device-side evidence. Other models and panel replacements need independent verification.
- The UEFI port inherits some `venus` board/binary/ACPI references. Sharing SM8350 does not establish board equivalence. Windows support is not claimed.
- The SMMU patch preserves an existing stream-ID-0 mapping on SM8350 and contains diagnostic delays/logging. It is broader than a finished Mars-only fix, changes reset behavior, and needs upstream-quality review before use outside a lab.
- Pinctrl diagnostics also introduce delays. ADSP auto-boot is disabled in the kernel patch and requires controlled userspace startup.
- The original two device trees use **different fixed USB modes**. The additional managed-USB tree and service passed two short PC/Ethernet round trips using controller reprobe. This is an experimental workaround, not a fully qualified role-switch driver or simultaneous charging/Ethernet validation; see `MANAGED_USB.en.md`.
- Battery property numbering is corrected only on `xiaomi,mars`. The later input-suspend attribute and userspace thresholds are specific to the tested OS1.0.11.0.UKAMIXM firmware protocol; do not assume XM IDs are stable across firmware or boards. Stop/resume was observed, but a complete long-term 85/80 charging cycle and PD/Ethernet operation remain unqualified. No FCC current-setting interface is included. See `CHARGING.en.md`.
- CPU frequency scaling, thermal policy and sustained operation remain unfinished. A fan does not substitute for a working thermal policy.
- Memory reserved for firmware, secure execution and active subsystems must not be reclaimed based only on an address appearing unused. The full physical 8 GB is not available as ordinary RAM.
- USB bus signaling at 480 Mbps is not an Ethernet throughput measurement. The observed adapter path delivered about 94.4 Mbps; gigabit was not established.
- Root filesystem preparation, module installation and initramfs assembly are not automated here. The reference init expects `/dev/sda34` to have `PARTNAME=userdata` and an existing marked ext4 root. This is device-specific and **not a portable partition map**.
- The physical test phone has existing display/radio faults. Do not attribute every fault to the port, or assume a different phone will behave identically.

No binary release is provided until firmware redistribution, recovery behavior and a repeatable clean build have been reviewed.
