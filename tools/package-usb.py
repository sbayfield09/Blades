#!/usr/bin/env python3
"""Package the verified executable and premade artwork into one USB ZIP."""
from pathlib import Path
import hashlib, shutil, sys, zipfile
base=Path(sys.argv[1]);version=sys.argv[2];app=base/'Blades'
with zipfile.ZipFile(base/'Blades-artwork-pack.zip') as pack:
    if pack.testzip() is not None: raise ValueError('Artwork ZIP CRC failure')
    payload=pack.read('Assets/blades.xzp')
    expected=pack.read('ARTWORK-SHA256SUMS.txt').decode().split()[0]
    if hashlib.sha256(payload).hexdigest()!=expected: raise ValueError('Artwork checksum mismatch')
    (app/'Assets').mkdir(parents=True,exist_ok=True)
    (app/'Assets/blades.xzp').write_bytes(payload)
    for name in ['ARTWORK-README.txt','ARTWORK-SHA256SUMS.txt','AUDIO-NOTICE.txt','FONT-NOTICE.txt','DASHX360-NOTICE.txt']:
        (app/'Assets'/name).write_bytes(pack.read(name))
files=[app/'default.xex',app/'README.txt',app/'build-info.txt',app/'SHA256SUMS',app/'LICENSE',app/'XSDK-LICENSE.txt',app/'THIRD_PARTY_NOTICES.md',app/'Config/README.txt']
files+=sorted((app/'Assets').rglob('*'))+sorted((app/'licenses').rglob('*'))
output=base/f'Blades-v{version}-USB.zip'
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as archive:
    for file in files:
        if file.is_file():archive.write(file,file.relative_to(base))
with zipfile.ZipFile(output) as archive:
    if archive.testzip() is not None:raise ValueError('USB ZIP CRC failure')
    assert archive.read('Blades/default.xex')==(app/'default.xex').read_bytes()
    assert archive.read('Blades/Assets/blades.xzp')==payload
    assert 'Blades/Config/library-paths.txt' not in archive.namelist()
    assert 'Blades/Config/ftp-settings.txt' not in archive.namelist()
    assert 'Blades/Config/jellyfin-settings.json' not in archive.namelist()
    assert not any('/Players/' in n or n.endswith('.log') or Path(n).name in {'sideload.ini','player.ini','settings.ini'} for n in archive.namelist())
    assert all(not n.startswith('/') and '..' not in Path(n).parts for n in archive.namelist())
    assert 'Blades/licenses/VIDEO-LICENSE-NOTICE.txt' in archive.namelist()
shutil.copyfile(output,base/'Blades-xbox360.zip')
print(f'USB-ready ZIP: {output}')
