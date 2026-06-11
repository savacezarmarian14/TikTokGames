"""Tests for connector lifecycle and mock event production."""

from __future__ import annotations

import asyncio
import json

import pytest

from tiktok_event_gateway.connectors import (
    BaseConnector,
    ConnectorRuntimeError,
    MockConnector,
    MockConnectorConfig,
)
from tiktok_event_gateway.models import CommentEvent, EventType, GiftEvent, LikeEvent
from tiktok_event_gateway.normalization import TikTokEventNormalizer, normalize_event_stream


class FailingConnector(BaseConnector):
    """Connector used to verify unexpected producer failures."""

    def __init__(self) -> None:
        super().__init__(name="failing-test")

    async def _run(self) -> None:
        raise RuntimeError("boom")


async def next_event(connector: MockConnector) -> dict[str, object]:
    """Read one raw event from a connector with a short timeout."""
    iterator = connector.events()
    return await asyncio.wait_for(iterator.__anext__(), timeout=1)


def test_mock_config_rejects_invalid_values() -> None:
    with pytest.raises(ValueError, match="interval_seconds"):
        MockConnectorConfig.from_values(interval_seconds=0)

    with pytest.raises(ValueError, match="at least one"):
        MockConnectorConfig.from_values(enabled_event_types=[])

    with pytest.raises(ValueError, match="unknown event type"):
        MockConnectorConfig.from_values(enabled_event_types=["share"])

    with pytest.raises(ValueError, match="does not emit"):
        MockConnectorConfig.from_values(enabled_event_types=["heartbeat"])


@pytest.mark.asyncio
async def test_mock_connector_emits_raw_events_until_stopped() -> None:
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=["comment"],
            username="tester",
            room_id="room-1",
        )
    )

    await connector.start()
    try:
        raw_event = await next_event(connector)
    finally:
        await connector.stop()

    assert connector.is_running() is False
    assert raw_event["type"] == "comment"
    assert raw_event["room_id"] == "room-1"
    assert raw_event["user"] == {
        "id": "mock-user-001-001",
        "unique_id": "tester",
        "nickname": "Mock Viewer",
    }
    assert raw_event["comment"] == "Mock comment #1"


@pytest.mark.asyncio
async def test_mock_connector_can_be_consumed_as_async_iterator() -> None:
    connector = MockConnector(
        MockConnectorConfig.from_values(interval_seconds=0.01, enabled_event_types=["like"])
    )

    await connector.start()
    try:
        raw_event = await asyncio.wait_for(connector.__aiter__().__anext__(), timeout=1)
    finally:
        await connector.stop()

    assert raw_event["type"] == "like"


@pytest.mark.asyncio
async def test_mock_connector_integrates_with_tiktok_normalizer() -> None:
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=["comment", "gift", "like"],
            username="normalizer_user",
            room_id="room-normalized",
        )
    )
    normalizer = TikTokEventNormalizer(connector_name="mock", room_id="room-normalized")
    events = []

    await connector.start()
    try:
        async for event in normalize_event_stream(connector.events(), normalizer):
            events.append(event)
            if len(events) == 3:
                break
    finally:
        await connector.stop()

    assert [type(event) for event in events] == [CommentEvent, GiftEvent, LikeEvent]
    assert all(event.source.connector == "mock" for event in events)
    assert events[0].user is not None
    assert events[0].user.username == "normalizer_user"
    assert json.loads(events[0].to_json())["event_type"] == "comment"


@pytest.mark.asyncio
async def test_mock_connector_respects_enabled_event_types() -> None:
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=[EventType.GIFT],
        )
    )

    await connector.start()
    try:
        first = await next_event(connector)
        second = await next_event(connector)
    finally:
        await connector.stop()

    assert first["type"] == "gift"
    assert second["type"] == "gift"


@pytest.mark.asyncio
async def test_stop_wakes_event_stream_consumers() -> None:
    connector = MockConnector(
        MockConnectorConfig.from_values(interval_seconds=10, enabled_event_types=["comment"])
    )
    iterator = connector.events()

    await connector.start()
    await asyncio.wait_for(iterator.__anext__(), timeout=1)
    await connector.stop()

    with pytest.raises(StopAsyncIteration):
        await asyncio.wait_for(iterator.__anext__(), timeout=1)


@pytest.mark.asyncio
async def test_connector_task_failure_reaches_event_consumer() -> None:
    connector = FailingConnector()
    iterator = connector.events()

    await connector.start()

    with pytest.raises(ConnectorRuntimeError, match="failing-test"):
        await asyncio.wait_for(iterator.__anext__(), timeout=1)
