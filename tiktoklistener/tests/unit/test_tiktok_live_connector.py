"""Tests for the optional TikTokLive connector boundary."""

from __future__ import annotations

import asyncio
import inspect
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Any

import pytest

from tiktok_event_gateway.connectors.tiktok_live import (
    TikTokLiveConnector,
    TikTokLiveConnectorConfig,
    TikTokLiveDependencyError,
)
from tiktok_event_gateway.models import CommentEvent, EventType, GiftEvent
from tiktok_event_gateway.normalization import normalize_event_stream


class FakeCommentEvent:
    """Fake TikTokLive CommentEvent class."""


class FakeGiftEvent:
    """Fake TikTokLive GiftEvent class."""


class FakeConnectEvent:
    """Fake TikTokLive ConnectEvent class."""


@dataclass
class FakeUser:
    id: str = "user-123"
    unique_id: str = "viewer_one"
    nickname: str = "Viewer One"
    subscriber: bool = True


@dataclass
class FakeGift:
    id: str = "5655"
    name: str = "Rose"


@dataclass
class FakeConnectPayload:
    message: str = "connected"
    timestamp: datetime = datetime(2026, 1, 2, tzinfo=timezone.utc)


@dataclass
class FakeCommentPayload:
    comment: str = "hello"
    user: FakeUser = field(default_factory=FakeUser)
    timestamp: datetime = datetime(2026, 1, 2, 3, 4, 5, tzinfo=timezone.utc)


@dataclass
class FakeGiftPayload:
    gift: FakeGift = field(default_factory=FakeGift)
    user: FakeUser = field(
        default_factory=lambda: FakeUser(id="user-456", unique_id="gifter", nickname="Gifter")
    )
    repeat_count: int = 2
    diamond_count: int = 1
    repeat_end: bool = True


@dataclass
class FakeGiftStreakPayload:
    gift: FakeGift = field(default_factory=FakeGift)
    user: FakeUser = field(
        default_factory=lambda: FakeUser(id="user-456", unique_id="gifter", nickname="Gifter")
    )
    repeat_count: int = 1
    repeat_end: bool = False
    streaking: bool = True


class FakeTikTokClient:
    """Small fake of the TikTokLive client listener surface."""

    def __init__(self, unique_id: str) -> None:
        self.unique_id = unique_id
        self.listeners: dict[type[Any], Any] = {}
        self.disconnected = False

    def add_listener(self, event_class: type[Any], handler: Any) -> None:
        self.listeners[event_class] = handler

    async def start(self) -> None:
        await self._emit(FakeConnectEvent, FakeConnectPayload())
        await self._emit(FakeCommentEvent, FakeCommentPayload())
        await self._emit(FakeGiftEvent, FakeGiftPayload())

    async def disconnect(self) -> None:
        self.disconnected = True

    async def _emit(self, event_class: type[Any], event: Any) -> None:
        handler = self.listeners.get(event_class)
        if handler is None:
            return
        result = handler(event)
        if inspect.isawaitable(result):
            await result


def dependencies():
    """Return fake TikTokLive dependency bundle."""
    from tiktok_event_gateway.connectors import tiktok_live

    return tiktok_live._TikTokLiveDependencies(
        client_class=FakeTikTokClient,
        event_classes={
            EventType.COMMENT: FakeCommentEvent,
            EventType.GIFT: FakeGiftEvent,
            EventType.CONNECT: FakeConnectEvent,
            EventType.JOIN: None,
            EventType.LIKE: None,
            EventType.FOLLOW: None,
            EventType.DISCONNECT: None,
            EventType.ERROR: None,
        },
    )


def test_tiktok_config_validation() -> None:
    with pytest.raises(ValueError, match="live_username"):
        TikTokLiveConnectorConfig.from_values(live_username="")

    with pytest.raises(ValueError, match="zero or greater"):
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            reconnect_delay_seconds=-1,
        )

    with pytest.raises(ValueError, match="unknown event type"):
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            enabled_event_types=["share"],
        )


@pytest.mark.asyncio
async def test_tiktok_connector_adapts_fake_events_without_leaking_objects(monkeypatch) -> None:
    from tiktok_event_gateway.connectors import tiktok_live

    monkeypatch.setattr(tiktok_live, "_load_tiktoklive_dependencies", dependencies)
    connector = TikTokLiveConnector(
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            reconnect_enabled=False,
            include_raw_event_debug=True,
            enabled_event_types=["connect", "comment", "gift"],
        )
    )

    await connector.start()
    try:
        raw_connect = await asyncio.wait_for(connector.events().__anext__(), timeout=1)
        raw_comment = await asyncio.wait_for(connector.events().__anext__(), timeout=1)
        raw_gift = await asyncio.wait_for(connector.events().__anext__(), timeout=1)
    finally:
        await connector.stop()

    assert raw_connect["type"] == "connect"
    assert raw_comment == {
        "type": "comment",
        "timestamp": "2026-01-02T03:04:05+00:00",
        "connector": "tiktok-live",
        "user": {
            "id": "user-123",
            "unique_id": "viewer_one",
            "nickname": "Viewer One",
            "subscriber": True,
        },
        "comment": "hello",
        "provider": {
            "comment": "hello",
            "user": {
                "id": "user-123",
                "unique_id": "viewer_one",
                "nickname": "Viewer One",
                "subscriber": True,
            },
            "timestamp": "2026-01-02T03:04:05+00:00",
        },
    }
    assert raw_gift["type"] == "gift"
    assert raw_gift["gift"] == {"id": "5655", "name": "Rose"}


