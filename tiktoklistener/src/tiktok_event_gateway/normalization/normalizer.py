"""Raw event normalization pipeline.

The normalizer converts connector-produced raw envelopes into stable Pydantic
gateway events. It must not depend on TikTokLive classes or client application
rules.
"""

from __future__ import annotations

import logging
from collections.abc import AsyncIterable, AsyncIterator, Mapping
from datetime import datetime, timezone
from typing import Any, Protocol

from tiktok_event_gateway.models import (
    CommentEvent,
    CommentPayload,
    ConnectEvent,
    ConnectPayload,
    DisconnectEvent,
    DisconnectPayload,
    ErrorEvent,
    ErrorPayload,
    EventType,
    FollowEvent,
    FollowPayload,
    GatewayEvent,
    GiftEvent,
    GiftPayload,
    JoinEvent,
    JoinPayload,
    LikeEvent,
    LikePayload,
    Source,
    SourcePlatform,
    User,
)

logger = logging.getLogger(__name__)

RawEvent = Mapping[str, Any] | object

_MISSING = object()


class EventNormalizer(Protocol):
    """Protocol implemented by provider-specific event normalizers."""

    def normalize(self, raw_event: RawEvent) -> GatewayEvent:
        """Convert one provider event into a normalized gateway event."""
        ...


async def normalize_event_stream(
    raw_events: AsyncIterable[RawEvent],
    normalizer: EventNormalizer,
) -> AsyncIterator[GatewayEvent]:
    """Yield normalized events from any async raw event stream."""
    async for raw_event in raw_events:
        yield normalizer.normalize(raw_event)


