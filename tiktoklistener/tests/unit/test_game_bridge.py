"""Tests for the optional RuleBasedGameBridge."""

from __future__ import annotations

import asyncio
import json
import socket
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import pytest

from tiktok_event_gateway.bridge import RuleBasedGameBridge, TemplateRenderer, load_rules, load_templates
from tiktok_event_gateway.bridge.deduplication import EventIdDeduplicator
from tiktok_event_gateway.bridge.output_transport import UdpGameBridgeTransport
from tiktok_event_gateway.bridge.rule_engine import RuleEngine
from tiktok_event_gateway.bridge.rule_loader import RuleLoadError
from tiktok_event_gateway.bridge.template_loader import TemplateLoadError, validate_rule_template_references
from tiktok_event_gateway.config import ConfigError, load_config
from tiktok_event_gateway.models import (
    CommentEvent,
    CommentPayload,
    EventType,
    FollowEvent,
    FollowPayload,
    GiftEvent,
    GiftPayload,
    LikeEvent,
    LikePayload,
    Source,
    SourcePlatform,
    User,
)


class CollectTransport:
    def __init__(self) -> None:
        self.started = False
        self.stopped = False
        self.messages: list[dict[str, Any]] = []

    async def start(self) -> None:
        self.started = True

    async def send(self, message: dict[str, Any]) -> None:
        self.messages.append(message)

    async def stop(self) -> None:
        self.stopped = True


def write_rules(tmp_path: Path, text: str) -> Path:
    path = tmp_path / "rules.xml"
    path.write_text(text, encoding="utf-8")
    return path


def write_templates(tmp_path: Path, text: str | None = None) -> Path:
    path = tmp_path / "templates.json"
    path.write_text(
        text
        or json.dumps(
            {
                "templates": {
                    "game_action": {
                        "message_type": "game_action",
                        "action": "{{params.action}}",
                        "amount": "{{params.amount}}",
                        "username": "{{event.user.username}}",
                    }
                }
            }
        ),
        encoding="utf-8",
    )
    return path


def gift_event() -> GiftEvent:
    return GiftEvent(
        event_id="evt-gift-1",
        timestamp=datetime(2026, 1, 1, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.TIKTOK, connector="test"),
        user=User(user_id="u1", username="viewer", display_name="Viewer"),
        payload=GiftPayload(
            gift_id="rose",
            gift_name="Rose",
            repeat_count=3,
            diamond_count=1,
            repeat_end=True,
        ),
    )


def comment_event(text: str = "!attack") -> CommentEvent:
    return CommentEvent(
        event_id="evt-comment-1",
        timestamp=datetime(2026, 1, 1, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.TIKTOK, connector="test"),
        user=User(user_id="u1", username="viewer", display_name="Viewer"),
        payload=CommentPayload(comment=text),
    )


def engine_from_xml(tmp_path: Path, xml: str) -> RuleEngine:
    rules = load_rules(write_rules(tmp_path, xml))
    bundle = load_templates(write_templates(tmp_path))
    validate_rule_template_references(rules, bundle)
    return RuleEngine(rules, TemplateRenderer(bundle.templates))


def test_load_rules_valid_xml(tmp_path: Path) -> None:
    rules = load_rules(
        write_rules(
            tmp_path,
            """
<rules>
  <rule id="r1" event_type="gift">
    <condition field="payload.gift_name" equals="Rose" />
    <message template="game_action"><param name="action" value="spawn" /></message>
  </rule>
</rules>
""",
        )
    )

    assert len(rules) == 1
    assert rules[0].rule_id == "r1"
    assert rules[0].event_type == "gift"


def test_load_rules_invalid_xml(tmp_path: Path) -> None:
    path = write_rules(tmp_path, "<rules><rule></rules>")

    with pytest.raises(RuleLoadError):
        load_rules(path)


def test_missing_template_reference_fails(tmp_path: Path) -> None:
    rules = load_rules(
        write_rules(
            tmp_path,
            """
<rules>
  <rule id="r1" event_type="gift">
    <message template="missing"><param name="action" value="spawn" /></message>
  </rule>
</rules>
""",
        )
    )
    bundle = load_templates(write_templates(tmp_path))

    with pytest.raises(TemplateLoadError):
        validate_rule_template_references(rules, bundle)


