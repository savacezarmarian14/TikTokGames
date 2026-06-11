"""Dashboard-like terminal client that displays event counts by type."""

from __future__ import annotations

import argparse
import asyncio
import json
from collections import Counter
from datetime import UTC, datetime
from typing import Any

from websockets.client import connect


def build_parser() -> argparse.ArgumentParser:
    """Create the dashboard argument parser."""
    parser = argparse.ArgumentParser(prog="terminal-dashboard")
    parser.add_argument("--url", default="ws://127.0.0.1:8765/events")
    parser.add_argument("--refresh-events", type=int, default=1)
    return parser


async def run_dashboard(url: str, refresh_events: int) -> None:
    """Connect to the gateway and render event counts."""
    counts: Counter[str] = Counter()
    total = 0
    last_event: dict[str, Any] | None = None
    started_at = datetime.now(UTC)

    async with connect(url) as websocket:
        async for message in websocket:
            event = json.loads(message)
            event_type = str(event.get("event_type", "unknown"))
            counts[event_type] += 1
            total += 1
            last_event = event

            if total % max(refresh_events, 1) == 0:
                render(counts, total, started_at, last_event)


def render(
    counts: Counter[str],
    total: int,
    started_at: datetime,
    last_event: dict[str, Any] | None,
) -> None:
    """Render a simple terminal dashboard."""
    elapsed = max((datetime.now(UTC) - started_at).total_seconds(), 0.001)
    print("\033[2J\033[H", end="")
    print("TikTok Event Gateway Dashboard")
    print("=" * 34)
    print(f"Total events: {total}")
    print(f"Events/sec:   {total / elapsed:.2f}")
    print()
    print("Counts by type:")
    for event_type, count in sorted(counts.items()):
        print(f"  {event_type:<12} {count}")
    print()
    if last_event is not None:
        print("Last event:")
        print(json.dumps(last_event, indent=2))


def main() -> None:
    """Run the terminal dashboard."""
    args = build_parser().parse_args()
    asyncio.run(run_dashboard(args.url, args.refresh_events))


if __name__ == "__main__":
    main()
