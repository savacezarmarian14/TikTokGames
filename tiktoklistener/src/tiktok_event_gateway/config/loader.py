"""YAML configuration loader.

This module will read YAML files and convert them into validated settings
models. Environment override behavior can be added here later.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml
from pydantic import ValidationError

from tiktok_event_gateway.config.settings import AppSettings


class ConfigError(ValueError):
    """Raised when configuration cannot be loaded or validated."""


def load_config(path: str | Path) -> AppSettings:
    """Load and strongly validate a YAML configuration file."""
    config_path = Path(path)
    if not config_path.exists():
        msg = f"config file does not exist: {config_path}"
        raise ConfigError(msg)

    try:
        raw_data = yaml.safe_load(config_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as exc:
        msg = f"invalid YAML in {config_path}: {exc}"
        raise ConfigError(msg) from exc

    if not isinstance(raw_data, dict):
        msg = f"config root must be a YAML mapping: {config_path}"
        raise ConfigError(msg)

    try:
        return AppSettings.model_validate(raw_data)
    except ValidationError as exc:
        msg = f"invalid config {config_path}: {exc}"
        raise ConfigError(msg) from exc


def dump_config(settings: AppSettings) -> str:
    """Serialize settings as YAML."""
    data: dict[str, Any] = settings.model_dump(mode="json")
    return yaml.safe_dump(data, sort_keys=False)
