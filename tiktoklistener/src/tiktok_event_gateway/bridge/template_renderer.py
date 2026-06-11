"""Render game JSON messages from JSON templates."""

from __future__ import annotations

import copy
import re
from typing import Any

from tiktok_event_gateway.bridge.utils import MISSING, read_dot_path

_PLACEHOLDER_RE = re.compile(r"{{\s*([^{}]+?)\s*}}")


class TemplateRenderError(ValueError):
    """Raised for unrecoverable template rendering errors."""


class TemplateRenderer:
    """Render nested JSON-compatible templates with event and params context."""

    def __init__(self, templates: dict[str, dict[str, Any]]) -> None:
        self._templates = templates

    def render(
        self,
        template_name: str,
        *,
        event: dict[str, Any],
        params: dict[str, Any] | None = None,
        rule: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        """Render one template into a JSON-serializable dictionary.

        Template placeholders can read from:
        - {{event.*}} for the normalized TikTok event
        - {{params.*}} for verbose rule parameters
        - {{rule.*}} for compact or verbose rule metadata
        """
        if template_name not in self._templates:
            msg = f"unknown game bridge template: {template_name}"
            raise TemplateRenderError(msg)
        context = {"event": event, "params": params or {}, "rule": rule or {}}
        rendered = self._render_value(copy.deepcopy(self._templates[template_name]), context)
        if not isinstance(rendered, dict):
            msg = f"game bridge template {template_name!r} must render to a JSON object"
            raise TemplateRenderError(msg)
        return rendered

    def _render_value(self, value: Any, context: dict[str, Any]) -> Any:
        if isinstance(value, dict):
            return {key: self._render_value(item, context) for key, item in value.items()}
        if isinstance(value, list):
            return [self._render_value(item, context) for item in value]
        if isinstance(value, str):
            return self._render_string(value, context)
        return value

    def _render_string(self, value: str, context: dict[str, Any]) -> Any:
        exact = _PLACEHOLDER_RE.fullmatch(value)
        if exact:
            resolved = read_dot_path(context, exact.group(1).strip(), default=MISSING)
            return None if resolved is MISSING else resolved

        def replace(match: re.Match[str]) -> str:
            resolved = read_dot_path(context, match.group(1).strip(), default=MISSING)
            return "" if resolved is MISSING or resolved is None else str(resolved)

        return _PLACEHOLDER_RE.sub(replace, value)
