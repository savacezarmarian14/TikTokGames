"""Tests for WebSocket subscription parsing."""

from __future__ import annotations

import pytest

from tiktok_event_gateway.models import EventType
from tiktok_event_gateway.transports import (
    event_type_values,
    event_types_from_message,
    event_types_from_path,
    parse_event_types,
)


def test_event_types_from_path_defaults_to_all_events() -> None:
    assert event_types_from_path("/events") is None


def test_event_types_from_path_parses_query_filter() -> None:
    assert event_types_from_path("/events?event_types=gift,comment") == frozenset(
        {EventType.GIFT, EventType.COMMENT}
    )


def test_event_types_from_message_parses_list_filter() -> None:
    assert event_types_from_message('{"event_types": ["gift", "join"]}') == frozenset(
        {EventType.GIFT, EventType.JOIN}
    )


def test_event_types_from_message_supports_all_events() -> None:
    assert event_types_from_message('{"all": true}') is None


def test_parse_event_types_rejects_unknown_type() -> None:
    with pytest.raises(ValueError):
        parse_event_types("gift,share")


def test_event_type_values_are_sorted_for_logs() -> None:
    values = event_type_values(frozenset({EventType.GIFT, EventType.COMMENT}))

    assert values == ["comment", "gift"]
