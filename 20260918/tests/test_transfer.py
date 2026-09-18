import os, pathlib, signal, socket, struct, subprocess, sys, tempfile, time, unittest
SOURCE = pathlib.Path(__file__).resolve().parents[1]
BUILD = tempfile.TemporaryDirectory(prefix='tcp-build-')
CLIENT = str(pathlib.Path(BUILD.name) / 'client')
SERVER = str(pathlib.Path(BUILD.name) / 'server')


def recv_exact(conn, length):
    data = b''
    while len(data) < length:
        part = conn.recv(length - len(data))
        if not part:
            raise AssertionError('Connection closed before expected bytes arrived')
        data += part
    return data

class ClientTests(unittest.TestCase):
    def test_missing_arguments(self):
        p = subprocess.run([CLIENT], capture_output=True, timeout=3)
        self.assertGreater(p.returncode, 0)
        self.assertIn(b'Usage:', p.stdout + p.stderr)

    def test_fragmented_ack(self):
        with tempfile.TemporaryDirectory() as tmp, socket.socket() as listener:
            path = pathlib.Path(tmp) / 'sample.bin'
            payload = bytes(range(256)) * 40
            path.write_bytes(payload)
            listener.bind(('127.0.0.1', 0))
            listener.listen()
            listener.settimeout(3)
            proc = subprocess.Popen([CLIENT, '127.0.0.1', str(listener.getsockname()[1]), 'sample.bin'], cwd=tmp, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            try:
                with listener.accept()[0] as conn:
                    conn.settimeout(3)
                    header = b''
                    while len(header) < 264:
                        part = conn.recv(264 - len(header))
                        self.assertTrue(part)
                        header += part
                    conn.sendall(b'O')
                    time.sleep(0.15)
                    try:
                        conn.sendall(b'K')
                        received = b''
                        while True:
                            part = conn.recv(65536)
                            if not part:
                                break
                            received += part
                    except (BrokenPipeError, ConnectionResetError):
                        received = b''
                    self.assertEqual(received, payload, 'Client must wait for both ACK bytes before sending file')
                self.assertEqual(proc.wait(timeout=3), 0)
            finally:
                if proc.poll() is None:
                    proc.kill()
                proc.communicate()

class ServerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='tcp-receive-')
        cls.directory = pathlib.Path(cls.tmp.name)
        cls.log = open(cls.directory / 'server.log', 'w+')
        # The learning server intentionally keeps its original fixed port 8080.
        with socket.socket() as probe:
            probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe.bind(('127.0.0.1', 8080))
        cls.server = subprocess.Popen([SERVER], cwd=cls.tmp.name, stdout=cls.log,
                                      stderr=cls.log, start_new_session=True)
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            cls.log.seek(0)
            if 'Server is listening' in cls.log.read():
                return
            if cls.server.poll() is not None:
                break
            time.sleep(0.02)
        cls.tearDownClass()
        raise AssertionError('Server did not start')

    @classmethod
    def tearDownClass(cls):
        if cls.server.poll() is None:
            os.killpg(cls.server.pid, signal.SIGTERM)
        cls.server.wait(timeout=3)
        cls.log.close()
        cls.tmp.cleanup()

    def connect(self):
        return socket.create_connection(('127.0.0.1', 8080), timeout=3)

    def wait_file(self, name, payload):
        path = self.directory / ('recv_' + name)
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline:
            if path.exists() and path.read_bytes() == payload:
                return
            time.sleep(0.02)
        self.fail('Received file differs: ' + name)

    def test_text_binary_empty_and_paths(self):
        with tempfile.TemporaryDirectory() as tmp:
            for name, payload in [('text.txt', 'hello 文件传输\n'.encode()),
                                  ('binary.bin', bytes(range(256)) * 4097),
                                  ('empty.bin', b'')]:
                with self.subTest(name=name):
                    path = pathlib.Path(tmp) / name
                    path.write_bytes(payload)
                    p = subprocess.run([CLIENT, '127.0.0.1', '8080', str(path)],
                                       capture_output=True, timeout=5)
                    self.assertEqual(p.returncode, 0, p.stderr)
                    self.wait_file(name, payload)
                    self.assertEqual(path.read_bytes(), payload)

    def test_fragmented_header_body_and_connection_close(self):
        payload = bytes(range(256)) * 33
        with self.connect() as conn:
            header = struct.pack('@256sQ', b'fragment.bin', len(payload))
            for i in range(0, len(header), 7):
                conn.sendall(header[i:i+7])
            ack = b''
            while len(ack) < 2:
                part = conn.recv(2 - len(ack))
                self.assertTrue(part)
                ack += part
            self.assertEqual(ack, b'OK')
            for i in range(0, len(payload), 113):
                conn.sendall(payload[i:i+113])
            self.assertEqual(conn.recv(1), b'')
        self.wait_file('fragment.bin', payload)
        self.log.seek(0)
        self.assertNotIn('Accept failed', self.log.read())

    def test_concurrent_client_while_first_waits(self):
        with self.connect() as slow, self.connect() as fast:
            fast.sendall(struct.pack('@256sQ', b'concurrent.bin', 1))
            self.assertEqual(recv_exact(fast, 2), b'OK')
            fast.sendall(b'X')
            self.assertEqual(fast.recv(1), b'')
        self.wait_file('concurrent.bin', b'X')

    def test_truncated_body_is_failure(self):
        with self.connect() as conn:
            conn.sendall(struct.pack('@256sQ', b'truncated.bin', 100))
            self.assertEqual(recv_exact(conn, 2), b'OK')
            conn.sendall(b'abc')
            conn.shutdown(socket.SHUT_WR)
            self.assertEqual(conn.recv(1), b'')
        self.wait_file('truncated.bin', b'abc')
        self.log.seek(0)
        self.assertIn('Receive file failed', self.log.read())

    def test_invalid_filename(self):
        for name in [b'x' * 256, b'../escape', b'']:
            with self.subTest(name=name[:20]), self.connect() as conn:
                conn.sendall(struct.pack('@256sQ', name, 0))
                self.assertEqual(conn.recv(2), b'')

    def test_bad_arguments_and_missing_file(self):
        for args in [('127.0.0.1', 'abc', 'x'), ('127.0.0.1', '99999', 'x'),
                     ('127.0.0.1', '8080junk', 'x'), ('bad-ip', '8080', 'x'),
                     ('127.0.0.1', '8080', '/does-not-exist-tcp-test')]:
            with self.subTest(args=args):
                p = subprocess.run([CLIENT, *args], capture_output=True, timeout=3)
                self.assertGreater(p.returncode, 0)

if __name__ == '__main__':
    try:
        for src, target in [('client_file.cpp', CLIENT), ('server_file.cpp', SERVER)]:
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-Wall',
                            '-Wextra', '-Werror', str(SOURCE / src), '-o', target], check=True)
        unittest.main(verbosity=2)
    finally:
        BUILD.cleanup()
