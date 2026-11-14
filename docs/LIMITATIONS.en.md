# Known limitations

This is an inspected development snapshot, not a security-hardened distribution.

- Only `mars` with 8 GB RAM and the tested K2 panel configuration has device-side evidence. Other models and panel replacements need independent verification.
- The UEFI port inherits some `venus` board/binary/ACPI references. Sharing SM8350 does not establish board equivalence. Windows support is not claimed.
- The SMMU patch preserves an existing stream-ID-0 mapping on SM8350 and contains diagnostic delays/logging. It is broader than a finished Mars-only fix, changes reset behavior, and needs upstream-quality review before use outside a lab.
- Pinctrl diagnostics also introduce delays. ADSP auto-boot is disabled in the kernel patch and requires controlled userspace startup.
- The two device trees intentionally use **different fixed USB modes**. They do not implement automatic switching, PD role negotiation, or simultaneous charging/Ethernet validation.
- Battery property numbering is corrected only when the machine is `xiaomi,mars`. Charge thresholds and charge-current writes are not implemented in the published patch. Voltage, capacity and temperature readings alone do not prove charging control works.
- CPU frequency scaling, thermal policy and sustained operation remain unfinished. A fan does not substitute for a working thermal policy.
- Memory reserved for firmware, secure execution and active subsystems must not be reclaimed based only on an address appearing unused. The full physical 8 GB is not available as ordinary RAM.
- USB bus signaling at 480 Mbps is not an Ethernet throughput measurement. The observed adapter path delivered about 94.4 Mbps; gigabit was not established.
- Root filesystem preparation, module installation and initramfs assembly are not automated here. The reference init expects `/dev/sda34` to have `PARTNAME=userdata` and an existing marked ext4 root. This is device-specific and **not a portable partition map**.
- The physical test phone has existing display/radio faults. Do not attribute every fault to the port, or assume a different phone will behave identically.

No binary release is provided until firmware redistribution, recovery behavior and a repeatable clean build have been reviewed.
