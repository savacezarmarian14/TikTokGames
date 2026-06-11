"""Example external game adapter that maps gift events to generic actions.

This file intentionally lives outside the gateway package. It demonstrates how
an application can interpret gateway JSON without putting application behavior
inside the gateway core.
"""

from __future__ import annotations

import argparse
import asyncio
import json
from dataclasses import dataclass
from typing import Any

from websockets.client import connect


@dataclass(frozen=True)
class GameAction:
    """Generic action emitted by this example adapter."""

    action_type: str
    intensity: int
    source_event_id: str
    actor: str


GIFT_ACTIONS = {
    "Rose": ("support", 1),
    "Mock Gift": ("support", 1),
    "Finger Heart": ("boost", 2),
    "Galaxy": ("celebration", 5),
}


def build_parser() -> argparse.ArgumentParser:
    """Create the adapter argument parser."""
    parser = argparse.ArgumentParser(prog="generic-game-adapter")
    parser.add_argument("--url", default="ws://127.0.0.1:8765/events")
    return parser


async def run_adapter(url: str) -> None:
    """Connect to the gateway and print mapped generic actions."""
    async with connect(url) as websocket:
        await websocket.send(json.dumps({"event_types": ["gift"]}))

        async for message in websocket:
            event: dict[str, Any] = json.loads(message)
            action = map_event_to_action(event)
            if action is not None:
                print(json.dumps(action.__dict__), flush=True)


def map_event_to_action(event: dict[str, Any]) -> GameAction | None:
    """Map a gateway gift event into this app's own action model."""
    if event.get("event_type") != "gift":
        return None

    payload = event.get("payload") or {}
    gift_name = str(payload.get("gift_name") or "")
    action_type, base_intensity = GIFT_ACTIONS.get(gift_name, ("support", 1))
    repeat_count = int(payload.get("repeat_count") or 1)
    user = event.get("user") or {}

    return GameAction(
        action_type=action_type,
        intensity=base_intensity * repeat_count,
        source_event_id=str(event.get("event_id")),
        actor=str(user.get("username") or user.get("display_name") or "unknown"),
    )


def main() -> None:
    """Run the example adapter."""
    args = build_parser().parse_args()
    asyncio.run(run_adapter(args.url))


if __name__ == "__main__":
    main()
