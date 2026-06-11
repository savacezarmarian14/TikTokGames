"""TikTokLive connector implementation.

This module is the isolation boundary for the optional TikTokLive dependency.
TikTokLive raw event objects must be adapted here and must not be exposed to
normalization, broker, transport, or client-facing modules.
"""

from __future__ import annotations

import asyncio
import importlib
import inspect
from collections.abc import Iterable
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any, Callable

import structlog

from tiktok_event_gateway.connectors.base import BaseConnector
from tiktok_event_gateway.models import EventType
from tiktok_event_gateway.normalization import TikTokEventNormalizer
from tiktok_event_gateway.utils.tiktok import build_tiktok_live_url, normalize_tiktok_username

logger = structlog.get_logger(__name__)

TIKTOK_CONNECTOR_EVENT_TYPES = frozenset(
    {
        EventType.COMMENT,
        EventType.JOIN,
        EventType.GIFT,
        EventType.LIKE,
        EventType.FOLLOW,
        EventType.CONNECT,
        EventType.DISCONNECT,
        EventType.ERROR,
    }
)


@dataclass(frozen=True)
class TikTokLiveConnectorConfig:
    """Configuration for the optional TikTokLive connector."""

    live_username: str
    reconnect_enabled: bool = True
    reconnect_delay_seconds: float = 5.0
    include_raw_event_debug: bool = False
    enabled_event_types: tuple[EventType, ...] = field(
        default_factory=lambda: (
            EventType.COMMENT,
            EventType.JOIN,
            EventType.GIFT,
            EventType.LIKE,
            EventType.FOLLOW,
            EventType.CONNECT,
            EventType.DISCONNECT,
            EventType.ERROR,
        )
    )

    @classmethod
    def from_values(
        cls,
        *,
        live_username: str,
        reconnect_enabled: bool = True,
        reconnect_delay_seconds: float = 5.0,
        include_raw_event_debug: bool = False,
        enabled_event_types: Iterable[EventType | str] | None = None,
    ) -> "TikTokLiveConnectorConfig":
        """Build and validate config from primitive values."""
        event_types = (
            tuple(_coerce_event_type(event_type) for event_type in enabled_event_types)
            if enabled_event_types is not None
            else cls(live_username=live_username).enabled_event_types
        )
        config = cls(
            live_username=normalize_tiktok_username(live_username),
            reconnect_enabled=reconnect_enabled,
            reconnect_delay_seconds=reconnect_delay_seconds,
            include_raw_event_debug=include_raw_event_debug,
            enabled_event_types=event_types,
        )
        config.validate()
        return config

    def validate(self) -> None:
        """Validate connector settings."""
        if not normalize_tiktok_username(self.live_username):
            msg = "live_username is required"
            raise ValueError(msg)
        if self.reconnect_delay_seconds < 0:
            msg = "reconnect_delay_seconds must be zero or greater"
            raise ValueError(msg)
        if not self.enabled_event_types:
            msg = "enabled_event_types must include at least one event type"
            raise ValueError(msg)
        unsupported = set(self.enabled_event_types) - TIKTOK_CONNECTOR_EVENT_TYPES
        if unsupported:
            names = ", ".join(sorted(event_type.value for event_type in unsupported))
            msg = f"TikTokLive connector does not emit: {names}"
            raise ValueError(msg)


class TikTokLiveDependencyError(RuntimeError):
    """Raised when the optional TikTokLive dependency is unavailable."""


