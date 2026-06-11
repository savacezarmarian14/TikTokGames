"""Utility helpers for the rule-based game bridge."""

from __future__ import annotations

from enum import Enum
from typing import Any

MISSING = object()


def event_to_mapping(event: Any) -> dict[str, Any]:
    """Return a JSON-like dictionary from a normalized gateway event or mapping."""
    if isinstance(event, dict):
        return event
    to_dict = getattr(event, "to_dict", None)
    if callable(to_dict):
        return to_dict(include_raw=False)
    model_dump = getattr(event, "model_dump", None)
    if callable(model_dump):
        return model_dump(mode="json", exclude_none=True, exclude={"raw_event"})
    msg = f"unsupported event object: {type(event).__name__}"
    raise TypeError(msg)


def read_dot_path(data: Any, path: str, *, default: Any = MISSING) -> Any:
    """Safely read a dot-separated path from dictionaries or simple objects."""
    current = data
    for part in path.split("."):
        if current is MISSING or current is None:
            return default
        if isinstance(current, dict):
            if part not in current:
                return default
            current = current[part]
            continue
        try:
            current = getattr(current, part)
        except AttributeError:
            return default
        except Exception:
            return default
        if isinstance(current, Enum):
            current = current.value
    return current


def convert_value(value: Any, value_type: str = "str") -> Any:
    """Convert XML string values into the configured primitive type."""
    normalized = (value_type or "str").strip().lower()
    if value is None:
        return None
    if normalized == "str":
        return str(value)
    if normalized == "int":
        if isinstance(value, bool):
            return int(value)
        return int(value)
    if normalized == "float":
        return float(value)
    if normalized == "bool":
        if isinstance(value, bool):
            return value
        text = str(value).strip().lower()
        if text in {"1", "true", "yes", "y", "on"}:
            return True
        if text in {"0", "false", "no", "n", "off"}:
            return False
        msg = f"cannot convert {value!r} to bool"
        raise ValueError(msg)
    msg = f"unsupported value type: {value_type}"
    raise ValueError(msg)
