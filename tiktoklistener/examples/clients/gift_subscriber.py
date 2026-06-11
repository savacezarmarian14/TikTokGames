"""Example client that subscribes only to gift events."""

from __future__ import annotations

import argparse
import asyncio
import json
from typing import Any

from websockets.client import connect


def build_parser() -> argparse.ArgumentParser:
    """Create the gift subscriber argument parser."""
    parser = argparse.ArgumentParser(prog="gift-subscriber")
    parser.add_argument("--url", default="ws://127.0.0.1:8765/events")
    return parser


async def run_client(url: str) -> None:
    """Connect to the gateway and print gift events."""
    async with connect(url) as websocket:
        await websocket.send(json.dumps({"event_types": ["gift"]}))

        async for message in websocket:
            event: dict[str, Any] = json.loads(message)
            payload = event.get("payload", {})
            user = event.get("user") or {}
            print(
                "gift "
                f"name={payload.get('gift_name')} "
                f"repeat={payload.get('repeat_count', 1)} "
                f"user={user.get('username') or user.get('display_name') or 'unknown'}",
                flush=True,
            )


def main() -> None:
    """Run the gift subscriber."""
    args = build_parser().parse_args()
    asyncio.run(run_client(args.url))


if __name__ == "__main__":
    main()
