"""Subscription parsing helpers for WebSocket clients."""

from __future__ import annotations

import json
from typing import Any
from urllib.parse import parse_qs, urlparse

from tiktok_event_gateway.models import EventType


def event_types_from_path(path: str) -> frozenset[EventType] | None:
    """Parse an optional event type filter from a WebSocket request path."""
    query = parse_qs(urlparse(path).query)
    values = query.get("event_types") or query.get("events")
    if not values:
        return None
    return parse_event_types(",".join(values))


def event_types_from_message(message: str | bytes) -> frozenset[EventType] | None:
    """Parse a subscription update message from a client."""
    if isinstance(message, bytes):
        message = message.decode("utf-8")
    data = json.loads(message)
    if not isinstance(data, dict):
        msg = "subscription message must be a JSON object"
        raise ValueError(msg)
    if data.get("all") is True:
        return None

    raw_event_types: Any = data.get("event_types", data.get("subscribe"))
    if raw_event_types is None:
        return None
    if isinstance(raw_event_types, str):
        return parse_event_types(raw_event_types)
    if isinstance(raw_event_types, list):
        return parse_event_types(",".join(str(item) for item in raw_event_types))

    msg = "event_types must be a string, list, or null"
    raise ValueError(msg)


def parse_event_types(value: str) -> frozenset[EventType]:
    """Parse a comma-separated event type list."""
    event_types = []
    for item in value.split(","):
        name = item.strip()
        if name:
            event_types.append(EventType(name))
    return frozenset(event_types)


def event_type_values(event_types: frozenset[EventType] | None) -> list[str] | None:
    """Return sorted event type values for logging."""
    if event_types is None:
        return None
    return sorted(event_type.value for event_type in event_types)
