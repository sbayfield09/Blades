from pathlib import Path
import sys
p=Path(sys.argv[1]);text=p.read_text()
assert text.count('BEGIN CERTIFICATE')>100
Path(sys.argv[2]).write_text('static const char update_ca_roots[]=\n'+''.join('"'+line.replace('\\','\\\\').replace('"','\\"')+'\\n"\n' for line in text.splitlines())+';\n')
