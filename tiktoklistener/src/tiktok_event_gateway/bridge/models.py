"""Models used by the rule-based game bridge.

The bridge is intentionally provider-agnostic: it receives normalized gateway
JSON events and emits game-specific JSON messages using external XML rules and
JSON templates supplied by each game application.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


JsonDict = dict[str, Any]


@dataclass(frozen=True)
class RuleCondition:
    """One XML condition that must pass for a rule to match."""

    field: str
    operator: str
    expected: Any = None
    ignore_case: bool = False
    value_type: str = "str"


@dataclass(frozen=True)
class RuleParam:
    """One parameter provided by a rule message."""

    name: str
    value: Any = None
    from_path: str | None = None
    default: Any = None
    value_type: str = "str"


@dataclass(frozen=True)
class RuleMessage:
    """A message declaration inside a rule."""

    template: str
    params: tuple[RuleParam, ...] = ()


@dataclass(frozen=True)
class GameRule:
    """A game-specific rule loaded from XML.

    The bridge supports two rule formats:

    * Verbose rules using <condition> and <message>/<param>.
    * Compact rules using attributes directly on <rule>:
      event_type, event_value, action, and optional template.
    """

    rule_id: str
    event_type: str
    conditions: tuple[RuleCondition, ...] = ()
    messages: tuple[RuleMessage, ...] = ()
    event_value: str | None = None
    action: str | None = None
    compact: bool = False

    def to_template_context(self) -> dict[str, Any]:
        """Return rule data available to JSON templates as {{rule.*}}."""
        return {
            "id": self.rule_id,
            "event_type": self.event_type,
            "event_value": self.event_value,
            "action": self.action,
            "compact": self.compact,
        }


@dataclass(frozen=True)
class TemplateBundle:
    """JSON templates loaded from a game-specific template file."""

    path: Path
    templates: dict[str, JsonDict] = field(default_factory=dict)
