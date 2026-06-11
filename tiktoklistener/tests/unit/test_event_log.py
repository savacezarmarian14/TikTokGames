"""Tests for normalized event JSONL logging."""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

import pytest

from tiktok_event_gateway.logging import EventJsonlWriter
from tiktok_event_gateway.models import CommentEvent, CommentPayload, Source, SourcePlatform, User


@pytest.mark.asyncio
async def test_event_jsonl_writer_appends_normalized_events(tmp_path: Path) -> None:
    log_path = tmp_path / "events.jsonl"
    writer = EventJsonlWriter(log_path)
    event = CommentEvent(
        event_id="evt-log-1",
        timestamp=datetime(2026, 1, 2, 3, 4, 5, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.MOCK, connector="mock"),
        user=User(user_id="user-1", username="viewer"),
        payload=CommentPayload(comment="hello"),
        raw_event={"debug": True},
    )

    await writer.write(event)

    line = log_path.read_text(encoding="utf-8").strip()
    data = json.loads(line)
    assert data["event_id"] == "evt-log-1"
    assert data["event_type"] == "comment"
    assert "raw_event" not in data


@pytest.mark.asyncio
async def test_event_jsonl_writer_can_include_raw_debug_data(tmp_path: Path) -> None:
    log_path = tmp_path / "events.jsonl"
    writer = EventJsonlWriter(log_path, include_raw_event=True)
    event = CommentEvent(
        source=Source(platform=SourcePlatform.MOCK, connector="mock"),
        user=User(user_id="user-1"),
        payload=CommentPayload(comment="hello"),
        raw_event={"debug": True},
    )

    await writer.write(event)

    data = json.loads(log_path.read_text(encoding="utf-8"))
    assert data["raw_event"] == {"debug": True}