class TikTokLiveConnector(BaseConnector):
    """Connector that adapts TikTokLive events into gateway raw dictionaries."""

    def __init__(
        self,
        config: TikTokLiveConnectorConfig,
        *,
        max_queue_size: int = 1000,
        client_factory: Callable[[str], Any] | None = None,
    ) -> None:
        config.validate()
        self.config = TikTokLiveConnectorConfig.from_values(
            live_username=config.live_username,
            reconnect_enabled=config.reconnect_enabled,
            reconnect_delay_seconds=config.reconnect_delay_seconds,
            include_raw_event_debug=config.include_raw_event_debug,
            enabled_event_types=config.enabled_event_types,
        )
        self._client_factory = client_factory
        self._client: Any | None = None
        super().__init__(name="tiktok-live", max_queue_size=max_queue_size)

    def normalizer(self) -> TikTokEventNormalizer:
        """Create the normalizer configured for this connector."""
        return TikTokEventNormalizer(
            include_raw_event=self.config.include_raw_event_debug,
            connector_name="tiktok-live",
        )

    async def stop(self) -> None:
        """Stop the TikTokLive client and connector task."""
        await self._disconnect_client()
        await super().stop()

    async def _run(self) -> None:
        while not self.stop_requested():
            try:
                await self._connect_once()
            except asyncio.CancelledError:
                raise
            except TikTokLiveDependencyError:
                logger.exception("TikTokLive dependency is not available")
                await self.emit(_error_raw_event("TikTokLive dependency is not installed"))
                return
            except Exception as exc:
                logger.exception("TikTokLive connector failed")
                await self.emit(_error_raw_event(str(exc) or exc.__class__.__name__))

            if not self.config.reconnect_enabled or self.stop_requested():
                break

            logger.info(
                "TikTokLive reconnect scheduled",
                delay_seconds=self.config.reconnect_delay_seconds,
            )
            await self.wait_until_stopped(timeout=self.config.reconnect_delay_seconds)

    async def _connect_once(self) -> None:
        dependencies = _load_tiktoklive_dependencies()
        client = self._create_client(dependencies.client_class)
        self._client = client
        self._register_event_handlers(client, dependencies.event_classes)

        if await self._is_live_preflight(client) is False:
            logger.info(
                "TikTokLive user is not live",
                live_username=f"@{self.config.live_username}",
                live_url=build_tiktok_live_url(self.config.live_username),
            )
            return

        logger.info(
            "TikTokLive connecting",
            live_username=f"@{self.config.live_username}",
            live_url=build_tiktok_live_url(self.config.live_username),
        )
        await self._start_client(client)
        logger.info(
            "TikTokLive connection closed",
            live_username=f"@{self.config.live_username}",
        )

    def _create_client(self, client_class: type[Any]) -> Any:
        # TikTokLive expects unique_id without a leading @ in recent versions.
        # Passing @username makes the library build internal referer URLs like
        # https://www.tiktok.com/@@username/live even though our public URL is
        # normalized correctly. Keep @ only for display/logging boundaries.
        unique_id = self.config.live_username
        if self._client_factory is not None:
            return self._client_factory(unique_id)
        return client_class(unique_id=unique_id)

    def _register_event_handlers(
        self,
        client: Any,
        event_classes: dict[EventType, type[Any] | None],
    ) -> None:
        for event_type in self.config.enabled_event_types:
            event_class = event_classes.get(event_type)
            if event_class is None:
                log_method = logger.debug if event_type == EventType.ERROR else logger.warning
                log_method(
                    "TikTokLive event class unavailable",
                    event_type=event_type.value,
                )
                continue
            self._register_handler(client, event_class, self._handler_for(event_type))

    def _register_handler(
        self,
        client: Any,
        event_class: type[Any],
        handler: Callable[[Any], Any],
    ) -> None:
        if hasattr(client, "add_listener"):
            client.add_listener(event_class, handler)
            return
        if hasattr(client, "on"):
            client.on(event_class)(handler)
            return
        msg = "TikTokLive client does not expose add_listener() or on()"
        raise RuntimeError(msg)

    def _handler_for(self, event_type: EventType) -> Callable[[Any], Any]:
        async def handle(event: Any) -> None:
            try:
                raw_event = _adapt_tiktok_event(
                    event_type,
                    event,
                    include_provider_snapshot=self.config.include_raw_event_debug,
                )
                if event_type == EventType.GIFT and _is_intermediate_gift_streak(raw_event):
                    logger.debug(
                        "TikTokLive intermediate gift streak skipped",
                        gift_name=raw_event.get("gift", {}).get("name"),
                        repeat_count=raw_event.get("repeat_count"),
                    )
                    return
                await self.emit(raw_event)
            except Exception as exc:
                logger.exception(
                    "failed to adapt TikTokLive event",
                    event_type=event_type.value,
                )
                await self.emit(_error_raw_event(str(exc) or exc.__class__.__name__))

        return handle

    async def _start_client(self, client: Any) -> None:
        # In TikTokLive, connect() is the correct coroutine-style lifecycle API:
        # it remains pending until the connection closes. start() may be
        # non-blocking and return an asyncio.Task, so using it first can create a
        # false reconnect loop and eventually hit TikTokLive rate limits.
        connect_method = getattr(client, "connect", None)
        if connect_method is not None:
            logger.info("TikTokLive connect() selected")
            await _call_client_method(connect_method)
            return

        start_method = getattr(client, "start", None)
        if start_method is not None:
            logger.info("TikTokLive start() selected")
            await _call_client_method(start_method)
            return

        run_method = getattr(client, "run", None)
        if run_method is not None:
            logger.info("TikTokLive run() selected")
            await _call_client_method(run_method, run_in_thread=True)
            return

        msg = "TikTokLive client does not expose connect(), start(), or run()"
        raise RuntimeError(msg)

    async def _is_live_preflight(self, client: Any) -> bool | None:
        is_live_method = getattr(client, "is_live", None)
        if is_live_method is None:
            return None
        try:
            result = await _call_client_method(is_live_method)
        except Exception as exc:
            logger.warning(
                "TikTokLive is_live() preflight failed; continuing with connect",
                live_username=f"@{self.config.live_username}",
                error=str(exc) or exc.__class__.__name__,
            )
            return None
        if isinstance(result, bool):
            return result
        return None

    async def _disconnect_client(self) -> None:
        client = self._client
        self._client = None
        if client is None:
            return
        for method_name in ("disconnect", "stop", "close"):
            method = getattr(client, method_name, None)
            if method is None:
                continue
            await _call_client_method(method)
            logger.info(
                "TikTokLive disconnected",
                live_username=f"@{self.config.live_username}",
            )
            return


