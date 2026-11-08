#!/usr/bin/env python3
"""Check verifier against a synthetic SDK-packed XEX, never a deployable title."""
import importlib.util
import struct
import sys
from pathlib import Path
from types import SimpleNamespace


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


root = Path(__file__).resolve().parents[1]
verify = load('verifier', root/'tools/verify_xex.py').verify
packer = load('packer', Path(sys.argv[1])/'tools/elf2xex.py')
image = bytearray(65536)
runs = [dict(start=4096, end=65536, type=1, pages=1, initialized=4096, uninitialized=57344)]
entry = 0x82001000
args = SimpleNamespace(stack_size=1048576, title_id=0x424c4453, version=0x10000)
image[4096:4100] = bytes.fromhex('4e800020')
packer.pe_headers(image, runs, entry, args.stack_size)
libraries = {
    'xam.xex': [dict(slot=0x82002000, thunk=0, ordinal=1)],
    'xboxkrnl.exe': [dict(slot=0x82002004, thunk=0, ordinal=1)]
}
packer.patch_import_records(image, libraries)
data = packer.build_xex(image, runs, entry, libraries, b'', args)
assert len(verify(data)) == 4
for offset in (0, 20, len(data)-1):
    corrupt = bytearray(data); corrupt[offset] ^= 1
    try:
        verify(corrupt)
    except (ValueError, struct.error):
        pass
    else:
        raise AssertionError(f'corruption at {offset} passed')
try:
    verify(data[:100])
except (ValueError, struct.error):
    pass
else:
    raise AssertionError('truncated XEX passed')
print('PASS: SDK-packed fixture accepted; corrupted/truncated XEX rejected (fixture not written to disk)')
