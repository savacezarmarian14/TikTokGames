"""Stable Pydantic event schemas.

This module will define the normalized JSON contract sent to client
applications. Schemas should be versioned and provider-neutral wherever
possible, while preserving useful metadata for downstream consumers.
"""

from __future__ import annotations

from datetime import datetime, timezone
from typing import Any, Literal, cast
from uuid import uuid4

from pydantic import AnyUrl, BaseModel, ConfigDict, Field, field_validator, model_validator

from tiktok_event_gateway.models.enums import EventType, SourcePlatform

SCHEMA_VERSION = "1.0"


def _new_event_id() -> str:
    """Create a stable string identifier suitable for JSON payloads."""
    return str(uuid4())


def _now_utc() -> datetime:
    """Return a timezone-aware UTC timestamp."""
    return datetime.now(timezone.utc)


def _ensure_json_compatible(value: Any, path: str = "value") -> Any:
    """Validate that a value can be represented as ordinary JSON data."""
    if value is None or isinstance(value, str | int | float | bool):
        return value
    if isinstance(value, list):
        return [_ensure_json_compatible(item, f"{path}[]") for item in value]
    if isinstance(value, dict):
        checked: dict[str, Any] = {}
        for key, item in value.items():
            if not isinstance(key, str):
                msg = f"{path} contains non-string key {key!r}"
                raise ValueError(msg)
            checked[key] = _ensure_json_compatible(item, f"{path}.{key}")
        return checked
    msg = f"{path} contains non-JSON-compatible value {value!r}"
    raise ValueError(msg)


class GatewayBaseModel(BaseModel):
    """Base Pydantic model for all gateway schema objects."""

    model_config = ConfigDict(extra="forbid", validate_assignment=True)


class ExtensiblePayload(GatewayBaseModel):
    """Base payload model that permits future provider-neutral additions."""

    model_config = ConfigDict(extra="allow", validate_assignment=True)

    @model_validator(mode="after")
    def validate_extra_values_are_json(self) -> "ExtensiblePayload":
        """Ensure extension fields remain safe for JSON clients."""
        for key, value in self.model_extra.items() if self.model_extra else ():
            _ensure_json_compatible(value, key)
        return self


class Source(GatewayBaseModel):
    """Provider information for a normalized event."""

    platform: SourcePlatform = SourcePlatform.UNKNOWN
    connector: str | None = None
    room_id: str | None = None
    stream_id: str | None = None


class User(GatewayBaseModel):
    """Provider-neutral user identity attached to user-originated events."""

    user_id: str
    username: str | None = None
    display_name: str | None = None
    avatar_url: AnyUrl | None = None
    is_moderator: bool | None = None
    is_subscriber: bool | None = None


class CommentPayload(ExtensiblePayload):
    """Payload for chat comment events."""

    comment: str = Field(min_length=1)


class JoinPayload(ExtensiblePayload):
    """Payload for viewer join events."""

    viewer_count: int | None = Field(default=None, ge=0)


class GiftPayload(ExtensiblePayload):
    """Payload for gift or donation events."""

    gift_id: str
    gift_name: str
    repeat_count: int = Field(default=1, ge=1)
    diamond_count: int | None = Field(default=None, ge=0)
    repeat_end: bool | None = None


class LikePayload(ExtensiblePayload):
    """Payload for like events."""

    like_count: int = Field(default=1, ge=1)
    total_like_count: int | None = Field(default=None, ge=0)


class FollowPayload(ExtensiblePayload):
    """Payload for follow events."""

    followed: bool = True


class ConnectPayload(ExtensiblePayload):
    """Payload for source connection events."""

    connection_id: str | None = None
    message: str | None = None


class DisconnectPayload(ExtensiblePayload):
    """Payload for source disconnection events."""

    reason: str | None = None
    code: str | None = None


class ErrorPayload(ExtensiblePayload):
    """Payload for gateway or connector error events."""

    message: str
    code: str | None = None
    recoverable: bool = True


class HeartbeatPayload(ExtensiblePayload):
    """Payload for service heartbeat events."""

    sequence: int = Field(ge=0)


