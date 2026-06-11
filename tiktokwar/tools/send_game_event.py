#!/usr/bin/env python3
"""Send a local UDP JSON game_event to TikTokWar for manual testing."""

import argparse
import json
import socket
from datetime import datetime, timezone
from uuid import uuid4


def build_message(action: str, username: str, gift_name: str, repeat_count: int) -> dict:
    return {
        "message_type": "game_event",
        "action": action,
        "event": {
            "id": f"evt_{uuid4().hex[:12]}",
            "type": "gift",
            "timestamp": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        },
        "user": {
            "id": "local_test_user",
            "username": username,
            "display_name": username,
        },
        "payload": {
            "gift_name": gift_name,
            "coin_value": 1,
            "repeat_count": repeat_count,
            "is_streak_final": True,
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Send a TikTokWar local UDP game_event.")
    parser.add_argument("action", nargs="+", help='Action string, for example: spawn Red Blue, heal Red')
    parser.add_argument("--host", default="127.0.0.1", help="UDP host. Default: 127.0.0.1")
    parser.add_argument("--port", type=int, default=7000, help="UDP port. Default: 7000")
    parser.add_argument("--username", default="local_tester", help="Username metadata")
    parser.add_argument("--gift", default="Local Test", help="Gift name metadata")
    parser.add_argument("--repeat", type=int, default=1, help="repeat_count used as command count")
    args = parser.parse_args()

    action = " ".join(args.action)
    message = build_message(action, args.username, args.gift, max(1, args.repeat))
    payload = json.dumps(message).encode("utf-8")

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto(payload, (args.host, args.port))

    print(f"Sent to {args.host}:{args.port}: {json.dumps(message, indent=2)}")


if __name__ == "__main__":
    main()
