"""Demo WebSocket client that prints gateway events.

Run against a local gateway:

    python -m examples.clients.websocket_printer --event-types gift,comment
"""

from __future__ import annotations

import argparse
import asyncio
import json

from websockets.client import connect


def build_parser() -> argparse.ArgumentParser:
    """Create the demo client argument parser."""
    parser = argparse.ArgumentParser(prog="websocket-printer")
    parser.add_argument("--url", default="ws://127.0.0.1:8765/events")
    parser.add_argument(
        "--event-types",
        default=None,
        help="Optional comma-separated event filter, such as gift,comment,join.",
    )
    parser.add_argument("--pretty", action="store_true", help="Pretty-print received JSON.")
    return parser


async def run_client(url: str, event_types: str | None, pretty: bool) -> None:
    """Connect to the gateway and print each received JSON event."""
    async with connect(url) as websocket:
        if event_types:
            await websocket.send(json.dumps({"event_types": _event_type_list(event_types)}))

        async for message in websocket:
            if pretty:
                print(json.dumps(json.loads(message), indent=2), flush=True)
            else:
                print(message, flush=True)


def _event_type_list(value: str) -> list[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def main() -> None:
    """Run the demo WebSocket client."""
    args = build_parser().parse_args()
    asyncio.run(run_client(args.url, args.event_types, args.pretty))


if __name__ == "__main__":
    main()
