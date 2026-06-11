"""Tests for replaying normalized event logs."""

from __future__ import annotations

import asyncio
from datetime import datetime, timezone
from pathlib import Path

import pytest

from tiktok_event_gateway.broker import EventBroker
from tiktok_event_gateway.config import AppSettings
from tiktok_event_gateway.models import CommentEvent, CommentPayload, Source, SourcePlatform, User
from tiktok_event_gateway.runtime import read_event_log, replay_event_log


class FakeWebSocketServer:
    """Tiny stand-in for replay tests that avoids binding a real socket."""

    url = "ws://127.0.0.1:8765/events"

    def __init__(self) -> None:
        self.started = False
        self.stopped = False

    async def start(self) -> None:
        self.started = True

    async def stop(self) -> None:
        self.stopped = True


def event_json(event_id: str, second: int) -> str:
    event = CommentEvent(
        event_id=event_id,
        timestamp=datetime(2026, 1, 2, 3, 4, second, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.MOCK, connector="mock"),
        user=User(user_id="user-1", username="viewer"),
        payload=CommentPayload(comment=f"event {second}"),
    )
    return event.to_json()


@pytest.mark.asyncio
async def test_read_event_log_uses_gateway_event_models(tmp_path: Path) -> None:
    event_log = tmp_path / "events.jsonl"
    event_log.write_text(event_json("evt-1", 5) + "\n\n", encoding="utf-8")

    events = [event async for event in read_event_log(event_log)]

    assert len(events) == 1
    assert isinstance(events[0], CommentEvent)
    assert events[0].payload.comment == "event 5"


@pytest.mark.asyncio
async def test_replay_event_log_publishes_through_broker(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
) -> None:
    from tiktok_event_gateway import runtime

    event_log = tmp_path / "events.jsonl"
    event_log.write_text(
        event_json("evt-1", 5) + "\n" + event_json("evt-2", 6) + "\n",
        encoding="utf-8",
    )
    broker = EventBroker(subscriber_queue_size=10)
    subscription = broker.subscribe()
    server = FakeWebSocketServer()

    monkeypatch.setattr(runtime, "build_broker_and_server", lambda settings: (broker, server))

    await replay_event_log(
        event_log_path=event_log,
        settings=AppSettings(),
        speed_multiplier=10_000,
    )

    first = await asyncio.wait_for(subscription.queue.get(), timeout=1)
    second = await asyncio.wait_for(subscription.queue.get(), timeout=1)
    assert first.event_id == "evt-1"
    assert second.event_id == "evt-2"
    assert server.started is True
    assert server.stopped is True


@pytest.mark.asyncio
async def test_replay_rejects_invalid_speed(tmp_path: Path) -> None:
    event_log = tmp_path / "events.jsonl"
    event_log.write_text(event_json("evt-1", 5), encoding="utf-8")

    with pytest.raises(ValueError, match="speed_multiplier"):
        await replay_event_log(
            event_log_path=event_log,
            settings=AppSettings(),
            speed_multiplier=0,
        )


@pytest.mark.asyncio
async def test_read_event_log_reports_malformed_lines(tmp_path: Path) -> None:
    event_log = tmp_path / "events.jsonl"
    event_log.write_text('{"event_type": "comment"}\n', encoding="utf-8")

    with pytest.raises(ValueError, match="invalid event log line 1"):
        _ = [event async for event in read_event_log(event_log)]
