"""Run TikTok Event Gateway for one TikTok LIVE target.

Usage:
    python scripts/listen_live.py "https://www.tiktok.com/@some_user/live"
    python scripts/listen_live.py "@some_user"
"""

from __future__ import annotations

import argparse
import asyncio
import re
import sys
from pathlib import Path
from typing import Any
from urllib.parse import urlparse


REPO_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = REPO_ROOT / "src"
if str(SRC_ROOT) not in sys.path:
    sys.path.insert(0, str(SRC_ROOT))

from tiktok_event_gateway.utils.tiktok import build_tiktok_live_url, normalize_tiktok_username


def build_parser() -> argparse.ArgumentParser:
    """Create the command-line parser."""
    parser = argparse.ArgumentParser(
        prog="listen_live.py",
        description="Listen to a TikTok LIVE through TikTok Event Gateway.",
    )
    parser.add_argument(
        "live",
        help="TikTok LIVE URL, profile URL, @username, or username.",
    )
    parser.add_argument(
        "--config",
        default=str(REPO_ROOT / "config" / "tiktoklive-full-logs.yaml"),
        help="Gateway YAML config to use.",
    )
    parser.add_argument(
        "--event-log",
        default=None,
        help="Optional JSONL event log path. Defaults to logs/<username>-events.jsonl.",
    )
    parser.add_argument(
        "--host",
        default=None,
        help="Override WebSocket host from config.",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=None,
        help="Override WebSocket port from config.",
    )
    parser.add_argument(
        "--path",
        default=None,
        help="Override WebSocket path from config.",
    )
    parser.add_argument(
        "--game-bridge-enabled",
        action="store_true",
        help="Enable the rule-based game bridge.",
    )
    parser.add_argument(
        "--game-rules-path",
        default=None,
        help="Path to the game-specific XML rules file.",
    )
    parser.add_argument(
        "--game-templates-path",
        default=None,
        help="Path to the game-specific JSON templates file.",
    )
    parser.add_argument(
        "--game-output-type",
        choices=["stdout", "udp"],
        default=None,
        help="Game bridge output transport.",
    )
    parser.add_argument(
        "--game-output-host",
        default=None,
        help="Game bridge output host for UDP.",
    )
    parser.add_argument(
        "--game-output-port",
        type=int,
        default=None,
        help="Game bridge output port for UDP.",
    )
    parser.add_argument(
        "--game-output-url",
        default=None,
        help="Reserved game bridge output URL for future transports.",
    )
    parser.add_argument(
        "--game-log-unmatched-events",
        action="store_true",
        help="Log normalized events that do not match any game bridge rule.",
    )
    return parser


def extract_live_username(value: str) -> str:
    """Extract a TikTok username from a LIVE URL, profile URL, or raw username."""
    text = value.strip()
    if not text:
        msg = "Live URL or username is required."
        raise ValueError(msg)

    if "://" in text:
        parsed = urlparse(text)
        match = re.search(r"/@([^/?#]+)/?", parsed.path)
        if not match:
            msg = f"Could not find @username in TikTok URL: {value}"
            raise ValueError(msg)
        username = normalize_tiktok_username(match.group(1))
        if not username:
            msg = f"Invalid TikTok username in URL: {value}"
            raise ValueError(msg)
        return username

    username = normalize_tiktok_username(text.split("?", 1)[0].split("/", 1)[0])
    if not username:
        msg = f"Invalid TikTok username: {value}"
        raise ValueError(msg)
    return username


def default_event_log_path(username: str) -> Path:
    """Build a useful default event log path for a live username."""
    safe_username = normalize_tiktok_username(username).replace("/", "_")
    return REPO_ROOT / "logs" / f"{safe_username}-events.jsonl"


def client_websocket_url(host: str, port: int, path: str) -> str:
    """Return the URL local clients should use for the WebSocket server."""
    client_host = "127.0.0.1" if host in {"0.0.0.0", "::"} else host
    return f"ws://{client_host}:{port}{path}"


