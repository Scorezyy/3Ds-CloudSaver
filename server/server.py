#!/usr/bin/env python3
"""
3DS CloudSaver - Companion Server (CSTP)
=========================================

This server runs on your PC/NAS and receives save files from the 3DS app.
It implements the CloudSaver Transfer Protocol (CSTP), a simple TCP-based
protocol for file management.

Usage:
    python3 server.py [--port PORT] [--dir DIRECTORY]

Defaults:
    Port: 5000
    Directory: ./cloudsaves

Protocol (all integers are big-endian):
    Request:  [4B cmd][4B payload_len][payload...]
    Response: [4B status][4B payload_len][payload...]

Commands:
    PING -> PONG
    LIST path -> OK + newline-separated file list
    MKDR path -> OK__ / ERR_
    UPLD path_len + path + file_size + file_data -> OK__ / ERR_
    DNLD path -> OK__ + file_data / ERR_
    DELE path -> OK__ / ERR_
    EXST path -> YES_ / NO__ / ERR_
"""

import argparse
import os
import socket
import struct
import threading
import sys
import datetime

DEFAULT_PORT = 5000
DEFAULT_DIR  = "./cloudsaves"

# Status codes (4 bytes, padded with underscores/nulls)
RSP_OK   = b"OK__"
RSP_ERR  = b"ERR_"
RSP_PONG = b"PONG"
RSP_YES  = b"YES_"
RSP_NO   = b"NO__"


