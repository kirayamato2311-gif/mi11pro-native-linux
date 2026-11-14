# SPDX-License-Identifier: MIT
"""Decode the exact stock panel command streams; never accesses hardware."""
from pathlib import Path
import hashlib
import json
import re

import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path, help='Panel-only DTSI extracted from your own stock firmware')
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
source = args.source
text = source.read_text()
out = args.output
out.mkdir(parents=True, exist_ok=True)

def prop(node, name):
    match = re.search(re.escape(name) + r'\s*=\s*([^;]+);', node)
    if not match:
        raise ValueError('Missing property: ' + name)
    return match.group(1).strip()

def raw(value):
    if value.startswith('<'):
        return b''.join(int(word, 0).to_bytes(4, 'big') for word in value[1:-1].split())
    if value.startswith('['):
        return bytes.fromhex(value[1:-1])
    raise ValueError('Not an encoded byte stream')

def integer(node, name):
    data = raw(prop(node, name))
    assert len(data) == 4, name
    return int.from_bytes(data, 'big')

def packets(data):
    position = 0
    result = []
    while position < len(data):
        if len(data) - position < 7:
            raise ValueError('Truncated packet header')
        dtype, last, channel, ack, wait, hi, lo = data[position:position+7]
        size = (hi << 8) | lo
        position += 7
        if not size or position + size > len(data):
            raise ValueError('Invalid packet length')
        if dtype not in (0x05, 0x15, 0x39):
            raise ValueError(f'Unsupported packet type {dtype:#x}')
        if channel != 0 or ack != 0 or last not in (0, 1):
            raise ValueError('Unexpected packet flags')
        payload = data[position:position+size]
        position += size
        # Downstream DCS short-write headers include their zero parameter.
        if dtype == 0x05 and len(payload) == 2 and payload[1] == 0:
            wire = payload[:1]
        else:
            wire = payload
        if dtype == 0x05 and len(wire) != 1:
            raise ValueError('Invalid DCS short command')
        if dtype == 0x15 and len(wire) != 2:
            raise ValueError('Invalid DCS short parameter')
        result.append(dict(type=dtype, last=last, channel=channel,
            wait_ms=wait, payload=payload.hex(), wire=wire.hex()))
    assert position == len(data)
    return result

timings = []
declarations = []
for match in re.finditer(r'timing@(\d+)\s*\{', text):
    end = match.end(); depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    node = text[match.start():end]
    hz = integer(node, 'qcom,mdss-dsi-panel-framerate')
    fields = {name: integer(node, 'qcom,mdss-dsi-' + name) for name in
        ('panel-width', 'panel-height', 'h-front-porch', 'h-back-porch',
         'h-pulse-width', 'v-front-porch', 'v-back-porch', 'v-pulse-width')}
    item = dict(refresh_hz=hz, timing=fields, commands={})
    for action in ('on', 'off'):
        data = raw(prop(node, f'qcom,mdss-dsi-{action}-command'))
        stream = packets(data)
        item['commands'][action] = stream
        (out / f'{hz}hz-{action}.bin').write_bytes(data)
        symbol = f'mars_{hz}hz_{action}'
        if hz != 60:
            declarations.append('#ifdef MARS_PANEL_ALL_MODES')
        declarations.append(f'static const struct mars_panel_command {symbol}[] = {{')
        for packet in stream:
            wire = bytes.fromhex(packet['wire'])
            assert len(wire) <= 256
            values = ', '.join(f'0x{x:02x}' for x in wire)
            declarations.append(f'    {{ .type = 0x{packet["type"]:02x}, .length = {len(wire)}, .wait_ms = {packet["wait_ms"]}, .data = {{ {values} }} }},')
        declarations.append('};\n')
        if hz != 60:
            declarations.append('#endif\n')
    pps_packets = [bytes.fromhex(p['wire'])[1:] for p in item['commands']['on']
        if bytes.fromhex(p['wire'])[0] == 0x9e]
    assert len(pps_packets) == 1 and len(pps_packets[0]) == 128
    pps = pps_packets[0]
    u16 = lambda offset: int.from_bytes(pps[offset:offset+2], 'big')
    item['dsc'] = dict(version=f'{pps[0] >> 4}.{pps[0] & 15}',
        bits_per_component=pps[3] >> 4, line_buffer_depth=pps[3] & 15,
        bits_per_pixel_x16=((pps[4] & 3) << 8) | pps[5],
        picture_height=u16(6), picture_width=u16(8),
        slice_height=u16(10), slice_width=u16(12), chunk_size=u16(14))
    assert item['dsc']['picture_width'] == fields['panel-width']
    assert item['dsc']['picture_height'] == fields['panel-height']
    assert item['dsc']['slice_width'] == integer(node, 'qcom,mdss-dsc-slice-width')
    assert item['dsc']['slice_height'] == integer(node, 'qcom,mdss-dsc-slice-height')
    (out / f'{hz}hz-pps.bin').write_bytes(pps)
    timings.append(item)
    print(hz, 'Hz:', len(item['commands']['on']), 'on packets,',
        len(item['commands']['off']), 'off packets; DSC', item['dsc'])
assert sorted(t['refresh_hz'] for t in timings) == [60, 90, 120]
header = '''/* Generated from this phone's original panel DT. Not a standalone driver. */
#ifndef MARS_PANEL_COMMANDS_H
#define MARS_PANEL_COMMANDS_H
struct mars_panel_command {
    unsigned short length;
    unsigned char type;
    unsigned char wait_ms;
    unsigned char data[256];
};
''' + '\n'.join(declarations) + '\n#endif\n'
(out / 'mars-panel-commands.h').write_text(header)
manifest = dict(source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
    panel='qcom,mdss_dsi_k2_38_08_0a_mp_dsc_cmd',
    hardware_tested=False, timings=timings)
(out / 'panel-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Validated all packet boundaries and PPS dimensions; output:', out)