def load_gateway_dependencies() -> tuple[Any, Any, Any, Any]:
    """Import gateway dependencies only when the script actually runs."""
    try:
        from tiktok_event_gateway.config import load_config
        from tiktok_event_gateway.logging import configure_logging
        from tiktok_event_gateway.models import EventType
        from tiktok_event_gateway.runtime import run_gateway
    except ModuleNotFoundError as exc:
        msg = (
            "Missing Python dependencies. Activate your virtualenv and run: "
            'python -m pip install -e ".[tiktok]"'
        )
        raise RuntimeError(msg) from exc
    return load_config, configure_logging, EventType, run_gateway


async def run_from_args(args: argparse.Namespace) -> None:
    """Load config, apply target overrides, and run the gateway."""
    load_config, configure_logging, event_type, run_gateway = load_gateway_dependencies()
    username = extract_live_username(args.live)
    settings = load_config(args.config)

    settings.connector.provider = "tiktok"
    settings.connector.tiktok.live_username = username
    settings.connector.tiktok.include_raw_event_debug = True
    settings.connector.tiktok.enabled_event_types = [
        event_type.COMMENT,
        event_type.JOIN,
        event_type.GIFT,
        event_type.LIKE,
        event_type.FOLLOW,
        event_type.CONNECT,
        event_type.DISCONNECT,
        event_type.ERROR,
    ]
    settings.logging.level = "DEBUG"
    settings.logging.json = True
    settings.logging.event_log.enabled = True
    settings.logging.event_log.include_raw_event = True
    settings.logging.event_log.path = (
        Path(args.event_log) if args.event_log else default_event_log_path(username)
    )

    if args.host is not None:
        settings.websocket.host = args.host
    if args.port is not None:
        settings.websocket.port = args.port
    if args.path is not None:
        settings.websocket.path = args.path
    if args.game_bridge_enabled:
        settings.game_bridge.enabled = True
    if args.game_rules_path is not None:
        settings.game_bridge.rules_path = Path(args.game_rules_path)
    if args.game_templates_path is not None:
        settings.game_bridge.templates_path = Path(args.game_templates_path)
    if args.game_output_type is not None:
        settings.game_bridge.output.type = args.game_output_type
    if args.game_output_host is not None:
        settings.game_bridge.output.host = args.game_output_host
    if args.game_output_port is not None:
        settings.game_bridge.output.port = args.game_output_port
    if args.game_output_url is not None:
        settings.game_bridge.output.url = args.game_output_url
    if args.game_log_unmatched_events:
        settings.game_bridge.log_unmatched_events = True

    ws_url = client_websocket_url(
        settings.websocket.host,
        settings.websocket.port,
        settings.websocket.path,
    )

    print("TikTok Event Gateway live listener")
    print(f"Live target: @{username}")
    print(f"Live URL: {build_tiktok_live_url(username)}")
    print(f"WebSocket: {ws_url}")
    print(f"Event log: {settings.logging.event_log.path}")
    if settings.game_bridge.enabled:
        print("Game bridge: enabled")
        print(f"Game rules: {settings.game_bridge.rules_path}")
        print(f"Game templates: {settings.game_bridge.templates_path}")
        print(f"Game output: {settings.game_bridge.output.type}")
        if settings.game_bridge.output.type == "udp":
            print(
                "Game UDP target: "
                f"{settings.game_bridge.output.host}:{settings.game_bridge.output.port}"
            )
    else:
        print("Game bridge: disabled")
    print()
    print("In another terminal you can run:")
    print("  python -m examples.clients.websocket_printer --pretty")
    print("  python -m examples.clients.gift_subscriber")
    print()
    print("Starting gateway. Press Ctrl+C to stop.")
    print()

    configure_logging(settings.logging)
    await run_gateway(settings)


def main() -> None:
    """Script entrypoint."""
    args = build_parser().parse_args()
    try:
        asyncio.run(run_from_args(args))
    except KeyboardInterrupt:
        print("Stopped.")
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(2) from exc


if __name__ == "__main__":
    main()