@pytest.mark.parametrize(
    ("condition", "should_match"),
    [
        ('field="payload.gift_name" equals="Rose"', True),
        ('field="payload.gift_name" not_equals="TikTok"', True),
        ('field="payload.gift_name" contains="os"', True),
        ('field="payload.gift_name" starts_with="Ro"', True),
        ('field="payload.repeat_count" greater_than="2" type="int"', True),
        ('field="payload.repeat_count" less_than="2" type="int"', False),
        ('field="user.username" exists="true" type="bool"', True),
        ('field="payload.gift_name" equals="rose" ignore_case="true"', True),
    ],
)
def test_rule_conditions(tmp_path: Path, condition: str, should_match: bool) -> None:
    engine = engine_from_xml(
        tmp_path,
        f"""
<rules>
  <rule id="r1" event_type="gift">
    <condition {condition} />
    <message template="game_action">
      <param name="action" value="spawn" />
      <param name="amount" from="payload.repeat_count" type="int" default="1" />
    </message>
  </rule>
</rules>
""",
    )

    messages = engine.render_messages(gift_event().to_dict())

    assert bool(messages) is should_match
    if should_match:
        assert messages[0]["amount"] == 3
        assert messages[0]["username"] == "viewer"


def test_event_type_matching_ignores_other_events(tmp_path: Path) -> None:
    engine = engine_from_xml(
        tmp_path,
        """
<rules>
  <rule id="r1" event_type="gift">
    <message template="game_action"><param name="action" value="spawn" /></message>
  </rule>
</rules>
""",
    )

    assert engine.render_messages(comment_event().to_dict()) == []


def test_param_default_and_type_conversion(tmp_path: Path) -> None:
    engine = engine_from_xml(
        tmp_path,
        """
<rules>
  <rule id="r1" event_type="gift">
    <message template="game_action">
      <param name="action" value="spawn" />
      <param name="amount" from="payload.missing" default="7" type="int" />
    </message>
  </rule>
</rules>
""",
    )

    messages = engine.render_messages(gift_event().to_dict())

    assert messages[0]["amount"] == 7
    assert isinstance(messages[0]["amount"], int)


def test_template_renderer_nested_and_missing_placeholder() -> None:
    renderer = TemplateRenderer(
        {
            "nested": {
                "outer": {
                    "amount": "{{params.amount}}",
                    "text": "hello {{event.user.username}}",
                    "missing": "{{event.payload.nope}}",
                },
                "items": ["{{params.amount}}"],
            }
        }
    )

    rendered = renderer.render(
        "nested",
        event={"user": {"username": "viewer"}, "payload": {}},
        params={"amount": 5},
    )

    assert rendered["outer"]["amount"] == 5
    assert rendered["outer"]["text"] == "hello viewer"
    assert rendered["outer"]["missing"] is None
    assert rendered["items"] == [5]


@pytest.mark.asyncio
async def test_bridge_generates_gift_and_comment_messages(tmp_path: Path) -> None:
    rules_path = write_rules(
        tmp_path,
        """
<rules>
  <rule id="gift" event_type="gift">
    <condition field="payload.gift_name" equals="Rose" />
    <message template="game_action">
      <param name="action" value="spawn" />
      <param name="amount" from="payload.repeat_count" type="int" default="1" />
    </message>
  </rule>
  <rule id="comment" event_type="comment">
    <condition field="payload.comment" equals="!attack" ignore_case="true" />
    <message template="game_action">
      <param name="action" value="attack" />
      <param name="amount" value="1" type="int" />
    </message>
  </rule>
</rules>
""",
    )
    templates_path = write_templates(tmp_path)
    transport = CollectTransport()
    bridge = RuleBasedGameBridge(
        rules_path=rules_path,
        templates_path=templates_path,
        output_transport=transport,
    )

    await bridge.start()
    await bridge.process_event(gift_event())
    await bridge.process_event(comment_event("!ATTACK"))
    await bridge.stop()

    assert transport.started is True
    assert transport.stopped is True
    assert [msg["action"] for msg in transport.messages] == ["spawn", "attack"]
    assert transport.messages[0]["amount"] == 3


