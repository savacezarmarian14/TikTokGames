"""Small in-memory event ID deduplication cache."""

from __future__ import annotations

from collections import OrderedDict


class EventIdDeduplicator:
    """LRU-style cache for processed event IDs."""

    def __init__(self, max_size: int = 10000) -> None:
        if max_size <= 0:
            msg = "deduplication max_size must be greater than zero"
            raise ValueError(msg)
        self.max_size = max_size
        self._seen: OrderedDict[str, None] = OrderedDict()

    def seen_or_add(self, event_id: str | None) -> bool:
        """Return True if event_id was already seen; otherwise store it."""
        if not event_id:
            return False
        if event_id in self._seen:
            self._seen.move_to_end(event_id)
            return True
        self._seen[event_id] = None
        while len(self._seen) > self.max_size:
            self._seen.popitem(last=False)
        return False

    def __len__(self) -> int:
        return len(self._seen)
