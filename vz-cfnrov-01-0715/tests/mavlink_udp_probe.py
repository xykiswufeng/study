#!/usr/bin/env python3
"""Passive-ish MAVLink UDP probe for the Orange Pi serial bridge.

The probe only transmits a standard GCS heartbeat so that UDP-LISTEN socat
creates its peer socket. It never sends arm, mode, actuator, or manual-control
commands.
"""

import argparse
import socket
import time


def crc_accumulate(byte, crc):
    tmp = byte ^ (crc & 0xFF)
    tmp ^= (tmp << 4) & 0xFF
    return ((crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4)) & 0xFFFF


def gcs_heartbeat(sequence):
    payload = bytes((0, 0, 0, 0, 6, 8, 0, 4, 3))
    header = bytes((len(payload), sequence & 0xFF, 255, 190, 0))
    crc = 0xFFFF
    for byte in header + payload:
        crc = crc_accumulate(byte, crc)
    crc = crc_accumulate(50, crc)  # HEARTBEAT CRC extra
    return bytes((0xFE,)) + header + payload + bytes((crc & 0xFF, crc >> 8))


def extract_frames(buffer):
    frames = []
    while buffer:
        if buffer[0] not in (0xFE, 0xFD):
            buffer = buffer[1:]
            continue
        if len(buffer) < 8:
            break
        if buffer[0] == 0xFE:
            payload_len = buffer[1]
            frame_len = 6 + payload_len + 2
            if len(buffer) < frame_len:
                break
            frame = buffer[:frame_len]
            frames.append((frame[3], frame[4], frame[5], frame[6:6 + payload_len]))
            buffer = buffer[frame_len:]
        else:
            payload_len = buffer[1]
            incompat_flags = buffer[2]
            signature_len = 13 if incompat_flags & 1 else 0
            frame_len = 10 + payload_len + 2 + signature_len
            if len(buffer) < frame_len:
                break
            frame = buffer[:frame_len]
            msgid = frame[7] | (frame[8] << 8) | (frame[9] << 16)
            frames.append((frame[5], frame[6], msgid, frame[10:10 + payload_len]))
            buffer = buffer[frame_len:]
    return frames, buffer


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="192.168.1.101")
    parser.add_argument("--remote-port", type=int, default=15001)
    parser.add_argument("--local-port", type=int, default=5555)
    parser.add_argument("--seconds", type=float, default=12.0)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", args.local_port))
    sock.settimeout(0.25)

    destination = (args.host, args.remote_port)
    started = time.monotonic()
    next_heartbeat = started
    sequence = 0
    buffer = b""
    frame_count = 0
    message_ids = set()
    vehicle_heartbeats = []

    while time.monotonic() - started < args.seconds:
        now = time.monotonic()
        if now >= next_heartbeat:
            sock.sendto(gcs_heartbeat(sequence), destination)
            sequence += 1
            next_heartbeat = now + 1.0
        try:
            data, source = sock.recvfrom(65535)
        except socket.timeout:
            continue
        if source[0] != args.host:
            continue
        buffer += data
        frames, buffer = extract_frames(buffer)
        for sysid, compid, msgid, payload in frames:
            frame_count += 1
            message_ids.add(msgid)
            if msgid == 0 and len(payload) >= 9:
                heartbeat = {
                    "sysid": sysid,
                    "compid": compid,
                    "vehicle_type": payload[4],
                    "autopilot": payload[5],
                    "base_mode": payload[6],
                    "system_status": payload[7],
                    "mavlink_version": payload[8],
                }
                if heartbeat not in vehicle_heartbeats:
                    vehicle_heartbeats.append(heartbeat)

    print(f"frames={frame_count}")
    print("message_ids=" + ",".join(str(value) for value in sorted(message_ids)))
    for heartbeat in vehicle_heartbeats:
        print("heartbeat=" + ",".join(f"{key}:{value}" for key, value in heartbeat.items()))
    if not vehicle_heartbeats:
        print("RESULT=NO_VEHICLE_HEARTBEAT")
        return 2
    print("RESULT=VEHICLE_HEARTBEAT_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