class CSTProtocol:
    """Handles one client connection."""

    def __init__(self, conn: socket.socket, addr, base_dir: str):
        self.conn = conn
        self.addr = addr
        self.base_dir = os.path.abspath(base_dir)

    def run(self):
        print(f"[+] Connection from {self.addr}")
        try:
            while True:
                cmd = self._recv_exact(4)
                if not cmd:
                    break
                payload_len = struct.unpack(">I", self._recv_exact(4))[0]
                payload = self._recv_exact(payload_len) if payload_len > 0 else b""
                self._handle_command(cmd, payload)
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            print(f"[-] Connection lost from {self.addr}: {e}")
        finally:
            self.conn.close()
            print(f"[-] Disconnected: {self.addr}")

    def _recv_exact(self, n: int) -> bytes:
        data = b""
        while len(data) < n:
            chunk = self.conn.recv(n - len(data))
            if not chunk:
                return None
            data += chunk
        return data

    def _send_response(self, status: bytes, payload: bytes = b""):
        self.conn.sendall(status)
        self.conn.sendall(struct.pack(">I", len(payload)))
        if payload:
            self.conn.sendall(payload)

    def _safe_path(self, raw_path: str) -> str:
        """Resolve a path safely within the base directory."""
        # Remove leading slashes, prevent directory traversal
        clean = raw_path.lstrip("/").replace("..", "")
        full = os.path.join(self.base_dir, clean)
        full = os.path.abspath(full)
        if not full.startswith(self.base_dir):
            return None
        return full

    def _handle_command(self, cmd: bytes, payload: bytes):
        cmd_str = cmd.decode("ascii", errors="replace")
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")

        if cmd == b"PING":
            print(f"  [{timestamp}] PING from {self.addr}")
            self._send_response(RSP_PONG)

        elif cmd == b"LIST":
            path = payload.decode("utf-8", errors="replace")
            print(f"  [{timestamp}] LIST {path}")
            full = self._safe_path(path)
            if not full or not os.path.isdir(full):
                self._send_response(RSP_ERR, b"Directory not found")
                return
            try:
                entries = os.listdir(full)
                listing = "\n".join(entries)
                self._send_response(RSP_OK, listing.encode("utf-8"))
            except OSError as e:
                self._send_response(RSP_ERR, str(e).encode("utf-8"))

        elif cmd == b"MKDR":
            path = payload.decode("utf-8", errors="replace")
            print(f"  [{timestamp}] MKDR {path}")
            full = self._safe_path(path)
            if not full:
                self._send_response(RSP_ERR, b"Invalid path")
                return
            try:
                os.makedirs(full, exist_ok=True)
                self._send_response(RSP_OK)
            except OSError as e:
                self._send_response(RSP_ERR, str(e).encode("utf-8"))

        elif cmd == b"UPLD":
            # Payload: [4B path_len][path][4B file_size][file_data]
            if len(payload) < 8:
                self._send_response(RSP_ERR, b"Invalid payload")
                return
            path_len = struct.unpack(">I", payload[:4])[0]
            path = payload[4:4+path_len].decode("utf-8", errors="replace")
            file_size = struct.unpack(">I", payload[4+path_len:8+path_len])[0]
            file_data = payload[8+path_len:8+path_len+file_size]

            print(f"  [{timestamp}] UPLD {path} ({file_size} bytes)")
            full = self._safe_path(path)
            if not full:
                self._send_response(RSP_ERR, b"Invalid path")
                return
            try:
                os.makedirs(os.path.dirname(full), exist_ok=True)
                with open(full, "wb") as f:
                    f.write(file_data)
                self._send_response(RSP_OK)
                print(f"       -> Saved to {full}")
            except OSError as e:
                self._send_response(RSP_ERR, str(e).encode("utf-8"))

        elif cmd == b"DNLD":
            path = payload.decode("utf-8", errors="replace")
            print(f"  [{timestamp}] DNLD {path}")
            full = self._safe_path(path)
            if not full or not os.path.isfile(full):
                self._send_response(RSP_ERR, b"File not found")
                return
            try:
                with open(full, "rb") as f:
                    data = f.read()
                self._send_response(RSP_OK, data)
            except OSError as e:
                self._send_response(RSP_ERR, str(e).encode("utf-8"))

        elif cmd == b"DELE":
            path = payload.decode("utf-8", errors="replace")
            print(f"  [{timestamp}] DELE {path}")
            full = self._safe_path(path)
            if not full:
                self._send_response(RSP_ERR, b"Invalid path")
                return
            try:
                if os.path.isfile(full):
                    os.remove(full)
                elif os.path.isdir(full):
                    os.rmdir(full)
                self._send_response(RSP_OK)
            except OSError as e:
                self._send_response(RSP_ERR, str(e).encode("utf-8"))

        elif cmd == b"EXST":
            path = payload.decode("utf-8", errors="replace")
            full = self._safe_path(path)
            if full and os.path.exists(full):
                self._send_response(RSP_YES)
            else:
                self._send_response(RSP_NO)

        else:
            print(f"  [{timestamp}] Unknown command: {cmd_str}")
            self._send_response(RSP_ERR, b"Unknown command")


def main():
    parser = argparse.ArgumentParser(
        description="3DS CloudSaver Companion Server")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT,
                        help=f"TCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--dir", type=str, default=DEFAULT_DIR,
                        help=f"Storage directory (default: {DEFAULT_DIR})")
    parser.add_argument("--bind", type=str, default="0.0.0.0",
                        help="Bind address (default: 0.0.0.0)")
    args = parser.parse_args()

    # Create storage directory
    os.makedirs(args.dir, exist_ok=True)
    abs_dir = os.path.abspath(args.dir)

    print("=" * 60)
    print("  3DS CloudSaver - Companion Server")
    print("=" * 60)
    print(f"  Storage:  {abs_dir}")
    print(f"  Bind:     {args.bind}:{args.port}")
    print(f"  Protocol: CSTP v1")
    print("=" * 60)
    print()
    print("Waiting for 3DS connections...")
    print()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((args.bind, args.port))
    server.listen(5)

    try:
        while True:
            conn, addr = server.accept()
            handler = CSTProtocol(conn, addr, abs_dir)
            thread = threading.Thread(target=handler.run, daemon=True)
            thread.start()
    except KeyboardInterrupt:
        print("\n[!] Server shutting down...")
    finally:
        server.close()


if __name__ == "__main__":
    main()