@pytest.mark.asyncio
async def test_unmatched_event_produces_no_output(tmp_path: Path) -> None:
    rules_path = write_rules(
        tmp_path,
        """
<rules>
  <rule id="gift" event_type="gift">
    <condition field="payload.gift_name" equals="TikTok" />
    <message template="game_action"><param name="action" value="spawn" /></message>
  </rule>
</rules>
""",
    )
    transport = CollectTransport()
    bridge = RuleBasedGameBridge(
        rules_path=rules_path,
        templates_path=write_templates(tmp_path),
        output_transport=transport,
    )

    await bridge.start()
    await bridge.process_event(gift_event())
    await bridge.stop()

    assert transport.messages == []


def test_deduplicator_skips_duplicate_event_id() -> None:
    dedup = EventIdDeduplicator(max_size=2)

    assert dedup.seen_or_add("evt1") is False
    assert dedup.seen_or_add("evt1") is True


@pytest.mark.asyncio
async def test_bridge_deduplication_skips_duplicate_message(tmp_path: Path) -> None:
    rules_path = write_rules(
        tmp_path,
        """
<rules>
  <rule id="gift" event_type="gift">
    <message template="game_action"><param name="action" value="spawn" /></message>
  </rule>
</rules>
""",
    )
    transport = CollectTransport()
    bridge = RuleBasedGameBridge(
        rules_path=rules_path,
        templates_path=write_templates(tmp_path),
        output_transport=transport,
        deduplication_enabled=True,
    )

    await bridge.start()
    await bridge.process_event(gift_event())
    await bridge.process_event(gift_event())
    await bridge.stop()

    assert len(transport.messages) == 1


@pytest.mark.asyncio
async def test_udp_transport_sends_json_packet() -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 0))
    sock.setblocking(False)
    host, port = sock.getsockname()
    transport = UdpGameBridgeTransport(host=host, port=port)

    await transport.start()
    try:
        await transport.send({"action": "spawn", "amount": 1})
        data, _addr = await asyncio.wait_for(asyncio.get_running_loop().sock_recvfrom(sock, 4096), 1)
    finally:
        await transport.stop()
        sock.close()

    assert json.loads(data.decode("utf-8")) == {"action": "spawn", "amount": 1}


def test_config_rejects_enabled_bridge_without_paths(tmp_path: Path) -> None:
    config = tmp_path / "bad.yaml"
    config.write_text(
        """
game_bridge:
  enabled: true
""",
        encoding="utf-8",
    )

    with pytest.raises(ConfigError) as exc_info:
        load_config(config)

    assert "game_bridge.rules_path" in str(exc_info.value)


def write_compact_templates(tmp_path: Path) -> Path:
    path = tmp_path / "compact_templates.json"
    path.write_text(
        json.dumps(
            {
                "templates": {
                    "game_event": {
                        "message_type": "game_event",
                        "rule": {
                            "id": "{{rule.id}}",
                            "event_type": "{{rule.event_type}}",
                            "event_value": "{{rule.event_value}}",
                            "action": "{{rule.action}}",
                        },
                        "event": {
                            "id": "{{event.event_id}}",
                            "type": "{{event.event_type}}",
                        },
                        "user": {"username": "{{event.user.username}}"},
                        "payload": {
                            "gift_name": "{{event.payload.gift_name}}",
                            "comment": "{{event.payload.comment}}",
                            "like_count": "{{event.payload.like_count}}",
                        },
                    },
                    "custom_game_event": {
                        "message_type": "custom_game_event",
                        "action": "{{rule.action}}",
                        "value": "{{rule.event_value}}",
                        "username": "{{event.user.username}}",
                    },
                }
            }
        ),
        encoding="utf-8",
    )
    return path


def like_event() -> LikeEvent:
    return LikeEvent(
        event_id="evt-like-1",
        timestamp=datetime(2026, 1, 1, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.TIKTOK, connector="test"),
        user=User(user_id="u1", username="viewer", display_name="Viewer"),
        payload=LikePayload(like_count=7, total_like_count=100),
    )


