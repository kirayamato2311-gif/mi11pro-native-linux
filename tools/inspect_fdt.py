# SPDX-License-Identifier: MIT
"""Read-only FDT inspection for board bring-up; no third-party dependencies."""
import struct
import sys
from pathlib import Path

data = Path(sys.argv[1]).read_bytes()
magic, total, offset, strings_offset, _, _, _, _, strings_size, structure_size = struct.unpack_from('>10I', data)
if magic != 0xD00DFEED or total > len(data):
    raise ValueError('Invalid FDT header')
strings = data[strings_offset:strings_offset + strings_size]
stack = []
end = offset + structure_size
while offset < end:
    token, = struct.unpack_from('>I', data, offset)
    offset += 4
    if token == 1:
        stop = data.index(0, offset)
        stack.append(data[offset:stop].decode())
        offset = (stop + 4) & ~3
    elif token == 2:
        stack.pop()
    elif token == 3:
        size, name_offset = struct.unpack_from('>II', data, offset)
        offset += 8
        name = strings[name_offset:strings.index(0, name_offset)].decode()
        value = data[offset:offset + size]
        offset = (offset + size + 3) & ~3
        path = '/'.join(stack) or '/'
        if (len(stack) == 1 and name in ('model', 'compatible', 'qcom,board-id', 'qcom,msm-id')) or (
            name in ('reg', 'status', 'width', 'height', 'stride', 'format') and
            any(x.startswith(('memory', 'reserved-memory', 'framebuffer')) for x in stack)):
            if value and value[-1] == 0 and all(x == 0 or 32 <= x < 127 for x in value):
                rendered = repr(value.rstrip(b'\0').decode().replace('\0', ' | '))
            elif len(value) % 4 == 0:
                rendered = ' '.join(f'{x:08x}' for x in struct.unpack('>' + 'I' * (len(value) // 4), value))
            else:
                rendered = value.hex()
            print(f'{path}: {name} = {rendered}')
    elif token == 9:
        break
    elif token != 4:
        raise ValueError(f'Unknown FDT token {token}')
