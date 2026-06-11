"""Mock connector for local development and tests.

The mock connector will emit deterministic synthetic events into the raw event
queue, making it possible to exercise the gateway without connecting to TikTok.
"""

from __future__ import annotations

from collections.abc import Iterable
from dataclasses import dataclass, field
from datetime import datetime, timezone
from itertools import cycle
from typing import Any

from tiktok_event_gateway.connectors.base import BaseConnector
from tiktok_event_gateway.models import EventType

MOCK_EVENT_TYPES = frozenset(
    {
        EventType.COMMENT,
        EventType.JOIN,
        EventType.GIFT,
        EventType.LIKE,
        EventType.FOLLOW,
    }
)


@dataclass(frozen=True)
class MockConnectorConfig:
    """Configuration for the local mock event connector."""

    interval_seconds: float = 1.0
    enabled_event_types: tuple[EventType, ...] = field(
        default_factory=lambda: (
            EventType.COMMENT,
            EventType.JOIN,
            EventType.GIFT,
            EventType.LIKE,
            EventType.FOLLOW,
        )
    )
    username: str = "mock_viewer"
    display_name: str = "Mock Viewer"
    user_id: str = "mock-user-001"
    room_id: str = "mock-room"
    stream_id: str = "mock-stream"

    @classmethod
    def from_values(
        cls,
        *,
        interval_seconds: float = 1.0,
        enabled_event_types: Iterable[EventType | str] | None = None,
        username: str = "mock_viewer",
        display_name: str = "Mock Viewer",
        user_id: str = "mock-user-001",
        room_id: str = "mock-room",
        stream_id: str = "mock-stream",
    ) -> "MockConnectorConfig":
        """Build and validate config from primitive values."""
        event_types = (
            tuple(_coerce_event_type(event_type) for event_type in enabled_event_types)
            if enabled_event_types is not None
            else cls().enabled_event_types
        )
        config = cls(
            interval_seconds=interval_seconds,
            enabled_event_types=event_types,
            username=username,
            display_name=display_name,
            user_id=user_id,
            room_id=room_id,
            stream_id=stream_id,
        )
        config.validate()
        return config

    def validate(self) -> None:
        """Validate mock connector settings."""
        if self.interval_seconds <= 0:
            msg = "interval_seconds must be greater than zero"
            raise ValueError(msg)
        if not self.enabled_event_types:
            msg = "enabled_event_types must include at least one event type"
            raise ValueError(msg)
        unsupported = set(self.enabled_event_types) - MOCK_EVENT_TYPES
        if unsupported:
            names = ", ".join(sorted(event_type.value for event_type in unsupported))
            msg = f"mock connector does not emit: {names}"
            raise ValueError(msg)


class MockConnector(BaseConnector):
    """Async connector that periodically emits deterministic fake raw events."""

    def __init__(
        self,
        config: MockConnectorConfig | None = None,
        *,
        max_queue_size: int = 1000,
    ) -> None:
        self.config = config or MockConnectorConfig()
        self.config.validate()
        self._sequence = 0
        super().__init__(name="mock", max_queue_size=max_queue_size)

    async def _run(self) -> None:
        event_type_cycle = cycle(self.config.enabled_event_types)
        while not self.stop_requested():
            event_type = next(event_type_cycle)
            self._sequence += 1
            await self.emit(self._build_event(event_type, self._sequence))
            await self.wait_until_stopped(timeout=self.config.interval_seconds)

    def _build_event(self, event_type: EventType, sequence: int) -> dict[str, Any]:
        event: dict[str, Any] = {
            "type": event_type.value,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "connector": "mock",
            "room_id": self.config.room_id,
            "stream_id": self.config.stream_id,
            "sequence": sequence,
        }

        if event_type in {
            EventType.COMMENT,
            EventType.JOIN,
            EventType.GIFT,
            EventType.LIKE,
            EventType.FOLLOW,
        }:
            event["user"] = self._user(sequence)

        if event_type == EventType.COMMENT:
            event["comment"] = f"Mock comment #{sequence}"
        elif event_type == EventType.JOIN:
            event["viewer_count"] = 100 + sequence
        elif event_type == EventType.GIFT:
            event["gift"] = {"id": "mock-gift", "name": "Mock Gift"}
            event["repeat_count"] = 1 + (sequence % 3)
            event["diamond_count"] = 1
            event["repeat_end"] = True
        elif event_type == EventType.LIKE:
            event["like_count"] = 1 + (sequence % 5)
            event["total_like_count"] = sequence * 10
        elif event_type == EventType.FOLLOW:
            event["followed"] = True
        return event

    def _user(self, sequence: int) -> dict[str, Any]:
        suffix = f"{sequence:03d}"
        return {
            "id": f"{self.config.user_id}-{suffix}",
            "unique_id": self.config.username,
            "nickname": self.config.display_name,
        }


def _coerce_event_type(value: EventType | str) -> EventType:
    if isinstance(value, EventType):
        return value
    try:
        return EventType(value)
    except ValueError as exc:
        msg = f"unknown event type {value!r}"
        raise ValueError(msg) from exc
