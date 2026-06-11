"""Async runtime orchestration for the gateway pipeline.

Runtime code will own task startup, cancellation, queue wiring, signal handling,
and orderly shutdown across connectors, normalizers, brokers, and transports.
"""

from __future__ import annotations

import asyncio
from collections.abc import AsyncIterator
from contextlib import suppress
from pathlib import Path

import structlog

from tiktok_event_gateway.app import build_broker_and_server, build_connector, build_event_writer, build_game_bridge
from tiktok_event_gateway.config import AppSettings
from tiktok_event_gateway.models import GatewayEvent, parse_gateway_event
from tiktok_event_gateway.normalization import normalize_event_stream

logger = structlog.get_logger(__name__)


async def run_gateway(settings: AppSettings) -> None:
    """Run connector, normalizer, broker, and WebSocket server."""
    connector, normalizer = build_connector(settings)
    broker, server = build_broker_and_server(settings)
    event_writer = build_event_writer(settings)
    game_bridge = build_game_bridge(settings)

    await server.start()
    if game_bridge is not None:
        await game_bridge.start()
    await connector.start()
    logger.info("gateway started", websocket_url=server.url, connector=connector.name)

    try:
        async for event in normalize_event_stream(connector.events(), normalizer):
            await broker.publish(event)
            if game_bridge is not None:
                await game_bridge.process_event(event)
            if event_writer is not None:
                await event_writer.write(event)
    finally:
        await connector.stop()
        if game_bridge is not None:
            await game_bridge.stop()
        await server.stop()
        logger.info("gateway stopped")


async def replay_event_log(
    *,
    event_log_path: str | Path,
    settings: AppSettings,
    speed_multiplier: float | None = None,
    start_delay_seconds: float = 0.0,
) -> None:
    """Replay normalized JSONL events through the WebSocket broker."""
    if speed_multiplier is not None and speed_multiplier <= 0:
        msg = "speed_multiplier must be greater than zero"
        raise ValueError(msg)
    if start_delay_seconds < 0:
        msg = "start_delay_seconds must be zero or greater"
        raise ValueError(msg)

    effective_speed = speed_multiplier or settings.replay.speed_multiplier
    broker, server = build_broker_and_server(settings)
    await server.start()
    logger.info(
        "replay started",
        websocket_url=server.url,
        event_log_path=str(event_log_path),
        speed_multiplier=effective_speed,
    )

    try:
        if start_delay_seconds > 0:
            await asyncio.sleep(start_delay_seconds)

        previous_event: GatewayEvent | None = None
        async for event in read_event_log(event_log_path):
            if previous_event is not None:
                delay = (event.timestamp - previous_event.timestamp).total_seconds()
                if delay > 0:
                    await asyncio.sleep(delay / effective_speed)
            await broker.publish(event)
            previous_event = event
        logger.info("replay completed", event_log_path=str(event_log_path))
    finally:
        await server.stop()


async def read_event_log(path: str | Path) -> AsyncIterator[GatewayEvent]:
    """Yield parsed gateway events from a JSONL event log."""
    event_log_path = Path(path)
    if not event_log_path.exists():
        msg = f"event log does not exist: {event_log_path}"
        raise FileNotFoundError(msg)

    file = await asyncio.to_thread(event_log_path.open, "r", encoding="utf-8")
    try:
        line_number = 0
        while True:
            line = await asyncio.to_thread(file.readline)
            if not line:
                break
            line_number += 1
            text = line.strip()
            if not text:
                continue
            try:
                yield parse_gateway_event(text)
            except Exception as exc:
                msg = f"invalid event log line {line_number} in {event_log_path}: {exc}"
                raise ValueError(msg) from exc
    finally:
        await asyncio.to_thread(file.close)


async def cancel_and_wait(task: asyncio.Task[object]) -> None:
    """Cancel a task and suppress its cancellation."""
    task.cancel()
    with suppress(asyncio.CancelledError):
        await task