@dataclass(frozen=True)
class _TikTokLiveDependencies:
    client_class: type[Any]
    event_classes: dict[EventType, type[Any] | None]


def _load_tiktoklive_dependencies() -> _TikTokLiveDependencies:
    """Import TikTokLive symbols lazily and only inside this module."""
    try:
        from TikTokLive import TikTokLiveClient

        tiktok_events = importlib.import_module("TikTokLive.events")
    except ImportError as exc:
        raise TikTokLiveDependencyError(
            'Install the optional TikTokLive dependency with: pip install -e ".[tiktok]"'
        ) from exc

    return _TikTokLiveDependencies(
        client_class=TikTokLiveClient,
        event_classes={
            EventType.COMMENT: _event_class(tiktok_events, "CommentEvent"),
            EventType.JOIN: _event_class(tiktok_events, "JoinEvent", "MemberEvent"),
            EventType.GIFT: _event_class(tiktok_events, "GiftEvent"),
            EventType.LIKE: _event_class(tiktok_events, "LikeEvent"),
            EventType.FOLLOW: _event_class(tiktok_events, "FollowEvent"),
            EventType.CONNECT: _event_class(tiktok_events, "ConnectEvent", "ConnectedEvent"),
            EventType.DISCONNECT: _event_class(
                tiktok_events,
                "DisconnectEvent",
                "DisconnectedEvent",
            ),
            EventType.ERROR: _event_class(tiktok_events, "ErrorEvent"),
        },
    )


def _event_class(module: Any, *names: str) -> type[Any] | None:
    for name in names:
        value = getattr(module, name, None)
        if isinstance(value, type):
            return value
    return None


