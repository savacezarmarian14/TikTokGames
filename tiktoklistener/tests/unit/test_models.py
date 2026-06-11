"""Tests for normalized Pydantic event schemas."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

import pytest
from pydantic import ValidationError

from tiktok_event_gateway.models import (
    CommentEvent,
    CommentPayload,
    ConnectEvent,
    DisconnectEvent,
    ErrorEvent,
    ErrorPayload,
    EventType,
    FollowEvent,
    GiftEvent,
    GiftPayload,
    HeartbeatEvent,
    HeartbeatPayload,
    JoinEvent,
    LikeEvent,
    LikePayload,
    Source,
    SourcePlatform,
    User,
    parse_gateway_event,
)


def source() -> Source:
    """Create a reusable test source."""
    return Source(
        platform=SourcePlatform.TIKTOK,
        connector="tiktok-live",
        room_id="room-123",
    )


def user() -> User:
    """Create a reusable test user."""
    return User(user_id="user-123", username="viewer", display_name="Viewer")


@pytest.mark.parametrize(
    ("event", "expected_type"),
    [
        (
            CommentEvent(source=source(), user=user(), payload=CommentPayload(comment="hello")),
            EventType.COMMENT,
        ),
        (JoinEvent(source=source(), user=user()), EventType.JOIN),
        (
            GiftEvent(
                source=source(),
                user=user(),
                payload=GiftPayload(gift_id="5655", gift_name="Rose", repeat_count=2),
            ),
            EventType.GIFT,
        ),
        (
            LikeEvent(source=source(), user=user(), payload=LikePayload(like_count=3)),
            EventType.LIKE,
        ),
        (FollowEvent(source=source(), user=user()), EventType.FOLLOW),
        (ConnectEvent(source=source()), EventType.CONNECT),
        (DisconnectEvent(source=source()), EventType.DISCONNECT),
        (
            ErrorEvent(source=source(), payload=ErrorPayload(message="connection lost")),
            EventType.ERROR,
        ),
        (
            HeartbeatEvent(source=source(), payload=HeartbeatPayload(sequence=1)),
            EventType.HEARTBEAT,
        ),
    ],
)
def test_event_creation_sets_stable_envelope(event, expected_type: EventType) -> None:
    assert event.version == "1.0"
    assert event.event_id
    assert event.event_type == expected_type
    assert event.timestamp.tzinfo is not None
    assert event.source.platform == SourcePlatform.TIKTOK


def test_json_serialization_uses_json_friendly_values() -> None:
    timestamp = datetime(2026, 1, 2, 3, 4, 5, tzinfo=timezone.utc)
    event = CommentEvent(
        event_id="evt-001",
        timestamp=timestamp,
        source=source(),
        user=user(),
        payload=CommentPayload(comment="hello"),
    )

    data = json.loads(event.to_json())

    assert data == {
        "version": "1.0",
        "event_id": "evt-001",
        "event_type": "comment",
        "timestamp": "2026-01-02T03:04:05Z",
        "source": {
            "platform": "tiktok",
            "connector": "tiktok-live",
            "room_id": "room-123",
        },
        "user": {
            "user_id": "user-123",
            "username": "viewer",
            "display_name": "Viewer",
        },
        "payload": {"comment": "hello"},
    }


def test_raw_event_is_excluded_by_default_and_available_for_debug() -> None:
    event = CommentEvent(
        source=source(),
        user=user(),
        payload=CommentPayload(comment="hello"),
        raw_event={"provider": {"event_name": "CommentEvent"}},
    )

    assert "raw_event" not in event.to_dict()
    assert "raw_event" not in json.loads(event.to_json())
    assert event.to_dict(include_raw=True)["raw_event"] == {
        "provider": {"event_name": "CommentEvent"}
    }


def test_payload_extension_fields_must_be_json_compatible() -> None:
    payload = CommentPayload(
        comment="hello",
        language="en",
        tags=["chat", "safe"],
    )

    assert payload.model_dump(mode="json") == {
        "comment": "hello",
        "language": "en",
        "tags": ["chat", "safe"],
    }

    with pytest.raises(ValidationError):
        CommentPayload(comment="hello", provider_object=object())


def test_rejects_non_json_raw_event_values() -> None:
    with pytest.raises(ValidationError):
        CommentEvent(
            source=source(),
            user=user(),
            payload=CommentPayload(comment="hello"),
            raw_event={"bad": object()},
        )


def test_user_required_for_user_originated_events() -> None:
    with pytest.raises(ValidationError):
        CommentEvent(
            source=source(),
            payload=CommentPayload(comment="hello"),
        )  # type: ignore[call-arg]


def test_validation_rejects_invalid_counts_and_blank_comment() -> None:
    with pytest.raises(ValidationError):
        CommentPayload(comment="")

    with pytest.raises(ValidationError):
        GiftPayload(gift_id="1", gift_name="Rose", repeat_count=0)

    with pytest.raises(ValidationError):
        LikePayload(like_count=0)


def test_parse_gateway_event_returns_concrete_model() -> None:
    raw_json = CommentEvent(
        source=source(),
        user=user(),
        payload=CommentPayload(comment="hello"),
    ).to_json()

    event = parse_gateway_event(raw_json)

    assert isinstance(event, CommentEvent)
    assert event.payload.comment == "hello"


def test_example_json_files_match_schema() -> None:
    examples_dir = Path(__file__).parents[2] / "examples" / "events"

    for example_file in examples_dir.glob("*.json"):
        event = parse_gateway_event(example_file.read_text(encoding="utf-8"))
        assert event.to_dict() == json.loads(example_file.read_text(encoding="utf-8"))
