"""Rule-based bridge from normalized gateway events to game JSON messages."""

from tiktok_event_gateway.bridge.game_bridge import RuleBasedGameBridge
from tiktok_event_gateway.bridge.rule_engine import RuleEngine
from tiktok_event_gateway.bridge.rule_loader import RuleLoadError, load_rules
from tiktok_event_gateway.bridge.template_loader import (
    TemplateLoadError,
    load_templates,
    validate_rule_template_references,
)
from tiktok_event_gateway.bridge.template_renderer import TemplateRenderer

__all__ = [
    "RuleBasedGameBridge",
    "RuleEngine",
    "RuleLoadError",
    "TemplateLoadError",
    "TemplateRenderer",
    "load_rules",
    "load_templates",
    "validate_rule_template_references",
]