async def _call_client_method(
    method: Callable[[], Any],
    *,
    run_in_thread: bool = False,
) -> Any:
    """Call a possibly-sync TikTokLive lifecycle method and await its result.

    TikTokLive.start() can return an asyncio.Task. The old implementation
    awaited only the start() coroutine itself and then returned immediately,
    leaving the background task alive while the connector scheduled a reconnect.
    This helper always awaits a returned awaitable too.
    """
    if inspect.iscoroutinefunction(method):
        result = await method()
    elif run_in_thread:
        result = await asyncio.to_thread(method)
    else:
        result = method()

    if inspect.isawaitable(result):
        return await result
    return result


def _adapt_tiktok_event(
    event_type: EventType,
    event: Any,
    *,
    include_provider_snapshot: bool = False,
) -> dict[str, Any]:
    raw: dict[str, Any] = {
        "type": event_type.value,
        "timestamp": _timestamp(event),
        "connector": "tiktok-live",
    }

    room_id = _text(event, "room_id", "roomId")
    if room_id is not None:
        raw["room_id"] = room_id
    stream_id = _text(event, "stream_id", "streamId")
    if stream_id is not None:
        raw["stream_id"] = stream_id

    user = _adapt_user(_read(event, "user_info", "user", "author", default=None))
    if user is not None:
        raw["user"] = user

    if event_type == EventType.COMMENT:
        raw["comment"] = _text(event, "comment", "text") or ""
    elif event_type == EventType.JOIN:
        viewer_count = _int(event, "viewer_count", "viewerCount")
        if viewer_count is not None:
            raw["viewer_count"] = viewer_count
    elif event_type == EventType.GIFT:
        gift = _read(event, "gift", default=None)
        raw["gift"] = {
            "id": _text(event, "gift_id", nested=(gift, "id")) or "unknown",
            "name": _text(event, "gift_name", nested=(gift, "name")) or "Unknown Gift",
        }
        raw["repeat_count"] = _int(event, "repeat_count", "repeatCount") or 1
        diamond_count = _int(event, "diamond_count", "diamondCount")
        if diamond_count is not None:
            raw["diamond_count"] = diamond_count
        repeat_end = _bool(event, "repeat_end", "repeatEnd")
        if repeat_end is not None:
            raw["repeat_end"] = repeat_end
        streaking = _bool(event, "streaking", "is_streaking", "isStreaking")
        if streaking is not None:
            raw["streaking"] = streaking
    elif event_type == EventType.LIKE:
        raw["like_count"] = _int(event, "like_count", "count") or 1
        total = _int(event, "total_like_count", "totalLikeCount", "total")
        if total is not None:
            raw["total_like_count"] = total
    elif event_type == EventType.FOLLOW:
        raw["followed"] = True
    elif event_type == EventType.CONNECT:
        raw["message"] = _text(event, "message") or "connected"
        connection_id = _text(event, "connection_id", "client_id")
        if connection_id is not None:
            raw["connection_id"] = connection_id
    elif event_type == EventType.DISCONNECT:
        raw["reason"] = _text(event, "reason") or "disconnected"
        code = _text(event, "code")
        if code is not None:
            raw["code"] = code
    elif event_type == EventType.ERROR:
        raw["message"] = _text(event, "message", "error") or repr(event)
        code = _text(event, "code")
        if code is not None:
            raw["code"] = code

    if include_provider_snapshot:
        raw["provider"] = _safe_snapshot(event)
    return raw


