#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check/apply this snapshot to existing clean, pinned upstream checkouts."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import shutil

ROOT = Path(__file__).resolve().parents[1]

def run(repo, *args, env=None):
    try:
        return subprocess.check_output(['git', '-C', str(repo), *args], env=env, stderr=subprocess.STDOUT).decode().strip()
    except subprocess.CalledProcessError as exc:
        raise SystemExit(exc.output.decode(errors='replace')) from exc

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--uefi', required=True, type=Path)
    parser.add_argument('--kernel', required=True, type=Path)
    parser.add_argument('--apply', action='store_true', help='Modify both clean source checkouts after preflight; never flashes hardware')
    parser.add_argument('--allow-dirty-check', action='store_true', help='Check patches against base indexes only; cannot be combined with --apply')
    args = parser.parse_args()
    if args.apply and args.allow_dirty_check:
        parser.error('--allow-dirty-check cannot be combined with --apply')
    manifest = json.loads((ROOT/'sources.json').read_text())
    pairs = [('uefi', args.uefi.resolve()), ('kernel', args.kernel.resolve())]
    patches = {}
    for name, repo in pairs:
        if run(repo, 'rev-parse', 'HEAD') != manifest[name]['commit']:
            raise SystemExit(f'{name}: checkout does not match sources.json')
        if not args.allow_dirty_check and run(repo, 'status', '--porcelain', '--untracked-files=normal', '--ignore-submodules=none'):
            raise SystemExit(f'{name}: use a clean checkout, including submodules')
        patch = ROOT/'patches'/('linux' if name == 'kernel' else 'uefi')/'0001-development-snapshot.patch'
        patches[name] = patch
        with tempfile.TemporaryDirectory() as temp:
            env = dict(os.environ, GIT_INDEX_FILE=str(Path(temp)/'index'))
            run(repo, 'read-tree', manifest[name]['commit'], env=env)
            run(repo, 'apply', '--cached', '--check', '--ignore-space-change', str(patch), env=env)
        print(f'{name}: base commit and patch preflight OK')
    overlay = ROOT/'uefi/overlay'
    for source in overlay.rglob('*'):
        if source.is_file():
            target = args.uefi/source.relative_to(overlay)
            if not args.allow_dirty_check and target.exists():
                raise SystemExit(f'Overlay target already exists: {target}')
    if not args.apply:
        print('Check only; no source files changed. No hardware accessed.')
        return
    for name, repo in pairs:
        run(repo, 'apply', '--ignore-space-change', str(patches[name]))
    shutil.copytree(overlay, args.uefi, dirs_exist_ok=True)
    print('Applied source patches and UEFI overlay. No build or flash performed.')

if __name__ == '__main__':
    main()
