"""Tests for converting raw connector envelopes into gateway events."""

from __future__ import annotations

import json
from datetime import datetime, timezone

from tiktok_event_gateway.models import (
    CommentEvent,
    ConnectEvent,
    DisconnectEvent,
    ErrorEvent,
    EventType,
    FollowEvent,
    GiftEvent,
    JoinEvent,
    LikeEvent,
)
from tiktok_event_gateway.normalization import TikTokEventNormalizer


class FakeUser:
    """Small object that mimics the useful shape of a provider user."""

    def __init__(self) -> None:
        self.id = "user-123"
        self.unique_id = "viewer_one"
        self.nickname = "Viewer One"
        self.moderator = False
        self.subscriber = True


class FakeCommentEvent:
    """Fake provider object without importing TikTokLive."""

    def __init__(self) -> None:
        self.comment = "hello from an object"
        self.user = FakeUser()
        self.timestamp = datetime(2026, 1, 2, 3, 4, 5)
        self.room_id = "room-123"


def test_normalizes_comment_dict() -> None:
    normalizer = TikTokEventNormalizer(room_id="room-default")

    event = normalizer.normalize(
        {
            "event_type": "comment",
            "timestamp": "2026-01-02T03:04:05+00:00",
            "user": {
                "id": "user-123",
                "unique_id": "viewer_one",
                "nickname": "Viewer One",
            },
            "comment": "hello stream",
            "unknown_provider_field": {"ignored": True},
        }
    )

    assert isinstance(event, CommentEvent)
    assert event.event_id
    assert event.event_type == EventType.COMMENT
    assert event.source.room_id == "room-default"
    assert event.user.user_id == "user-123"
    assert event.payload.comment == "hello stream"
    assert "raw_event" not in event.to_dict()


def test_normalizes_comment_object_and_timestamp_to_utc_json() -> None:
    normalizer = TikTokEventNormalizer()

    event = normalizer.normalize(FakeCommentEvent())
    data = json.loads(event.to_json())

    assert isinstance(event, CommentEvent)
    assert data["timestamp"] == "2026-01-02T03:04:05Z"
    assert data["user"]["username"] == "viewer_one"
    assert data["payload"]["comment"] == "hello from an object"


def test_debug_raw_event_is_included_only_when_enabled() -> None:
    raw_event = {
        "type": "like",
        "timestamp_ms": 1767225600000,
        "user": {"id": "user-123"},
        "like_count": 7,
        "provider_only": object(),
    }
    normalizer = TikTokEventNormalizer(include_raw_event=True)

    event = normalizer.normalize(raw_event)

    assert isinstance(event, LikeEvent)
    assert event.raw_event is not None
    assert event.raw_event["provider_only"].startswith("<object object")
    assert "raw_event" not in event.to_dict()
    assert "raw_event" in event.to_dict(include_raw=True)


def test_normalizes_supported_event_types_from_dicts() -> None:
    normalizer = TikTokEventNormalizer(connector_name="unit-test", room_id="room-123")

    events = [
        normalizer.normalize({"type": "join", "user": {"id": "u1"}, "viewer_count": 10}),
        normalizer.normalize(
            {
                "type": "gift",
                "user": {"id": "u2"},
                "gift": {"id": "5655", "name": "Rose"},
                "repeat_count": 2,
                "diamond_count": 1,
                "repeat_end": True,
            }
        ),
        normalizer.normalize({"type": "like", "user": {"id": "u3"}, "count": 3}),
        normalizer.normalize({"type": "follow", "user": {"id": "u4"}}),
        normalizer.normalize({"type": "connect", "connection_id": "conn-1"}),
        normalizer.normalize({"type": "disconnect", "reason": "shutdown", "code": "normal"}),
        normalizer.normalize({"type": "error", "message": "provider failed", "code": "provider"}),
    ]

    assert [type(event) for event in events] == [
        JoinEvent,
        GiftEvent,
        LikeEvent,
        FollowEvent,
        ConnectEvent,
        DisconnectEvent,
        ErrorEvent,
    ]
    assert all(event.source.connector == "unit-test" for event in events)


def test_invalid_event_returns_error_event_instead_of_raising() -> None:
    normalizer = TikTokEventNormalizer(include_raw_event=True)

    event = normalizer.normalize({"type": "comment", "comment": "missing user"})

    assert isinstance(event, ErrorEvent)
    assert event.payload.code == "normalization_error"
    assert event.payload.recoverable is True
    assert "missing required user identifier" in event.payload.message
    assert event.raw_event == {"type": "comment", "comment": "missing user"}


def test_unknown_event_type_returns_error_event() -> None:
    normalizer = TikTokEventNormalizer()

    event = normalizer.normalize({"type": "share", "user": {"id": "u1"}})

    assert isinstance(event, ErrorEvent)
    assert "unknown TikTok event type" in event.payload.message


def test_source_fields_can_come_from_raw_event() -> None:
    normalizer = TikTokEventNormalizer(room_id="default-room")

    event = normalizer.normalize(
        {
            "type": "connect",
            "connector": "mock-tiktok",
            "room_id": "event-room",
            "stream_id": "stream-1",
        }
    )

    assert isinstance(event, ConnectEvent)
    assert event.source.connector == "mock-tiktok"
    assert event.source.room_id == "event-room"
    assert event.source.stream_id == "stream-1"


def test_numeric_timestamp_seconds_are_normalized_to_utc() -> None:
    normalizer = TikTokEventNormalizer()

    event = normalizer.normalize(
        {
            "type": "disconnect",
            "timestamp": datetime(2026, 1, 2, 3, 4, 5, tzinfo=timezone.utc).timestamp(),
        }
    )

    assert isinstance(event, DisconnectEvent)
    assert json.loads(event.to_json())["timestamp"] == "2026-01-02T03:04:05Z"
