"""Tests for YAML configuration loading and validation."""

from __future__ import annotations

from pathlib import Path

import pytest

from tiktok_event_gateway.config import AppSettings, ConfigError, dump_config, load_config


def test_load_valid_mock_config(tmp_path: Path) -> None:
    config_path = tmp_path / "mock.yaml"
    config_path.write_text(
        """
gateway:
  name: "test-gateway"
connector:
  provider: "mock"
  mock:
    interval_seconds: 0.1
websocket:
  host: "127.0.0.1"
  port: 8765
  path: "/events"
logging:
  level: "INFO"
  json: true
""",
        encoding="utf-8",
    )

    settings = load_config(config_path)

    assert settings.gateway.name == "test-gateway"
    assert settings.connector.provider == "mock"
    assert settings.connector.mock.interval_seconds == 0.1


def test_bad_config_fails_early_with_clear_message(tmp_path: Path) -> None:
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

    with pytest.raises(ConfigError) as exc_info:
        load_config(config_path)

    message = str(exc_info.value)
    assert "connector.tiktok.live_username" in message
    assert "websocket.path" in message


def test_dump_config_round_trips(tmp_path: Path) -> None:
    config_path = tmp_path / "generated.yaml"
    config_path.write_text(dump_config(AppSettings()), encoding="utf-8")

    settings = load_config(config_path)

    assert settings.connector.provider == "mock"
    assert settings.websocket.path == "/events"


@pytest.mark.parametrize(
    "config_path",
    [
        Path("config/example.yaml"),
        Path("config/mock.yaml"),
        Path("config/tiktoklive.yaml"),
        Path("config/websocket.yaml"),
        Path("config/logging.yaml"),
    ],
)
def test_checked_in_config_examples_are_valid(config_path: Path) -> None:
    settings = load_config(config_path)

    assert settings.websocket.path.startswith("/")