class BaseEvent(GatewayBaseModel):
    """Shared envelope for every normalized gateway event."""

    version: str = SCHEMA_VERSION
    event_id: str = Field(default_factory=_new_event_id)
    event_type: EventType
    timestamp: datetime = Field(default_factory=_now_utc)
    source: Source
    user: User | None = None
    payload: ExtensiblePayload
    raw_event: dict[str, Any] | None = None

    @field_validator("timestamp")
    @classmethod
    def timestamp_must_be_timezone_aware(cls, value: datetime) -> datetime:
        """Normalize timestamps to UTC for stable client behavior."""
        if value.tzinfo is None:
            msg = "timestamp must be timezone-aware"
            raise ValueError(msg)
        return value.astimezone(timezone.utc)

    @field_validator("raw_event")
    @classmethod
    def raw_event_must_be_json_compatible(
        cls, value: dict[str, Any] | None
    ) -> dict[str, Any] | None:
        """Keep debug data JSON-compatible when explicitly attached."""
        if value is None:
            return None
        return _ensure_json_compatible(value, "raw_event")

    def to_dict(self, *, include_raw: bool = False) -> dict[str, Any]:
        """Return a JSON-friendly dictionary for client applications."""
        exclude = None if include_raw else {"raw_event"}
        return self.model_dump(mode="json", exclude_none=True, exclude=exclude)

    def to_json(self, *, include_raw: bool = False, indent: int | None = None) -> str:
        """Serialize the event as JSON for WebSocket delivery or files."""
        exclude = None if include_raw else {"raw_event"}
        return self.model_dump_json(exclude_none=True, exclude=exclude, indent=indent)


class CommentEvent(BaseEvent):
    """Normalized comment event."""

    event_type: Literal[EventType.COMMENT] = EventType.COMMENT
    user: User
    payload: CommentPayload


class JoinEvent(BaseEvent):
    """Normalized viewer join event."""

    event_type: Literal[EventType.JOIN] = EventType.JOIN
    user: User
    payload: JoinPayload = Field(default_factory=JoinPayload)


class GiftEvent(BaseEvent):
    """Normalized gift or donation event."""

    event_type: Literal[EventType.GIFT] = EventType.GIFT
    user: User
    payload: GiftPayload


class LikeEvent(BaseEvent):
    """Normalized like event."""

    event_type: Literal[EventType.LIKE] = EventType.LIKE
    user: User
    payload: LikePayload = Field(default_factory=LikePayload)


class FollowEvent(BaseEvent):
    """Normalized follow event."""

    event_type: Literal[EventType.FOLLOW] = EventType.FOLLOW
    user: User
    payload: FollowPayload = Field(default_factory=FollowPayload)


class ConnectEvent(BaseEvent):
    """Normalized source connection event."""

    event_type: Literal[EventType.CONNECT] = EventType.CONNECT
    payload: ConnectPayload = Field(default_factory=ConnectPayload)


class DisconnectEvent(BaseEvent):
    """Normalized source disconnection event."""

    event_type: Literal[EventType.DISCONNECT] = EventType.DISCONNECT
    payload: DisconnectPayload = Field(default_factory=DisconnectPayload)


class ErrorEvent(BaseEvent):
    """Normalized gateway or connector error event."""

    event_type: Literal[EventType.ERROR] = EventType.ERROR
    payload: ErrorPayload


class HeartbeatEvent(BaseEvent):
    """Normalized gateway heartbeat event."""

    event_type: Literal[EventType.HEARTBEAT] = EventType.HEARTBEAT
    payload: HeartbeatPayload


GatewayEvent = (
    CommentEvent
    | JoinEvent
    | GiftEvent
    | LikeEvent
    | FollowEvent
    | ConnectEvent
    | DisconnectEvent
    | ErrorEvent
    | HeartbeatEvent
)

EVENT_MODEL_BY_TYPE: dict[EventType, type[BaseEvent]] = {
    EventType.COMMENT: CommentEvent,
    EventType.JOIN: JoinEvent,
    EventType.GIFT: GiftEvent,
    EventType.LIKE: LikeEvent,
    EventType.FOLLOW: FollowEvent,
    EventType.CONNECT: ConnectEvent,
    EventType.DISCONNECT: DisconnectEvent,
    EventType.ERROR: ErrorEvent,
    EventType.HEARTBEAT: HeartbeatEvent,
}


def parse_gateway_event(data: str | bytes | dict[str, Any]) -> GatewayEvent:
    """Validate JSON data and return the concrete gateway event model."""
    if isinstance(data, str | bytes):
        envelope = BaseEvent.model_validate_json(data)
    else:
        envelope = BaseEvent.model_validate(data)
    event_model = EVENT_MODEL_BY_TYPE[envelope.event_type]
    if isinstance(data, str | bytes):
        return cast(GatewayEvent, event_model.model_validate_json(data))
    return cast(GatewayEvent, event_model.model_validate(data))
