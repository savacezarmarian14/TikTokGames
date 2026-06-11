"""JSONL logging for normalized gateway events."""

from __future__ import annotations

import asyncio
from pathlib import Path

from tiktok_event_gateway.models import GatewayEvent


class EventJsonlWriter:
    """Append normalized events to a JSONL file."""

    def __init__(self, path: str | Path, *, include_raw_event: bool = False) -> None:
        self.path = Path(path)
        self.include_raw_event = include_raw_event
        self._lock = asyncio.Lock()

    async def write(self, event: GatewayEvent) -> None:
        """Append one event as a JSON line."""
        line = event.to_json(include_raw=self.include_raw_event) + "\n"
        async with self._lock:
            await asyncio.to_thread(self._append_line, line)

    def _append_line(self, line: str) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        with self.path.open("a", encoding="utf-8") as file:
            file.write(line)
