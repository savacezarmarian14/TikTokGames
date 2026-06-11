"""Typed runtime settings.

Pydantic settings models will describe gateway, connector, queue, WebSocket,
and logging configuration.
"""

from __future__ import annotations

from pathlib import Path
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field, field_validator, model_validator

from tiktok_event_gateway.models import EventType
from tiktok_event_gateway.utils.tiktok import normalize_tiktok_username


class GatewaySettings(BaseModel):
    """General service metadata."""

    model_config = ConfigDict(extra="forbid")

    name: str = "tiktok-event-gateway"
    environment: str = "local"


class MockConnectorSettings(BaseModel):
    """Settings for the mock connector."""

    model_config = ConfigDict(extra="forbid")

    interval_seconds: float = Field(default=1.0, gt=0)
    enabled_event_types: list[EventType] = Field(
        default_factory=lambda: [
            EventType.COMMENT,
            EventType.JOIN,
            EventType.GIFT,
            EventType.LIKE,
            EventType.FOLLOW,
        ]
    )
    username: str = "mock_viewer"
    display_name: str = "Mock Viewer"
    user_id: str = "mock-user-001"
    room_id: str = "mock-room"
    stream_id: str = "mock-stream"


class TikTokConnectorSettings(BaseModel):
    """Settings for the optional TikTokLive connector."""

    model_config = ConfigDict(extra="forbid")

    live_username: str = ""
    reconnect_enabled: bool = True
    reconnect_delay_seconds: float = Field(default=5.0, ge=0)
    include_raw_event_debug: bool = False
    enabled_event_types: list[EventType] = Field(
        default_factory=lambda: [
            EventType.COMMENT,
            EventType.JOIN,
            EventType.GIFT,
            EventType.LIKE,
            EventType.FOLLOW,
            EventType.CONNECT,
            EventType.DISCONNECT,
            EventType.ERROR,
        ]
    )

    @field_validator("live_username")
    @classmethod
    def normalize_live_username(cls, value: str) -> str:
        """Store TikTok usernames internally without leading @ characters."""
        return normalize_tiktok_username(value)


class ConnectorSettings(BaseModel):
    """Connector selection and provider-specific settings."""

    model_config = ConfigDict(extra="forbid")

    provider: Literal["mock", "tiktok"] = "mock"
    mock: MockConnectorSettings = Field(default_factory=MockConnectorSettings)
    tiktok: TikTokConnectorSettings = Field(default_factory=TikTokConnectorSettings)

    @model_validator(mode="after")
    def validate_selected_provider(self) -> "ConnectorSettings":
        """Fail early if the selected provider is not fully configured."""
        if self.provider == "tiktok" and not self.tiktok.live_username.strip():
            msg = "connector.tiktok.live_username is required when provider is 'tiktok'"
            raise ValueError(msg)
        return self


class WebSocketSettings(BaseModel):
    """WebSocket server settings."""

    model_config = ConfigDict(extra="forbid")

    host: str = "127.0.0.1"
    port: int = Field(default=8765, ge=0, le=65535)
    path: str = "/events"
    heartbeat_interval_seconds: float = Field(default=15.0, gt=0)
    send_timeout_seconds: float = Field(default=5.0, gt=0)
    client_queue_size: int = Field(default=100, gt=0)

    @field_validator("path")
    @classmethod
    def path_must_start_with_slash(cls, value: str) -> str:
        """Validate URL path shape."""
        if not value.startswith("/"):
            msg = "websocket.path must start with '/'"
            raise ValueError(msg)
        return value


class QueueSettings(BaseModel):
    """Internal queue sizes."""

    model_config = ConfigDict(extra="forbid")

    raw_event_max_size: int = Field(default=1000, gt=0)
    normalized_event_max_size: int = Field(default=1000, gt=0)


class EventLogSettings(BaseModel):
    """Optional normalized event JSONL logging."""

    model_config = ConfigDict(extra="forbid")

    enabled: bool = False
    path: Path = Path("logs/events.jsonl")
    include_raw_event: bool = False


class LoggingSettings(BaseModel):
    """Structured logging settings."""

    model_config = ConfigDict(extra="forbid")

    level: Literal["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"] = "INFO"
    json: bool = True
    event_log: EventLogSettings = Field(default_factory=EventLogSettings)


class ReplaySettings(BaseModel):
    """Replay defaults."""

    model_config = ConfigDict(extra="forbid")

    speed_multiplier: float = Field(default=1.0, gt=0)




class GameBridgeOutputSettings(BaseModel):
    """Output transport settings for generated game messages."""

    model_config = ConfigDict(extra="forbid")

    type: Literal["stdout", "udp"] = "stdout"
    host: str = "127.0.0.1"
    port: int = Field(default=7000, ge=0, le=65535)
    url: str | None = None


class GameBridgeSettings(BaseModel):
    """Optional rule-based bridge from normalized events to game messages."""

    model_config = ConfigDict(extra="forbid")

    enabled: bool = False
    rules_path: Path | None = None
    templates_path: Path | None = None
    log_unmatched_events: bool = False
    deduplication_enabled: bool = True
    deduplication_max_size: int = Field(default=10000, gt=0)
    output: GameBridgeOutputSettings = Field(default_factory=GameBridgeOutputSettings)

    @model_validator(mode="after")
    def validate_enabled_bridge(self) -> "GameBridgeSettings":
        """Require rule/template paths only when the bridge is enabled."""
        if self.enabled:
            if self.rules_path is None:
                msg = "game_bridge.rules_path is required when game_bridge.enabled=true"
                raise ValueError(msg)
            if self.templates_path is None:
                msg = "game_bridge.templates_path is required when game_bridge.enabled=true"
                raise ValueError(msg)
        return self

class AppSettings(BaseModel):
    """Top-level validated gateway settings."""

    model_config = ConfigDict(extra="forbid")

    gateway: GatewaySettings = Field(default_factory=GatewaySettings)
    connector: ConnectorSettings = Field(default_factory=ConnectorSettings)
    websocket: WebSocketSettings = Field(default_factory=WebSocketSettings)
    queues: QueueSettings = Field(default_factory=QueueSettings)
    logging: LoggingSettings = Field(default_factory=LoggingSettings)
    replay: ReplaySettings = Field(default_factory=ReplaySettings)
    game_bridge: GameBridgeSettings = Field(default_factory=GameBridgeSettings)
