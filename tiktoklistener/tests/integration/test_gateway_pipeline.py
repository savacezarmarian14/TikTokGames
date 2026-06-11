"""End-to-end tests for mock connector to WebSocket client delivery."""

from __future__ import annotations

import asyncio
import json
from contextlib import suppress
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import pytest
from typer.testing import CliRunner
from websockets.client import connect

from tiktok_event_gateway import runtime
from tiktok_event_gateway.broker import EventBroker
from tiktok_event_gateway.cli import app
from tiktok_event_gateway.config import AppSettings
from tiktok_event_gateway.connectors import MockConnector, MockConnectorConfig
from tiktok_event_gateway.models import (
    CommentEvent,
    CommentPayload,
    EventType,
    Source,
    SourcePlatform,
    User,
)
from tiktok_event_gateway.normalization import TikTokEventNormalizer, normalize_event_stream
from tiktok_event_gateway.runtime import replay_event_log
from tiktok_event_gateway.transports import WebSocketEventServer


async def pump_mock_events(connector: MockConnector, broker: EventBroker) -> None:
    """Move mock connector events through the normalizer into the broker."""
    normalizer = TikTokEventNormalizer(connector_name="mock", room_id=connector.config.room_id)
    async for event in normalize_event_stream(connector.events(), normalizer):
        await broker.publish(event)


async def receive_event(websocket: Any) -> dict[str, Any]:
    """Receive and decode one event from a WebSocket client."""
    return json.loads(await asyncio.wait_for(websocket.recv(), timeout=2))


def event_json(event_id: str, second: int) -> str:
    """Create a replayable comment event JSON string."""
    event = CommentEvent(
        event_id=event_id,
        timestamp=datetime(2026, 1, 2, 3, 4, second, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.MOCK, connector="mock"),
        user=User(user_id="user-1", username="viewer"),
        payload=CommentPayload(comment=f"event {second}"),
    )
    return event.to_json()


@pytest.mark.asyncio
async def test_mock_connector_events_reach_multiple_websocket_clients() -> None:
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=5,
    )
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=["comment", "gift"],
            username="integration_user",
            room_id="integration-room",
        )
    )
    pump_task: asyncio.Task[None] | None = None

    await server.start()
    try:
        async with connect(server.url) as client_a, connect(server.url) as client_b:
            await connector.start()
            pump_task = asyncio.create_task(pump_mock_events(connector, broker))

            event_a = await receive_event(client_a)
            event_b = await receive_event(client_b)

            assert event_a["event_type"] == "comment"
            assert event_b["event_type"] == "comment"
            assert event_a["payload"] == {"comment": "Mock comment #1"}
            assert event_b["user"]["username"] == "integration_user"
    finally:
        await connector.stop()
        if pump_task is not None:
            pump_task.cancel()
            with suppress(asyncio.CancelledError):
                await pump_task
        await server.stop()


@pytest.mark.asyncio
async def test_websocket_query_subscription_filters_event_types() -> None:
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=5,
    )
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=["comment", "gift"],
        )
    )
    pump_task: asyncio.Task[None] | None = None

    await server.start()
    try:
        async with connect(f"{server.url}?event_types=gift") as websocket:
            await connector.start()
            pump_task = asyncio.create_task(pump_mock_events(connector, broker))
            event = await receive_event(websocket)

            assert event["event_type"] == "gift"
            assert event["payload"]["gift_id"] == "mock-gift"
    finally:
        await connector.stop()
        if pump_task is not None:
            pump_task.cancel()
            with suppress(asyncio.CancelledError):
                await pump_task
        await server.stop()


