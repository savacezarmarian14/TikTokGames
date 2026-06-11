"""Load and validate game bridge JSON templates."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from tiktok_event_gateway.bridge.models import GameRule, TemplateBundle


class TemplateLoadError(ValueError):
    """Raised when game bridge templates cannot be loaded or validated."""


def load_templates(path: str | Path) -> TemplateBundle:
    """Load a JSON template bundle from disk."""
    template_path = Path(path)
    if not template_path.exists():
        msg = f"game bridge templates file does not exist: {template_path}"
        raise TemplateLoadError(msg)
    try:
        raw = json.loads(template_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        msg = f"invalid game bridge templates JSON in {template_path}: {exc}"
        raise TemplateLoadError(msg) from exc
    if not isinstance(raw, dict) or not isinstance(raw.get("templates"), dict):
        msg = "game bridge templates JSON must contain a top-level object named 'templates'"
        raise TemplateLoadError(msg)
    templates: dict[str, dict[str, Any]] = {}
    for name, template in raw["templates"].items():
        if not isinstance(name, str) or not name:
            msg = "template names must be non-empty strings"
            raise TemplateLoadError(msg)
        if not isinstance(template, dict):
            msg = f"template {name!r} must be a JSON object"
            raise TemplateLoadError(msg)
        templates[name] = template
    return TemplateBundle(path=template_path, templates=templates)


def validate_rule_template_references(rules: list[GameRule], bundle: TemplateBundle) -> None:
    """Fail early if a rule references a missing template."""
    missing: list[str] = []
    for rule in rules:
        for message in rule.messages:
            if message.template not in bundle.templates:
                missing.append(f"{rule.rule_id}:{message.template}")
    if missing:
        msg = "game bridge rules reference missing templates: " + ", ".join(missing)
        raise TemplateLoadError(msg)
