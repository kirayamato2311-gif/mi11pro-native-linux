#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check public text export for common private/build artifacts and broken local links."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
blocked = {'.img', '.bin', '.ko', '.dtb', '.dtbo', '.qcn', '.pem', '.key', '.pub', '.zip', '.o', '.pyc', '.log', '.efi', '.fd'}
patterns = [
    re.compile(r'-----BEGIN (?:OPENSSH |RSA |EC )?PRIVATE KEY-----'),
    re.compile(r'\bgh[pousr]_[A-Za-z0-9]{25,}'),
    re.compile(r'\bgithub_pat_[A-Za-z0-9_]{25,}'),
    re.compile(r'\bssh-(?:ed25519|rsa)\s+[A-Za-z0-9+/]{32,}'),
    re.compile(r'https://login\.tailscale\.com/a/[A-Za-z0-9]+'),
    re.compile(r'(?<![0-9])[0-9]{15}(?![0-9])'),
]
errors = []
count = 0
for p in root.rglob('*'):
    rel = p.relative_to(root)
    if any(x in {'.git', '__pycache__', 'out', 'build'} for x in rel.parts) or not p.is_file():
        continue
    count += 1
    if p.suffix.lower() in blocked or p.name in {'authorized_keys', 'tailscaled.state'}:
        errors.append(f'Forbidden artifact: {rel}')
    try:
        s = p.read_text(encoding='utf-8')
    except UnicodeError:
        errors.append(f'Non-text file: {rel}')
        continue
    if '\0' in s:
        errors.append(f'NUL byte: {rel}')
    for pat in patterns:
        if pat.search(s):
            errors.append(f'Potential identifier/secret in {rel}; inspect locally')
    if p.suffix == '.md':
        for link in re.findall(r'\]\(([^)]+)\)', s):
            if '://' in link or link.startswith('#'):
                continue
            target = link.split('#', 1)[0]
            if target and not (p.parent/target).exists():
                errors.append(f'Broken local link in {rel}: {link}')
if errors:
    print('\n'.join(errors))
    sys.exit(1)
print(f'Checked {count} text files; no blocked artifacts, pattern hits or broken local links.')
print('Pattern scanning supplements manual review; it is not proof that no sensitive data exists.')
