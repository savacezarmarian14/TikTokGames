"""Shared enumerations for normalized gateway events.

Enums in this module describe provider-independent concepts such as event type
and provider name. They should not include game-specific meanings.
"""

from enum import Enum


class EventType(str, Enum):
    """Provider-neutral event categories emitted by the gateway."""

    COMMENT = "comment"
    JOIN = "join"
    GIFT = "gift"
    LIKE = "like"
    FOLLOW = "follow"
    CONNECT = "connect"
    DISCONNECT = "disconnect"
    ERROR = "error"
    HEARTBEAT = "heartbeat"


class SourcePlatform(str, Enum):
    """Known event source platforms.

    Values are intentionally broad so future connectors can be added without
    changing the event envelope shape.
    """

    TIKTOK = "tiktok"
    MOCK = "mock"
    GATEWAY = "gateway"
    UNKNOWN = "unknown"
