"""Exercise the production FTP server over loopback with actual disk I/O."""
import ftplib
import io
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import time
import hashlib
from concurrent.futures import ThreadPoolExecutor

class Server:
    def __init__(self, root, config):
        self.proc = subprocess.Popen([sys.argv[1], str(root), str(config)], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, text=True)
    def status(self, command="status"):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()
        line = self.proc.stdout.readline()
        assert line, f"FTP fixture exited: {self.proc.poll()}"
        return json.loads(line)
    def wait(self, predicate):
        deadline = time.monotonic() + 6
        while time.monotonic() < deadline:
            s = self.status()
            if predicate(s):
                return s
            time.sleep(.02)
        raise AssertionError(f"FTP status timeout: {s}")
    def close(self):
        self.proc.stdin.write("quit\n")
        self.proc.stdin.flush()
        assert self.proc.wait(timeout=6) == 0
        self.proc.stdin.close()
        self.proc.stdout.close()

def rejected(fn):
    try:
        fn()
    except ftplib.error_perm:
        return
    raise AssertionError("Request unexpectedly succeeded")

def client(port, source=None, username="blades", password="blades"):
    ftp = ftplib.FTP(timeout=5, source_address=source)
    ftp.connect("127.0.0.1", port)
    ftp.login(username, password)
    return ftp

