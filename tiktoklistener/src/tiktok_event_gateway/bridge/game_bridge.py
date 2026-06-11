"""Optional rule-based bridge from normalized TikTok events to game JSON messages."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import structlog

from tiktok_event_gateway.bridge.deduplication import EventIdDeduplicator
from tiktok_event_gateway.bridge.models import GameRule, TemplateBundle
from tiktok_event_gateway.bridge.output_transport import GameBridgeOutputTransport, build_output_transport
from tiktok_event_gateway.bridge.rule_engine import RuleEngine
from tiktok_event_gateway.bridge.rule_loader import load_rules
from tiktok_event_gateway.bridge.template_loader import load_templates, validate_rule_template_references
from tiktok_event_gateway.bridge.template_renderer import TemplateRenderer
from tiktok_event_gateway.bridge.utils import event_to_mapping

logger = structlog.get_logger(__name__)


class RuleBasedGameBridge:
    """Process normalized gateway events and send generated game JSON messages."""

    def __init__(
        self,
        *,
        rules_path: str | Path,
        templates_path: str | Path,
        output_transport: GameBridgeOutputTransport,
        log_unmatched_events: bool = False,
        deduplication_enabled: bool = True,
        deduplication_max_size: int = 10000,
    ) -> None:
        self.rules_path = Path(rules_path)
        self.templates_path = Path(templates_path)
        self.output_transport = output_transport
        self.log_unmatched_events = log_unmatched_events
        self.deduplicator = (
            EventIdDeduplicator(deduplication_max_size) if deduplication_enabled else None
        )
        self.rules: list[GameRule] = []
        self.template_bundle: TemplateBundle | None = None
        self.rule_engine: RuleEngine | None = None
        self.processed_events = 0
        self.generated_messages = 0
        self.failed_events = 0
        self.unmatched_events = 0

    @classmethod
    def from_settings(cls, settings: Any) -> "RuleBasedGameBridge | None":
        """Build a bridge from AppSettings. Return None when disabled."""
        bridge_settings = getattr(settings, "game_bridge", None)
        if bridge_settings is None or not bridge_settings.enabled:
            return None
        if bridge_settings.rules_path is None:
            msg = "game_bridge.rules_path is required when game_bridge.enabled=true"
            raise ValueError(msg)
        if bridge_settings.templates_path is None:
            msg = "game_bridge.templates_path is required when game_bridge.enabled=true"
            raise ValueError(msg)
        output = bridge_settings.output
        return cls(
            rules_path=bridge_settings.rules_path,
            templates_path=bridge_settings.templates_path,
            output_transport=build_output_transport(
                output_type=output.type,
                host=output.host,
                port=output.port,
            ),
            log_unmatched_events=bridge_settings.log_unmatched_events,
            deduplication_enabled=bridge_settings.deduplication_enabled,
            deduplication_max_size=bridge_settings.deduplication_max_size,
        )

    async def start(self) -> None:
        """Load rules/templates once and start the output transport."""
        self.rules = load_rules(self.rules_path)
        self.template_bundle = load_templates(self.templates_path)
        validate_rule_template_references(self.rules, self.template_bundle)
        self.rule_engine = RuleEngine(self.rules, TemplateRenderer(self.template_bundle.templates))
        await self.output_transport.start()
        logger.info(
            "game bridge started",
            rules_path=str(self.rules_path),
            templates_path=str(self.templates_path),
            rules_count=len(self.rules),
        )

    async def process_event(self, event: Any) -> None:
        """Convert one normalized event into game messages and send them."""
        if self.rule_engine is None:
            msg = "game bridge must be started before processing events"
            raise RuntimeError(msg)
        try:
            event_dict = event_to_mapping(event)
            event_id = event_dict.get("event_id")
            if self.deduplicator is not None and self.deduplicator.seen_or_add(str(event_id)):
                return
            self.processed_events += 1
            messages = self.rule_engine.render_messages(event_dict)
            if not messages:
                self.unmatched_events += 1
                if self.log_unmatched_events:
                    logger.debug(
                        "game bridge event did not match any rule",
                        event_id=event_id,
                        event_type=event_dict.get("event_type"),
                    )
                return
            for message in messages:
                await self.output_transport.send(message)
                self.generated_messages += 1
        except Exception:
            self.failed_events += 1
            logger.exception("game bridge failed to process event")

    async def stop(self) -> None:
        """Stop the configured output transport."""
        await self.output_transport.stop()
        logger.info(
            "game bridge stopped",
            processed_events=self.processed_events,
            generated_messages=self.generated_messages,
            failed_events=self.failed_events,
            unmatched_events=self.unmatched_events,
        )