def _adapt_user(user: Any) -> dict[str, Any] | None:
    if user is None:
        return None
    user_id = _text(user, "user_id", "id", "unique_id", "sec_uid", "userId")
    if user_id is None:
        return None
    adapted = {
        "id": user_id,
        "unique_id": _text(user, "username", "unique_id", "nickname"),
        "nickname": _text(user, "display_name", "nickname", "name"),
    }
    avatar_url = _text(user, "avatar_url", "profile_picture_url")
    if avatar_url is not None:
        adapted["avatar_url"] = avatar_url
    moderator = _bool(user, "is_moderator", "moderator")
    if moderator is not None:
        adapted["moderator"] = moderator
    subscriber = _bool(user, "is_subscriber", "subscriber")
    if subscriber is not None:
        adapted["subscriber"] = subscriber
    return {key: value for key, value in adapted.items() if value is not None}


def _error_raw_event(message: str) -> dict[str, Any]:
    return {
        "type": "error",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "connector": "tiktok-live",
        "message": message,
        "code": "tiktok_live_connector_error",
        "recoverable": True,
    }


def _is_intermediate_gift_streak(raw_event: dict[str, Any]) -> bool:
    """Return true for TikTok gift combo updates that are not final yet.

    TikTokLive can emit a gift streak several times as the repeat count grows,
    for example 1, 6, 9, 20, then 75. Gateway consumers should receive one
    normalized gift event for the final streak value, not every partial update.
    """
    repeat_end = raw_event.get("repeat_end")
    if repeat_end is False:
        return True

    streaking = raw_event.get("streaking")
    return streaking is True


def _timestamp(event: Any) -> str:
    value = _read(
        event,
        "timestamp",
        "timestamp_ms",
        "created_at",
        "create_time",
        "event_time",
        default=None,
    )
    if isinstance(value, datetime):
        timestamp = value if value.tzinfo else value.replace(tzinfo=timezone.utc)
        return timestamp.astimezone(timezone.utc).isoformat()
    if value is not None:
        return str(value)
    return datetime.now(timezone.utc).isoformat()


def _read(source: Any, *names: str, default: Any = None) -> Any:
    if source is None:
        return default

    for name in names:
        if isinstance(source, dict):
            if name in source:
                return source[name]
            continue

        try:
            return getattr(source, name)
        except AttributeError:
            continue
        except Exception as exc:
            logger.debug(
                "failed to read provider attribute",
                attribute=name,
                source_type=type(source).__name__,
                error=str(exc),
            )
            continue

    return default


def _text(source: Any, *names: str, nested: tuple[Any, str] | None = None) -> str | None:
    value = _read(source, *names, default=None)
    if value is None and nested is not None:
        nested_source, nested_name = nested
        value = _read(nested_source, nested_name, default=None)
    if value is None:
        return None
    text = str(value).strip()
    return text or None


def _int(source: Any, *names: str) -> int | None:
    value = _read(source, *names, default=None)
    if value is None or value == "":
        return None
    try:
        return int(value)
    except (TypeError, ValueError):
        return None


def _bool(source: Any, *names: str) -> bool | None:
    value = _read(source, *names, default=None)
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
    return None


def _safe_snapshot(value: Any, *, depth: int = 0) -> Any:
    if depth > 4:
        return repr(value)
    if value is None or isinstance(value, str | int | float | bool):
        return value
    if isinstance(value, datetime):
        timestamp = value if value.tzinfo else value.replace(tzinfo=timezone.utc)
        return timestamp.astimezone(timezone.utc).isoformat()
    if isinstance(value, dict):
        return {
            str(key): _safe_snapshot(item, depth=depth + 1)
            for key, item in value.items()
            if not str(key).startswith("_")
        }
    if isinstance(value, list | tuple | set):
        return [_safe_snapshot(item, depth=depth + 1) for item in value]
    if hasattr(value, "__dict__"):
        return {
            str(key): _safe_snapshot(item, depth=depth + 1)
            for key, item in vars(value).items()
            if not str(key).startswith("_")
        }
    return repr(value)


def _coerce_event_type(value: EventType | str) -> EventType:
    if isinstance(value, EventType):
        return value
    try:
        return EventType(value)
    except ValueError as exc:
        msg = f"unknown event type {value!r}"
        raise ValueError(msg) from exc
