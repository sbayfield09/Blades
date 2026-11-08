#!/usr/bin/env python3
"""Validate xsdk's unencrypted/basic-compressed XEX2, not console compatibility."""
import argparse
import hashlib
import struct
from pathlib import Path


def verify(data, *, resource_dll=False, title_dll=False):
    def require(condition, message):
        if not condition:
            raise ValueError(message)

    def be(offset):
        require(0 <= offset <= len(data)-4, 'integer outside file')
        return struct.unpack_from('>I', data, offset)[0]

    require(len(data) >= 24 and data[:4] == b'XEX2', 'not a XEX2 executable')
    flags, payload, _, security, count = struct.unpack_from('>5I', data, 4)
    require(flags & 1 and 24+count*8 <= security < payload < len(data), 'invalid title/header offsets')
    if resource_dll or title_dll:
        require(flags == 9, 'resource DLL must be a title DLL')
    require(security+0x184 <= payload, 'truncated security header')
    headers = dict(struct.iter_unpack('>II', data[24:24+count*8]))
    require(len(headers) == count, 'duplicate optional headers')
    expected = data[security+0x164:security+0x178]
    actual = hashlib.sha1(data[security+0x17c:payload]+data[:security+8]).digest()
    require(actual == expected, 'header digest mismatch')
    require(0x3ff in headers, 'missing file format header')
    format_offset = headers[0x3ff]
    require(format_offset+8 <= payload, 'format header outside XEX headers')
    length, encryption, compression = struct.unpack_from('>IHH', data, format_offset)
    require(encryption == 0 and compression == 1, 'expected unencrypted basic compression')
    require(length >= 8 and (length-8) % 8 == 0 and format_offset+length <= payload, 'invalid compression table')
    image_size = be(security+4)
    require(0 < image_size <= 128*1024*1024, 'invalid image size')
    image = bytearray()
    cursor = payload
    for compressed, zeros in struct.iter_unpack('>II', data[format_offset+8:format_offset+length]):
        require(cursor+compressed <= len(data), 'truncated payload')
        require(len(image)+compressed+zeros <= image_size, 'compression exceeds image')
        image.extend(data[cursor:cursor+compressed]); image.extend(bytes(zeros))
        cursor += compressed
    require(cursor == len(data), 'unreferenced trailing file data')
    image.extend(bytes(image_size-len(image)))
    require(image[:4] == b'MZ\x90\x00', 'invalid DOS signature')
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    require(pe+94 <= len(image) and image[pe:pe+4] == b'PE\0\0', 'invalid PE signature')
    require(struct.unpack_from('<H', image, pe+4)[0] == 0x1f2, 'PE machine is not big-endian PowerPC')
    require(struct.unpack_from('<H', image, pe+92)[0] == 14, 'PE subsystem is not Xbox')
    base = headers.get(0x10201, 0)
    entry = headers.get(0x10100, 0)
    require(base <= entry < base+len(image), 'entry point outside image')
    image_flags = be(security+0x10c)
    page_bytes = 4096 if image_flags & 0x10000000 else 65536
    # Select the expected heap geometry independently of the page flag.
    # A self-consistent hash chain with the wrong page size is still invalid.
    require(0x80000000 <= base < 0xa0000000, 'image base outside XEX address ranges')
    expected_page_bytes = 4096 if base >= 0x90000000 else 65536
    require(page_bytes == expected_page_bytes, 'XEX page size disagrees with image address range')
    require(base % page_bytes == 0 and base+image_size <= (0xa0000000 if page_bytes == 4096 else 0x90000000),
            'image crosses XEX heap bounds or is unaligned')
    descriptor_count = be(security+0x180)
    require(descriptor_count*page_bytes == image_size, 'page count differs from image size')
    require(security+0x184+descriptor_count*24 <= payload, 'page descriptors outside headers')
    expected = data[security+0x114:security+0x128]
    for page in range(descriptor_count):
        off = security+0x184+page*24
        descriptor = data[off:off+24]
        require(be(off) >> 4 == 1, 'expected one page per descriptor')
        require(hashlib.sha1(image[page*page_bytes:(page+1)*page_bytes]+descriptor).digest() == expected,
                f'image digest mismatch at page {page}')
        expected = descriptor[4:]
    require(expected == bytes(20), 'invalid page-chain terminator')
    require(0x103ff in headers, 'missing import table')
    imp = headers[0x103ff]
    require(imp+12 <= payload, 'import header outside XEX headers')
    length, string_size, libraries = struct.unpack_from('>III', data, imp)
    end = imp+length
    require(imp+12+string_size <= end <= payload, 'import table outside XEX headers')
    names = [x.decode('ascii') for x in data[imp+12:imp+12+string_size].split(b'\0') if x]
    require(libraries == len(names) == be(security+0x128), 'import library count mismatch')
    expected_names = set() if resource_dll else set(names) if title_dll else {'xam.xex', 'xboxkrnl.exe'}
    if title_dll:
        require('xboxkrnl.exe' in expected_names and expected_names <= {'xboxkrnl.exe','xam.xex'}, 'title DLL imports outside approved kernel/XAM APIs')
    require(set(names) == expected_names,
            f'unexpected import libraries: {names}')
    expected = data[security+0x12c:security+0x140]
    cursor = imp+12+string_size
    results = []
    for _ in range(libraries):
        require(cursor+40 <= end, 'truncated import record')
        size = be(cursor)
        require(size >= 40 and cursor+size <= end, 'invalid import record size')
        record = data[cursor:cursor+size]
        require(hashlib.sha1(record[4:]).digest() == expected, 'import digest mismatch')
        expected = record[4:24]
        index, slots = struct.unpack_from('>HH', record, 36)
        require(index < libraries and size == 40+slots*4, 'invalid import slot table')
        for address, in struct.iter_unpack('>I', record[40:]):
            require(base <= address <= base+len(image)-4, 'import address outside image')
        results.append(f'{names[index]}: {slots} import slots')
        cursor += size
    require(cursor == end and expected == bytes(20), 'invalid import-chain terminator')
    if resource_dll or title_dll:
        require(struct.unpack_from('<H', image, pe+22)[0] & 0x2000, 'missing PE DLL flag')
        require(0x20104 not in headers, 'resource DLL must not allocate TLS')
    if title_dll:
        require(page_bytes == 4096, 'Peer Pressure helper must use 4 KB pages')
        results.append('Peer Pressure title DLL; approved kernel/XAM imports; no TLS')
    if resource_dll:
        resource = headers.get(0x2ff, payload)
        require(resource+20 <= payload and be(resource) == 20, 'expected one bounded resource')
        name, address, size = struct.unpack_from('>8sII', data, resource+4)
        require(name == b'lua\0\0\0\0\0' and base <= address <= base+len(image)-size,
                'invalid lua resource')
        require(size >= 22 and image[address-base:address-base+8] == b'XUIZ\0\0\0\3',
                'resource must be XUIZ version 3')
        require(image[entry-base:entry-base+8] == bytes.fromhex('386000014e800020'),
                'resource DLL must have only a return-TRUE entry stub')
        results.append(f'import-free resource DLL; lua XUIZ {size} bytes; no TLS')
    return [f'XEX2 PowerPC-BE, Xbox subsystem, entry {entry:#x}',
            f'{descriptor_count} verified {page_bytes}-byte image pages; header/image/import digest chains valid', *results]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('xex', type=Path)
    parser.add_argument('--resource-dll', action='store_true')
    parser.add_argument('--title-dll', action='store_true')
    args = parser.parse_args()
    try:
        for result in verify(args.xex.read_bytes(), resource_dll=args.resource_dll, title_dll=args.title_dll):
            print('PASS:', result)
    except (OSError, ValueError, struct.error, UnicodeError) as error:
        raise SystemExit(f'FAIL: {error}') from error
    print('Console launch and Peer Pressure compatibility remain unverified.')


if __name__ == '__main__':
    main()
