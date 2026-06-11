"""Load and validate game bridge XML rules."""

from __future__ import annotations

from pathlib import Path
import re
import xml.etree.ElementTree as ET

from tiktok_event_gateway.bridge.models import GameRule, RuleCondition, RuleMessage, RuleParam

CONDITION_OPERATORS = (
    "equals",
    "not_equals",
    "contains",
    "starts_with",
    "greater_than",
    "less_than",
    "exists",
)

_COMPACT_REQUIRED_ATTRS = {"event_type", "event_value", "action"}


class RuleLoadError(ValueError):
    """Raised when XML rules cannot be loaded or validated."""


def load_rules(path: str | Path) -> list[GameRule]:
    """Load game bridge rules from XML.

    Both formats are supported:

    Verbose:
        <rule id="r1" event_type="gift">
            <condition field="payload.gift_name" equals="Rose" />
            <message template="game_action">
                <param name="action" value="spawn" />
            </message>
        </rule>

    Compact:
        <rule event_type="gift" event_value="Rose" action="spawn" />
        <rule event_type="like" event_value="*" action="like_bonus" />
    """
    rules_path = Path(path)
    if not rules_path.exists():
        msg = f"game bridge rules file does not exist: {rules_path}"
        raise RuleLoadError(msg)

    try:
        root = ET.fromstring(rules_path.read_text(encoding="utf-8"))
    except ET.ParseError as exc:
        msg = f"invalid game bridge XML rules in {rules_path}: {exc}"
        raise RuleLoadError(msg) from exc

    if root.tag != "rules":
        msg = "game bridge rules XML root must be <rules>"
        raise RuleLoadError(msg)

    rules: list[GameRule] = []
    seen_ids: set[str] = set()
    for index, rule_el in enumerate(root.findall("rule"), start=1):
        rule = _parse_rule(rule_el, index)
        if rule.rule_id in seen_ids:
            msg = f"duplicate game bridge rule id: {rule.rule_id}"
            raise RuleLoadError(msg)
        seen_ids.add(rule.rule_id)
        rules.append(rule)
    return rules


def _parse_rule(rule_el: ET.Element, index: int) -> GameRule:
    if _is_compact_rule(rule_el):
        return _parse_compact_rule(rule_el, index)
    return _parse_verbose_rule(rule_el)


def _is_compact_rule(rule_el: ET.Element) -> bool:
    """Return True when a <rule> uses the compact attribute-only format."""
    return _COMPACT_REQUIRED_ATTRS.issubset(rule_el.attrib)


def _parse_compact_rule(rule_el: ET.Element, index: int) -> GameRule:
    event_type = _required_attr(rule_el, "event_type", "compact rule")
    event_value = _required_attr(rule_el, "event_value", "compact rule")
    action = _required_attr(rule_el, "action", "compact rule")
    template = rule_el.attrib.get("template", "game_event").strip() or "game_event"
    rule_id = rule_el.attrib.get("id") or _compact_rule_id(index, event_type, event_value, action)
    rule_id = rule_id.strip()
    if not rule_id:
        msg = "compact rule id cannot be empty"
        raise RuleLoadError(msg)
    if list(rule_el):
        msg = f"compact rule {rule_id!r} must not contain child elements"
        raise RuleLoadError(msg)
    return GameRule(
        rule_id=rule_id,
        event_type=event_type,
        event_value=event_value,
        action=action,
        compact=True,
        messages=(RuleMessage(template=template),),
    )


def _parse_verbose_rule(rule_el: ET.Element) -> GameRule:
    rule_id = _required_attr(rule_el, "id", "rule")
    event_type = _required_attr(rule_el, "event_type", f"rule {rule_id}")

    conditions = tuple(_parse_condition(el, rule_id) for el in rule_el.findall("condition"))
    messages = tuple(_parse_message(el, rule_id) for el in rule_el.findall("message"))
    if not messages:
        msg = f"game bridge rule {rule_id!r} must contain at least one <message>"
        raise RuleLoadError(msg)
    return GameRule(
        rule_id=rule_id,
        event_type=event_type,
        conditions=conditions,
        messages=messages,
    )


def _parse_condition(condition_el: ET.Element, rule_id: str) -> RuleCondition:
    field = _required_attr(condition_el, "field", f"condition in rule {rule_id}")
    operators = [name for name in CONDITION_OPERATORS if name in condition_el.attrib]
    if len(operators) != 1:
        msg = (
            f"condition for field {field!r} in rule {rule_id!r} must define exactly one "
            f"operator from: {', '.join(CONDITION_OPERATORS)}"
        )
        raise RuleLoadError(msg)
    operator = operators[0]
    expected = condition_el.attrib.get(operator)
    return RuleCondition(
        field=field,
        operator=operator,
        expected=expected,
        ignore_case=_xml_bool(condition_el.attrib.get("ignore_case", "false")),
        value_type=condition_el.attrib.get("type", "str"),
    )


def _parse_message(message_el: ET.Element, rule_id: str) -> RuleMessage:
    template = _required_attr(message_el, "template", f"message in rule {rule_id}")
    params: list[RuleParam] = []
    seen_names: set[str] = set()
    for param_el in message_el.findall("param"):
        name = _required_attr(param_el, "name", f"param in rule {rule_id}")
        if name in seen_names:
            msg = f"duplicate param {name!r} in rule {rule_id!r} message {template!r}"
            raise RuleLoadError(msg)
        seen_names.add(name)
        has_value = "value" in param_el.attrib
        has_from = "from" in param_el.attrib
        if has_value and has_from:
            msg = f"param {name!r} in rule {rule_id!r} cannot define both value and from"
            raise RuleLoadError(msg)
        if not has_value and not has_from and "default" not in param_el.attrib:
            msg = f"param {name!r} in rule {rule_id!r} must define value, from, or default"
            raise RuleLoadError(msg)
        params.append(
            RuleParam(
                name=name,
                value=param_el.attrib.get("value"),
                from_path=param_el.attrib.get("from"),
                default=param_el.attrib.get("default"),
                value_type=param_el.attrib.get("type", "str"),
            )
        )
    return RuleMessage(template=template, params=tuple(params))


def _required_attr(element: ET.Element, attr: str, context: str) -> str:
    value = element.attrib.get(attr)
    if value is None or value.strip() == "":
        msg = f"missing required attribute {attr!r} on {context}"
        raise RuleLoadError(msg)
    return value.strip()


def _xml_bool(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "y", "on"}


def _compact_rule_id(index: int, event_type: str, event_value: str, action: str) -> str:
    raw = f"compact_{index}_{event_type}_{event_value}_{action}"
    safe = re.sub(r"[^a-zA-Z0-9_]+", "_", raw).strip("_")
    return safe or f"compact_{index}"
