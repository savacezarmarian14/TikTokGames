"""Command-line interface for TikTok Event Gateway."""

from __future__ import annotations

import asyncio
from collections.abc import Coroutine
from pathlib import Path
from typing import Any, Optional

import typer

from tiktok_event_gateway.config import AppSettings, ConfigError, dump_config, load_config
from tiktok_event_gateway.config.settings import (
    ConnectorSettings,
    EventLogSettings,
    LoggingSettings,
    TikTokConnectorSettings,
)
from tiktok_event_gateway.logging import configure_logging
from tiktok_event_gateway.runtime import replay_event_log, run_gateway

app = typer.Typer(
    name="tiktok-event-gateway",
    help="Reusable middleware for normalizing TikTok LIVE events over WebSockets.",
    no_args_is_help=True,
)


@app.command()
def run(
    config: Path = typer.Option(
        Path("config/example.yaml"),
        "--config",
        "-c",
        help="YAML configuration file.",
    ),
    game_bridge_enabled: bool = typer.Option(
        False,
        "--game-bridge-enabled",
        help="Enable the optional rule-based game bridge.",
    ),
    game_rules_path: Optional[Path] = typer.Option(
        None,
        "--game-rules-path",
        help="Game-specific XML rules file.",
    ),
    game_templates_path: Optional[Path] = typer.Option(
        None,
        "--game-templates-path",
        help="Game-specific JSON templates file.",
    ),
    game_output_type: Optional[str] = typer.Option(
        None,
        "--game-output-type",
        help="Game bridge output transport: stdout or udp.",
    ),
    game_output_host: Optional[str] = typer.Option(
        None,
        "--game-output-host",
        help="Game bridge output host for UDP.",
    ),
    game_output_port: Optional[int] = typer.Option(
        None,
        "--game-output-port",
        help="Game bridge output port for UDP.",
    ),
) -> None:
    """Run the gateway with a live connector and WebSocket server."""
    settings = _load_or_exit(config)
    _apply_game_bridge_overrides(
        settings,
        enabled=game_bridge_enabled,
        rules_path=game_rules_path,
        templates_path=game_templates_path,
        output_type=game_output_type,
        output_host=game_output_host,
        output_port=game_output_port,
    )
    configure_logging(settings.logging)
    _run_async(run_gateway(settings))


@app.command("validate-config")
def validate_config(
    config: Path = typer.Option(
        Path("config/example.yaml"),
        "--config",
        "-c",
        help="YAML configuration file.",
    ),
) -> None:
    """Validate a YAML configuration file and exit."""
    settings = _load_or_exit(config)
    typer.echo(f"Config is valid: {config}")
    typer.echo(f"Provider: {settings.connector.provider}")
    typer.echo(
        f"WebSocket: ws://{settings.websocket.host}:"
        f"{settings.websocket.port}{settings.websocket.path}"
    )


@app.command("replay-events")
def replay_events(
    event_log: Path = typer.Argument(..., help="JSONL file containing normalized gateway events."),
    config: Path = typer.Option(
        Path("config/example.yaml"),
        "--config",
        "-c",
        help="YAML configuration file for WebSocket/logging settings.",
    ),
    speed: Optional[float] = typer.Option(
        None,
        "--speed",
        min=0.0001,
        help="Replay speed multiplier. 2.0 is twice as fast.",
    ),
    start_delay: float = typer.Option(
        1.0,
        "--start-delay",
        min=0.0,
        help="Seconds to wait after starting WebSocket server before replaying.",
    ),
) -> None:
    """Replay a normalized JSONL event log through the WebSocket broker."""
    settings = _load_or_exit(config)
    configure_logging(settings.logging)

    _run_async(
        replay_event_log(
            event_log_path=event_log,
            settings=settings,
            speed_multiplier=speed,
            start_delay_seconds=start_delay,
        )
    )


@app.command("generate-example-config")
def generate_example_config(
    output: Path = typer.Option(..., "--output", "-o", help="File to write."),
    mode: str = typer.Option(
        "mock",
        "--mode",
        help="Example mode: mock, tiktok, websocket, or logging.",
    ),
) -> None:
    """Generate an example YAML configuration file."""
    settings = _example_settings(mode)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(dump_config(settings), encoding="utf-8")
    typer.echo(f"Wrote example config: {output}")



def _apply_game_bridge_overrides(
    settings: AppSettings,
    *,
    enabled: bool = False,
    rules_path: Optional[Path] = None,
    templates_path: Optional[Path] = None,
    output_type: Optional[str] = None,
    output_host: Optional[str] = None,
    output_port: Optional[int] = None,
) -> None:
    """Apply optional CLI overrides for RuleBasedGameBridge."""
    if enabled:
        settings.game_bridge.enabled = True
    if rules_path is not None:
        settings.game_bridge.rules_path = rules_path
    if templates_path is not None:
        settings.game_bridge.templates_path = templates_path
    if output_type is not None:
        settings.game_bridge.output.type = output_type
    if output_host is not None:
        settings.game_bridge.output.host = output_host
    if output_port is not None:
        settings.game_bridge.output.port = output_port

def _load_or_exit(path: Path) -> AppSettings:
    try:
        return load_config(path)
    except ConfigError as exc:
        typer.secho(str(exc), fg=typer.colors.RED, err=True)
        raise typer.Exit(code=2) from exc


def _run_async(coro: Coroutine[Any, Any, None]) -> None:
    try:
        asyncio.run(coro)
    except KeyboardInterrupt:
        typer.echo("Interrupted", err=True)


def _example_settings(mode: str) -> AppSettings:
    normalized = mode.strip().lower()
    if normalized == "mock":
        return AppSettings()
    if normalized == "tiktok":
        return AppSettings(
            connector=ConnectorSettings(
                provider="tiktok",
                tiktok=TikTokConnectorSettings(live_username="@example_user"),
            )
        )
    if normalized == "websocket":
        return AppSettings()
    if normalized == "logging":
        return AppSettings(
            logging=LoggingSettings(
                json=True,
                event_log=EventLogSettings(enabled=True, path=Path("logs/events.jsonl")),
            )
        )
    typer.secho(
        "mode must be one of: mock, tiktok, websocket, logging",
        fg=typer.colors.RED,
        err=True,
    )
    raise typer.Exit(code=2)


def main() -> None:
    """Run the Typer application."""
    app()


if __name__ == "__main__":
    main()
