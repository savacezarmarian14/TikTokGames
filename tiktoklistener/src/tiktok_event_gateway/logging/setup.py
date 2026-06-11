"""Structured logging configuration.

This module will configure structlog and standard-library logging so production
logs are machine-readable while local development remains pleasant.
"""

from __future__ import annotations

import logging
import sys

import structlog

from tiktok_event_gateway.config import LoggingSettings


def configure_logging(settings: LoggingSettings) -> None:
    """Configure standard logging and structlog."""
    root_level = getattr(logging, settings.level)
    logging.basicConfig(
        level=root_level,
        format="%(message)s",
        stream=sys.stdout,
        force=True,
    )

    # Keep our application debug logs useful without flooding the terminal with
    # low-level HTTP/TLS traces emitted by TikTokLive dependencies.
    for noisy_logger_name in ("httpcore", "httpx", "hpack", "h11"):
        logging.getLogger(noisy_logger_name).setLevel(logging.WARNING)

    processors = [
        structlog.contextvars.merge_contextvars,
        structlog.processors.add_log_level,
        structlog.processors.TimeStamper(fmt="iso", utc=True),
        structlog.processors.StackInfoRenderer(),
        structlog.processors.format_exc_info,
    ]
    if settings.json:
        processors.append(structlog.processors.JSONRenderer())
    else:
        processors.append(structlog.dev.ConsoleRenderer())

    structlog.configure(
        processors=processors,
        wrapper_class=structlog.make_filtering_bound_logger(root_level),
        logger_factory=structlog.PrintLoggerFactory(),
        cache_logger_on_first_use=False,
    )