@pytest.mark.asyncio
async def test_tiktok_connector_integrates_with_existing_normalizer(monkeypatch) -> None:
    from tiktok_event_gateway.connectors import tiktok_live

    monkeypatch.setattr(tiktok_live, "_load_tiktoklive_dependencies", dependencies)
    connector = TikTokLiveConnector(
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            reconnect_enabled=False,
            enabled_event_types=["comment", "gift"],
        )
    )
    events = []

    await connector.start()
    try:
        async for event in normalize_event_stream(connector.events(), connector.normalizer()):
            events.append(event)
            if len(events) == 2:
                break
    finally:
        await connector.stop()

    assert [type(event) for event in events] == [CommentEvent, GiftEvent]
    assert events[0].payload.comment == "hello"
    assert events[1].payload.gift_name == "Rose"


@pytest.mark.asyncio
async def test_tiktok_connector_skips_intermediate_gift_streak_updates(monkeypatch) -> None:
    from tiktok_event_gateway.connectors import tiktok_live

    class StreakTikTokClient(FakeTikTokClient):
        async def start(self) -> None:
            await self._emit(FakeGiftEvent, FakeGiftStreakPayload(repeat_count=1))
            await self._emit(FakeGiftEvent, FakeGiftStreakPayload(repeat_count=6))
            await self._emit(
                FakeGiftEvent,
                FakeGiftStreakPayload(
                    repeat_count=75,
                    repeat_end=True,
                    streaking=False,
                ),
            )

    def streak_dependencies():
        deps = dependencies()
        return tiktok_live._TikTokLiveDependencies(
            client_class=StreakTikTokClient,
            event_classes=deps.event_classes,
        )

    monkeypatch.setattr(tiktok_live, "_load_tiktoklive_dependencies", streak_dependencies)
    connector = TikTokLiveConnector(
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            reconnect_enabled=False,
            enabled_event_types=["gift"],
        )
    )

    await connector.start()
    try:
        raw_gift = await asyncio.wait_for(connector.events().__anext__(), timeout=1)
        with pytest.raises(asyncio.TimeoutError):
            await asyncio.wait_for(connector.events().__anext__(), timeout=0.05)
    finally:
        await connector.stop()

    assert raw_gift["type"] == "gift"
    assert raw_gift["repeat_count"] == 75
    assert raw_gift["repeat_end"] is True


@pytest.mark.asyncio
async def test_missing_tiktoklive_dependency_emits_error_event(monkeypatch) -> None:
    from tiktok_event_gateway.connectors import tiktok_live

    def missing_dependency():
        raise TikTokLiveDependencyError("missing")

    monkeypatch.setattr(tiktok_live, "_load_tiktoklive_dependencies", missing_dependency)
    connector = TikTokLiveConnector(
        TikTokLiveConnectorConfig.from_values(
            live_username="@viewer",
            reconnect_enabled=False,
        )
    )

    await connector.start()
    try:
        raw_event = await asyncio.wait_for(connector.events().__anext__(), timeout=1)
    finally:
        await connector.stop()

    assert raw_event["type"] == "error"
    assert raw_event["code"] == "tiktok_live_connector_error"
    assert "TikTokLive dependency is not installed" in raw_event["message"]


def test_tiktok_config_normalizes_username() -> None:
    config = TikTokLiveConnectorConfig.from_values(live_username="  @@viewer  ")

    assert config.live_username == "viewer"


@pytest.mark.asyncio
async def test_call_client_method_awaits_task_returned_by_async_method() -> None:
    from tiktok_event_gateway.connectors.tiktok_live import _call_client_method

    completed: list[str] = []

    async def background_work() -> None:
        await asyncio.sleep(0)
        completed.append("done")

    async def start() -> asyncio.Task[None]:
        return asyncio.create_task(background_work())

    await _call_client_method(start)

    assert completed == ["done"]


@pytest.mark.asyncio
async def test_start_client_prefers_connect_over_start() -> None:
    class ClientWithConnectAndStart:
        def __init__(self) -> None:
            self.used: str | None = None

        async def connect(self) -> None:
            self.used = "connect"

        async def start(self) -> None:
            msg = "start should not be selected when connect exists"
            raise AssertionError(msg)

    client = ClientWithConnectAndStart()
    connector = TikTokLiveConnector(
        TikTokLiveConnectorConfig.from_values(
            live_username="viewer",
            reconnect_enabled=False,
        )
    )

    await connector._start_client(client)

    assert client.used == "connect"


@pytest.mark.asyncio
async def test_is_live_false_skips_connect() -> None:
    class OfflineClient:
        def __init__(self, unique_id: str) -> None:
            self.unique_id = unique_id
            self.connect_called = False

        def add_listener(self, event_class: type[Any], handler: Any) -> None:
            return None

        async def is_live(self) -> bool:
            return False

        async def connect(self) -> None:
            self.connect_called = True
            msg = "connect should not be called when is_live() is False"
            raise AssertionError(msg)

    created: list[OfflineClient] = []

    def offline_dependencies():
        from tiktok_event_gateway.connectors import tiktok_live

        class FactoryClient(OfflineClient):
            def __init__(self, unique_id: str) -> None:
                super().__init__(unique_id)
                created.append(self)

        return tiktok_live._TikTokLiveDependencies(
            client_class=FactoryClient,
            event_classes={event_type: None for event_type in EventType},
        )

    from tiktok_event_gateway.connectors import tiktok_live

    monkeypatch = pytest.MonkeyPatch()
    monkeypatch.setattr(tiktok_live, "_load_tiktoklive_dependencies", offline_dependencies)
    try:
        connector = TikTokLiveConnector(
            TikTokLiveConnectorConfig.from_values(
                live_username="@@viewer",
                reconnect_enabled=False,
            )
        )
        await connector.start()
        await asyncio.sleep(0)
    finally:
        monkeypatch.undo()
        await connector.stop()

    assert created
    assert created[0].unique_id == "viewer"
    assert created[0].connect_called is False