def follow_event() -> FollowEvent:
    return FollowEvent(
        event_id="evt-follow-1",
        timestamp=datetime(2026, 1, 1, tzinfo=timezone.utc),
        source=Source(platform=SourcePlatform.TIKTOK, connector="test"),
        user=User(user_id="u1", username="viewer", display_name="Viewer"),
        payload=FollowPayload(followed=True),
    )


def compact_engine_from_xml(tmp_path: Path, xml: str) -> RuleEngine:
    rules = load_rules(write_rules(tmp_path, xml))
    bundle = load_templates(write_compact_templates(tmp_path))
    validate_rule_template_references(rules, bundle)
    return RuleEngine(rules, TemplateRenderer(bundle.templates))


def test_compact_gift_rule_defaults_to_game_event_template(tmp_path: Path) -> None:
    engine = compact_engine_from_xml(
        tmp_path,
        """
<rules>
  <rule event_type="gift" event_value="Rose" action="spawn" />
</rules>
""",
    )

    messages = engine.render_messages(gift_event().to_dict())

    assert len(messages) == 1
    assert messages[0]["message_type"] == "game_event"
    assert messages[0]["rule"]["event_type"] == "gift"
    assert messages[0]["rule"]["event_value"] == "Rose"
    assert messages[0]["rule"]["action"] == "spawn"
    assert messages[0]["payload"]["gift_name"] == "Rose"


def test_compact_comment_rule(tmp_path: Path) -> None:
    engine = compact_engine_from_xml(
        tmp_path,
        """
<rules>
  <rule event_type="comment" event_value="!attack" action="attack" />
</rules>
""",
    )

    messages = engine.render_messages(comment_event("!attack").to_dict())

    assert len(messages) == 1
    assert messages[0]["rule"]["action"] == "attack"
    assert messages[0]["payload"]["comment"] == "!attack"


def test_compact_wildcard_like_rule(tmp_path: Path) -> None:
    engine = compact_engine_from_xml(
        tmp_path,
        """
<rules>
  <rule event_type="like" event_value="*" action="like_bonus" />
</rules>
""",
    )

    messages = engine.render_messages(like_event().to_dict())

    assert len(messages) == 1
    assert messages[0]["rule"]["event_value"] == "*"
    assert messages[0]["rule"]["action"] == "like_bonus"
    assert messages[0]["payload"]["like_count"] == 7


def test_compact_wildcard_follow_rule(tmp_path: Path) -> None:
    engine = compact_engine_from_xml(
        tmp_path,
        """
<rules>
  <rule event_type="follow" event_value="*" action="follow_bonus" />
</rules>
""",
    )

    messages = engine.render_messages(follow_event().to_dict())

    assert len(messages) == 1
    assert messages[0]["rule"]["action"] == "follow_bonus"


def test_compact_rule_optional_custom_template(tmp_path: Path) -> None:
    engine = compact_engine_from_xml(
        tmp_path,
        """
<rules>
  <rule event_type="gift" event_value="Rose" action="spawn" template="custom_game_event" />
</rules>
""",
    )

    messages = engine.render_messages(gift_event().to_dict())

    assert len(messages) == 1
    assert messages[0] == {
        "message_type": "custom_game_event",
        "action": "spawn",
        "value": "Rose",
        "username": "viewer",
    }


def test_template_renderer_replaces_rule_placeholders() -> None:
    renderer = TemplateRenderer(
        {
            "rule_template": {
                "action": "{{rule.action}}",
                "event_type": "{{rule.event_type}}",
                "event_value": "{{rule.event_value}}",
                "combined": "{{rule.event_type}}:{{rule.event_value}}:{{rule.action}}",
            }
        }
    )

    rendered = renderer.render(
        "rule_template",
        event={},
        rule={"event_type": "gift", "event_value": "Rose", "action": "spawn"},
    )

    assert rendered == {
        "action": "spawn",
        "event_type": "gift",
        "event_value": "Rose",
        "combined": "gift:Rose:spawn",
    }
