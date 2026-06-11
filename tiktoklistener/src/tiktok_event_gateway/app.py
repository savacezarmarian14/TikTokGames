"""Application composition root for TikTok Event Gateway.

This module owns wiring between validated settings and runtime components.
Runtime orchestration lives in ``runtime.py``; provider-specific behavior stays
behind connectors and normalizers.
"""

from __future__ import annotations

from tiktok_event_gateway.bridge import RuleBasedGameBridge
from tiktok_event_gateway.broker import EventBroker
from tiktok_event_gateway.config import AppSettings
from tiktok_event_gateway.connectors import (
    BaseConnector,
    MockConnector,
    MockConnectorConfig,
    TikTokLiveConnector,
    TikTokLiveConnectorConfig,
)
from tiktok_event_gateway.logging import EventJsonlWriter
from tiktok_event_gateway.normalization import EventNormalizer, TikTokEventNormalizer
from tiktok_event_gateway.transports import WebSocketEventServer


def build_connector(settings: AppSettings) -> tuple[BaseConnector, EventNormalizer]:
    """Create the configured connector and matching normalizer."""
    if settings.connector.provider == "mock":
        mock_settings = settings.connector.mock
        mock_config = MockConnectorConfig.from_values(
            interval_seconds=mock_settings.interval_seconds,
            enabled_event_types=mock_settings.enabled_event_types,
            username=mock_settings.username,
            display_name=mock_settings.display_name,
            user_id=mock_settings.user_id,
            room_id=mock_settings.room_id,
            stream_id=mock_settings.stream_id,
        )
        connector = MockConnector(
            mock_config,
            max_queue_size=settings.queues.raw_event_max_size,
        )
        normalizer = TikTokEventNormalizer(
            connector_name="mock",
            room_id=mock_config.room_id,
            stream_id=mock_config.stream_id,
        )
        return connector, normalizer

    tiktok_settings = settings.connector.tiktok
    tiktok_config = TikTokLiveConnectorConfig.from_values(
        live_username=tiktok_settings.live_username,
        reconnect_enabled=tiktok_settings.reconnect_enabled,
        reconnect_delay_seconds=tiktok_settings.reconnect_delay_seconds,
        include_raw_event_debug=tiktok_settings.include_raw_event_debug,
        enabled_event_types=tiktok_settings.enabled_event_types,
    )
    connector = TikTokLiveConnector(
        tiktok_config,
        max_queue_size=settings.queues.raw_event_max_size,
    )
    return connector, connector.normalizer()


def build_broker_and_server(settings: AppSettings) -> tuple[EventBroker, WebSocketEventServer]:
    """Create the event broker and WebSocket server from settings."""
    broker = EventBroker(subscriber_queue_size=settings.websocket.client_queue_size)
    server = WebSocketEventServer(
        broker=broker,
        host=settings.websocket.host,
        port=settings.websocket.port,
        path=settings.websocket.path,
        heartbeat_interval_seconds=settings.websocket.heartbeat_interval_seconds,
        send_timeout_seconds=settings.websocket.send_timeout_seconds,
    )
    return broker, server


def build_event_writer(settings: AppSettings) -> EventJsonlWriter | None:
    """Create an optional normalized event JSONL writer."""
    event_log = settings.logging.event_log
    if not event_log.enabled:
        return None
    return EventJsonlWriter(event_log.path, include_raw_event=event_log.include_raw_event)


def build_game_bridge(settings: AppSettings) -> RuleBasedGameBridge | None:
    """Create the optional rule-based game bridge from settings."""
    return RuleBasedGameBridge.from_settings(settings)