@pytest.mark.asyncio
async def test_websocket_runtime_subscription_update_filters_event_types() -> None:
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=5,
    )
    connector = MockConnector(
        MockConnectorConfig.from_values(
            interval_seconds=0.01,
            enabled_event_types=["comment", "gift"],
        )
    )
    pump_task: asyncio.Task[None] | None = None

    await server.start()
    try:
        async with connect(server.url) as websocket:
            await websocket.send(json.dumps({"event_types": ["gift"]}))
            await asyncio.sleep(0.02)
            await connector.start()
            pump_task = asyncio.create_task(pump_mock_events(connector, broker))
            event = await receive_event(websocket)

            assert event["event_type"] == "gift"
    finally:
        await connector.stop()
        if pump_task is not None:
            pump_task.cancel()
            with suppress(asyncio.CancelledError):
                await pump_task
        await server.stop()


@pytest.mark.asyncio
async def test_websocket_server_sends_heartbeat_when_idle() -> None:
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=0.01,
    )

    await server.start()
    try:
        async with connect(server.url) as websocket:
            event = await receive_event(websocket)

            assert event["event_type"] == "heartbeat"
            assert event["source"]["platform"] == "gateway"
    finally:
        await server.stop()


@pytest.mark.asyncio
async def test_broker_drops_old_events_for_slow_subscriber() -> None:
    broker = EventBroker(subscriber_queue_size=1)
    subscription = broker.subscribe()
    normalizer = TikTokEventNormalizer(connector_name="mock")
    connector = MockConnector(
        MockConnectorConfig.from_values(interval_seconds=1, enabled_event_types=["comment"])
    )

    first = normalizer.normalize(connector._build_event(EventType.COMMENT, 1))
    second = normalizer.normalize(connector._build_event(EventType.COMMENT, 2))

    await broker.publish(first)
    await broker.publish(second)

    delivered = await asyncio.wait_for(subscription.queue.get(), timeout=1)
    assert delivered.event_id == second.event_id
    assert subscription.dropped_events == 1


@pytest.mark.asyncio
async def test_malformed_raw_event_reaches_websocket_as_error_event() -> None:
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=5,
    )
    normalizer = TikTokEventNormalizer(connector_name="mock")

    await server.start()
    try:
        async with connect(server.url) as websocket:
            await asyncio.sleep(0.01)
            event = normalizer.normalize({"type": "comment", "comment": "missing user"})
            await broker.publish(event)
            received = await receive_event(websocket)

            assert received["event_type"] == "error"
            assert received["payload"]["code"] == "normalization_error"
            assert "missing required user identifier" in received["payload"]["message"]
    finally:
        await server.stop()


@pytest.mark.asyncio
async def test_replay_mode_reaches_websocket_client(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    event_log = tmp_path / "events.jsonl"
    event_log.write_text(event_json("evt-replay-1", 5) + "\n", encoding="utf-8")
    broker = EventBroker(subscriber_queue_size=10)
    server = WebSocketEventServer(
        broker=broker,
        host="127.0.0.1",
        port=0,
        heartbeat_interval_seconds=5,
    )

    monkeypatch.setattr(runtime, "build_broker_and_server", lambda settings: (broker, server))
    replay_task = asyncio.create_task(
        replay_event_log(
            event_log_path=event_log,
            settings=AppSettings(),
            speed_multiplier=100,
            start_delay_seconds=0.2,
        )
    )
    try:
        while server._server is None:
            await asyncio.sleep(0.01)
        async with connect(server.url) as websocket:
            received = await receive_event(websocket)

            assert received["event_id"] == "evt-replay-1"
            assert received["event_type"] == "comment"
    finally:
        replay_task.cancel()
        with suppress(asyncio.CancelledError):
            await replay_task
        await server.stop()


def test_validate_config_command_rejects_invalid_config(tmp_path: Path) -> None:
    config_path = tmp_path / "bad.yaml"
    config_path.write_text(
        """
connector:
  provider: "tiktok"
  tiktok:
    live_username: ""
websocket:
  path: "events"
""",
        encoding="utf-8",
    )

    result = CliRunner(mix_stderr=False).invoke(app, ["validate-config", "--config", str(config_path)])

    assert result.exit_code == 2
    assert "invalid config" in result.stderr
    assert "connector.tiktok.live_username" in result.stderr