with tempfile.TemporaryDirectory(prefix="blades-ftp-") as temporary:
    root = Path(temporary)
    (root / "HDD").mkdir()
    (root / "USB0").mkdir()
    (root / "private").mkdir()
    (root / "private/secret").write_bytes(b"outside export")
    (root / "HDD/escape").symlink_to(root / "private", target_is_directory=True)
    config = root / "Config/ftp-settings.txt"
    server = Server(root, config)
    try:
        s = server.wait(lambda s: s["state"] == 2)
        assert s["enabled"] and not s["devices"] and s["port"]
        ftp = ftplib.FTP(timeout=5)
        ftp.connect("127.0.0.1", s["port"])
        rejected(lambda: ftp.login("blades", "wrong"))
        rejected(lambda: ftp.nlst())
        ftp.login("blades", "blades")
        assert ftp.pwd() == "/"
        assert set(ftp.nlst()) == {"HDD", "USB0"}
        assert {n for n, facts in ftp.mlsd() if facts["type"] == "dir"} == {"HDD", "USB0"}
        rows = []
        ftp.retrlines("LIST", rows.append)
        assert len(rows) == 2 and all(line.startswith("d") for line in rows)
        assert "type=dir;" in ftp.sendcmd("MLST /")
        rejected(lambda: ftp.cwd("/private"))
        rejected(lambda: ftp.cwd("/HDD/../../private"))
        rejected(lambda: ftp.cwd("/HDD/escape"))
        rejected(lambda: ftp.retrbinary("RETR /HDD/escape/secret", lambda b: None))
        rejected(lambda: ftp.storbinary("STOR /HDD/escape/new", io.BytesIO(b"bad")))
        rejected(lambda: ftp.rmd("/HDD"))
        rejected(lambda: ftp.rename("/HDD", "/USB0/bad"))
        ftp.cwd("/hdd")  # Drive aliases ignore case.
        ftp.mkd("A folder")
        ftp.cwd("A folder")
        data = os.urandom(1024 * 1024 + 137)
        ftp.storbinary("STOR game.bin", io.BytesIO(data))
        assert (root / "HDD/A folder/game.bin").read_bytes() == data
        assert ftp.size("game.bin") == len(data)
        assert ftp.sendcmd("MDTM game.bin").startswith("213 ")
        received = io.BytesIO()
        ftp.retrbinary("RETR game.bin", received.write)
        assert received.getvalue() == data
        received = io.BytesIO()
        ftp.retrbinary("RETR game.bin", received.write, rest=65539)
        assert received.getvalue() == data[65539:]
        # Exercise aggregation boundaries and fragmented writes, including final tails.
        for size in (1,262143,262144,262145,1048593):
            payload=os.urandom(size)
            ftp.storbinary("STOR blocks.bin",io.BytesIO(payload),blocksize=1739)
            assert (root / "HDD/A folder/blocks.bin").read_bytes()==payload
        def parallel(number):
            peer=client(s["port"]);payload=os.urandom(8*1024*1024+137+number)
            peer.storbinary(f"STOR /USB0/parallel-{number}.bin",io.BytesIO(payload),blocksize=65536)
            digest=hashlib.sha256();peer.retrbinary(f"RETR /USB0/parallel-{number}.bin",digest.update,blocksize=65536)
            assert digest.digest()==hashlib.sha256(payload).digest();peer.quit()
        with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(parallel,range(2)))
        ftp.storbinary("STOR resume.bin", io.BytesIO(data[:1000]))
        ftp.storbinary("STOR resume.bin", io.BytesIO(data[1000:]), rest=1000)
        assert (root / "HDD/A folder/resume.bin").read_bytes() == data
        ftp.storbinary("APPE resume.bin", io.BytesIO(b"tail"))
        assert (root / "HDD/A folder/resume.bin").read_bytes() == data + b"tail"
        ftp.storbinary("STOR empty", io.BytesIO())
        ftp.storbinary("STOR café.txt", io.BytesIO(b"utf8 name"))
        assert "café.txt" in ftp.nlst()
        ftp.rename("empty", "renamed")
        ftp.delete("renamed")
        ftp.cwd("/")
        assert ftp.pwd() == "/"
        ftp.cwd("..")
        assert ftp.pwd() == "/"
        # EPSV explicitly, in addition to ftplib's usual IPv4 PASV.
        host, port = ftplib.parse229(ftp.sendcmd("EPSV"), ftp.sock.getpeername())
        with socket.create_connection((host, port), timeout=5) as conn:
            assert ftp.sendcmd("NLST").startswith("150")
            payload = b""
            while block := conn.recv(8192):
                payload += block
        assert ftp.voidresp().startswith("226") and b"HDD" in payload
        # Multiple FTP sessions on one PC count as one connected device.
        second = client(s["port"])
        assert server.wait(lambda s: s["devices"] == 1)
        third = client(s["port"], ("127.0.0.2", 0))
        server.wait(lambda s: s["devices"] == 2)
        third.quit()
        server.wait(lambda s: s["devices"] == 1)
        # Turning off cancels control/data waits; partial uploads never get success.
        ftp.cwd("/USB0")
        pending = ftp.transfercmd("STOR interrupted.bin")
        pending.sendall(b"partial")
        server.status("off")
        server.wait(lambda s: s["state"] == 0 and s["devices"] == 0)
        pending.close()
        ftp.close()
        second.close()
    finally:
        server.close()
    assert config.read_text().startswith("enabled=0\n")
    server = Server(root, config)
    try:
        server.wait(lambda s: not s["enabled"] and s["state"] == 0)
        server.status("on")
        port = server.wait(lambda s: s["state"] == 2)["port"]
        check = client(port)
        assert set(check.nlst()) == {"HDD", "USB0"}
        check.quit()
    finally:
        server.close()
    assert config.read_text().startswith("enabled=1\n")
    server = Server(root, config)
    try:
        server.wait(lambda s: s["state"] == 2 and s["enabled"])
        server.status("credentials")
        server.status("invalid")
        time.sleep(.2)
        port = server.wait(lambda s: s["state"] == 2)["port"]
        check = client(port, username="custom_user", password="custom_pass")
        assert set(check.nlst()) == {"HDD", "USB0"}
        check.quit()
        # Immediate off+exit still persists the final choice.
        server.status("off")
    finally:
        server.close()
    assert config.read_text().startswith("enabled=0\n")
    assert "username=custom_user\npassword=custom_pass\n" in config.read_text()
    server = Server(root, config)
    try:
        server.wait(lambda s: s["state"] == 0)
        server.status("on")
        port = server.wait(lambda s: s["state"] == 2)["port"]
        check = client(port, username="custom_user", password="custom_pass")
        check.quit()
    finally:
        server.close()
    assert not (root / "private/new").exists()
    # Existing v0.30 installations keep their saved off state and default login.
    config.write_text("enabled=0\n")
    server = Server(root, config)
    try:
        server.wait(lambda s: s["state"] == 0 and not s["enabled"])
        server.status("on")
        check = client(server.wait(lambda s: s["state"] == 2)["port"])
        check.quit()
    finally:
        server.close()
    # An unreadable/unwritable settings location is reported, never silently saved.
    (root / "blocked").write_text("not a directory")
    server = Server(root, root / "blocked/ftp-settings.txt")
    try:
        server.wait(lambda s: s["state"] == 2 and s["save_error"])
        server.status("off")
        server.wait(lambda s: not s["enabled"] and s["save_error"])
    finally:
        server.close()
print("PASS: real FTP login/root/listings, PASV/EPSV, binary upload/download/resume/append, Unicode, metadata, rename/delete, path boundaries, unique device counts, off cancellation, on/off restart persistence and shared status/toggle navigation")