class TikTokEventNormalizer:
    """Normalize TikTok-like events into the gateway event schema.

    This class intentionally does not import TikTokLive. It accepts dictionaries
    and object-like values so the connector can pass sanitized provider events
    or, during tests, fake TikTok-like objects.
    """

    def __init__(
        self,
        *,
        include_raw_event: bool = False,
        connector_name: str = "tiktok-live",
        room_id: str | None = None,
        stream_id: str | None = None,
    ) -> None:
        self.include_raw_event = include_raw_event
        self.default_source = Source(
            platform=SourcePlatform.TIKTOK,
            connector=connector_name,
            room_id=room_id,
            stream_id=stream_id,
        )

    def normalize(self, raw_event: RawEvent) -> GatewayEvent:
        """Convert a raw TikTok-like event into a gateway event.

        Invalid or unsupported input is converted into an error event so one bad
        provider message does not crash the service pipeline.
        """
        raw_snapshot = _to_raw_debug_dict(raw_event)
        raw_for_debug = raw_snapshot if self.include_raw_event else None

        try:
            event_type = self._detect_event_type(raw_event)
            source = self._build_source(raw_event)
            timestamp = _extract_timestamp(raw_event)

            if event_type == EventType.COMMENT:
                return CommentEvent(
                    timestamp=timestamp,
                    source=source,
                    user=self._build_user(raw_event),
                    payload=CommentPayload(comment=_required_text(raw_event, "comment", "text")),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.JOIN:
                return JoinEvent(
                    timestamp=timestamp,
                    source=source,
                    user=self._build_user(raw_event),
                    payload=JoinPayload(viewer_count=_optional_int(raw_event, "viewer_count")),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.GIFT:
                gift = _read(raw_event, "gift", default={})
                return GiftEvent(
                    timestamp=timestamp,
                    source=source,
                    user=self._build_user(raw_event),
                    payload=GiftPayload(
                        gift_id=_required_text(raw_event, "gift_id", nested=(gift, "id")),
                        gift_name=_required_text(raw_event, "gift_name", nested=(gift, "name")),
                        repeat_count=_optional_int(raw_event, "repeat_count") or 1,
                        diamond_count=_optional_int(raw_event, "diamond_count"),
                        repeat_end=_optional_bool(raw_event, "repeat_end"),
                    ),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.LIKE:
                return LikeEvent(
                    timestamp=timestamp,
                    source=source,
                    user=self._build_user(raw_event),
                    payload=LikePayload(
                        like_count=_optional_int(raw_event, "like_count", "count") or 1,
                        total_like_count=_optional_int(raw_event, "total_like_count", "total"),
                    ),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.FOLLOW:
                return FollowEvent(
                    timestamp=timestamp,
                    source=source,
                    user=self._build_user(raw_event),
                    payload=FollowPayload(
                        followed=_optional_bool(raw_event, "followed") is not False
                    ),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.CONNECT:
                return ConnectEvent(
                    timestamp=timestamp,
                    source=source,
                    payload=ConnectPayload(
                        connection_id=_optional_text(raw_event, "connection_id"),
                        message=_optional_text(raw_event, "message"),
                    ),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.DISCONNECT:
                return DisconnectEvent(
                    timestamp=timestamp,
                    source=source,
                    payload=DisconnectPayload(
                        reason=_optional_text(raw_event, "reason"),
                        code=_optional_text(raw_event, "code"),
                    ),
                    raw_event=raw_for_debug,
                )
            if event_type == EventType.ERROR:
                return ErrorEvent(
                    timestamp=timestamp,
                    source=source,
                    payload=ErrorPayload(
                        message=_optional_text(raw_event, "message", "error") or "provider error",
                        code=_optional_text(raw_event, "code"),
                        recoverable=_optional_bool(raw_event, "recoverable") is not False,
                    ),
                    raw_event=raw_for_debug,
                )

            msg = f"unsupported event type {event_type!s}"
            raise NormalizationError(msg)
        except Exception as exc:
            logger.warning("failed to normalize TikTok event", exc_info=True)
            return self._error_event(
                message=str(exc) or exc.__class__.__name__,
                raw_event=raw_for_debug,
            )

    def _detect_event_type(self, raw_event: RawEvent) -> EventType:
        type_value = _optional_text(
            raw_event,
            "event_type",
            "type",
            "kind",
            "event",
            "name",
            "__event_type__",
        )
        if type_value is None:
            type_value = raw_event.__class__.__name__

        normalized = _normalize_type_name(type_value)
        aliases = {
            "chat": EventType.COMMENT,
            "comment": EventType.COMMENT,
            "commentevent": EventType.COMMENT,
            "joinevent": EventType.JOIN,
            "member": EventType.JOIN,
            "memberevent": EventType.JOIN,
            "join": EventType.JOIN,
            "gift": EventType.GIFT,
            "giftevent": EventType.GIFT,
            "like": EventType.LIKE,
            "likeevent": EventType.LIKE,
            "follow": EventType.FOLLOW,
            "followevent": EventType.FOLLOW,
            "connect": EventType.CONNECT,
            "connectevent": EventType.CONNECT,
            "connected": EventType.CONNECT,
            "disconnect": EventType.DISCONNECT,
            "disconnectevent": EventType.DISCONNECT,
            "disconnected": EventType.DISCONNECT,
            "error": EventType.ERROR,
            "errorevent": EventType.ERROR,
        }

        if normalized in aliases:
            return aliases[normalized]

        # Test doubles and some provider wrapper classes often include prefixes
        # around the meaningful TikTok event name, for example
        # ``FakeCommentEvent``. Match the longest known suffix first so
        # ``DisconnectEvent`` is not accidentally classified as ``ConnectEvent``.
        for alias in sorted(aliases, key=len, reverse=True):
            if normalized.endswith(alias):
                return aliases[alias]

        msg = f"unknown TikTok event type {type_value!r}"
        raise NormalizationError(msg)

    def _build_source(self, raw_event: RawEvent) -> Source:
        return Source(
            platform=SourcePlatform.TIKTOK,
            connector=_optional_text(raw_event, "connector") or self.default_source.connector,
            room_id=_optional_text(raw_event, "room_id") or self.default_source.room_id,
            stream_id=_optional_text(raw_event, "stream_id") or self.default_source.stream_id,
        )

    def _build_user(self, raw_event: RawEvent) -> User:
        raw_user = _read(raw_event, "user", "author", default=_MISSING)
        source = raw_user if raw_user is not _MISSING else raw_event
        user_id = _optional_text(source, "user_id", "id", "unique_id", "sec_uid", "userId")

        if user_id is None:
            raise NormalizationError("missing required user identifier")

        return User(
            user_id=user_id,
            username=_optional_text(source, "username", "unique_id", "nickname"),
            display_name=_optional_text(source, "display_name", "nickname", "name"),
            avatar_url=_optional_text(source, "avatar_url", "profile_picture_url"),
            is_moderator=_optional_bool(source, "is_moderator", "moderator"),
            is_subscriber=_optional_bool(source, "is_subscriber", "subscriber"),
        )

    def _error_event(self, *, message: str, raw_event: dict[str, Any] | None) -> ErrorEvent:
        return ErrorEvent(
            source=self.default_source,
            payload=ErrorPayload(
                message=f"failed to normalize TikTok event: {message}",
                code="normalization_error",
                recoverable=True,
            ),
            raw_event=raw_event,
        )


class NormalizationError(ValueError):
    """Raised when a raw event cannot be converted into a gateway event."""


def _read(raw_event: RawEvent, *names: str, default: Any = None) -> Any:
    """Read the first matching field from a dict-like or object-like event."""
    for name in names:
        if isinstance(raw_event, Mapping) and name in raw_event:
            return raw_event[name]
        if not isinstance(raw_event, Mapping) and hasattr(raw_event, name):
            return getattr(raw_event, name)
    return default


def _optional_text(raw_event: RawEvent, *names: str) -> str | None:
    value = _read(raw_event, *names, default=None)
    if value is None:
        return None
    text = str(value).strip()
    return text or None


def _required_text(raw_event: RawEvent, *names: str, nested: tuple[Any, str] | None = None) -> str:
    value = _optional_text(raw_event, *names)
    if value is None and nested is not None:
        nested_source, nested_name = nested
        value = _optional_text(nested_source, nested_name)
    if value is None:
        msg = f"missing required text field: {'/'.join(names)}"
        raise NormalizationError(msg)
    return value


def _optional_int(raw_event: RawEvent, *names: str) -> int | None:
    value = _read(raw_event, *names, default=None)
    if value is None or value == "":
        return None
    try:
        return int(value)
    except (TypeError, ValueError) as exc:
        msg = f"invalid integer field {names[0]!r}: {value!r}"
        raise NormalizationError(msg) from exc


def _optional_bool(raw_event: RawEvent, *names: str) -> bool | None:
    value = _read(raw_event, *names, default=None)
    if value is None:
        return None
    if isinstance(value, bool):
        return value
    if isinstance(value, int):
        return bool(value)
    if isinstance(value, str):
        normalized = value.strip().lower()
        if normalized in {"1", "true", "yes", "y"}:
            return True
        if normalized in {"0", "false", "no", "n"}:
            return False
    msg = f"invalid boolean field {names[0]!r}: {value!r}"
    raise NormalizationError(msg)


def _extract_timestamp(raw_event: RawEvent) -> datetime:
    value = _read(
        raw_event,
        "timestamp",
        "timestamp_ms",
        "created_at",
        "create_time",
        "event_time",
        default=None,
    )
    if value is None:
        return datetime.now(timezone.utc)
    if isinstance(value, datetime):
        timestamp = value
    elif isinstance(value, int | float):
        numeric = float(value)
        timestamp = datetime.fromtimestamp(numeric / 1000 if numeric > 10_000_000_000 else numeric)
    elif isinstance(value, str):
        timestamp = _parse_timestamp_string(value)
    else:
        msg = f"unsupported timestamp value: {value!r}"
        raise NormalizationError(msg)

    if timestamp.tzinfo is None:
        timestamp = timestamp.replace(tzinfo=timezone.utc)
    return timestamp.astimezone(timezone.utc)


def _parse_timestamp_string(value: str) -> datetime:
    text = value.strip()
    if not text:
        msg = "timestamp cannot be blank"
        raise NormalizationError(msg)
    if text.isdigit():
        return _extract_timestamp({"timestamp": int(text)})
    try:
        return datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as exc:
        msg = f"invalid timestamp value: {value!r}"
        raise NormalizationError(msg) from exc


def _normalize_type_name(value: str) -> str:
    return "".join(character for character in value.lower() if character.isalnum())


def _to_json_compatible(value: Any, *, depth: int = 0) -> Any:
    """Best-effort conversion of provider data into debug-safe JSON values."""
    if depth > 5:
        return repr(value)
    if value is None or isinstance(value, str | int | float | bool):
        return value
    if isinstance(value, datetime):
        timestamp = value if value.tzinfo else value.replace(tzinfo=timezone.utc)
        return timestamp.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")
    if isinstance(value, Mapping):
        return {
            str(key): _to_json_compatible(item, depth=depth + 1)
            for key, item in value.items()
            if not str(key).startswith("_")
        }
    if isinstance(value, list | tuple | set):
        return [_to_json_compatible(item, depth=depth + 1) for item in value]
    if hasattr(value, "model_dump"):
        return _to_json_compatible(value.model_dump(), depth=depth + 1)
    if hasattr(value, "__dict__"):
        return {
            str(key): _to_json_compatible(item, depth=depth + 1)
            for key, item in vars(value).items()
            if not str(key).startswith("_")
        }
    return repr(value)


def _to_raw_debug_dict(value: Any) -> dict[str, Any]:
    converted = _to_json_compatible(value)
    if isinstance(converted, dict):
        return converted
    return {"value": converted}
