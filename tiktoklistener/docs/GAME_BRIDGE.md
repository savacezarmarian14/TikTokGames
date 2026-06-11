# RuleBasedGameBridge

`RuleBasedGameBridge` is an optional component inside the TikTok Event Gateway listener.
It receives the same normalized events that the generic WebSocket broker receives, matches them against a game-provided XML rules file, renders game-specific JSON messages from a game-provided JSON template file, and sends those generated messages to a local game application.

The bridge is disabled by default.

## Pipeline

```text
TikTok LIVE
  -> connector
  -> normalizer
  -> normalized gateway event
     -> existing generic WebSocket broker
     -> optional RuleBasedGameBridge
          -> rules.xml
          -> templates.json
          -> generated game JSON
          -> stdout or UDP localhost
          -> local game app
```

The bridge does not import TikTokLive and does not contain hardcoded game logic. Each game supplies its own `rules.xml` and `templates.json`.

## Configuration

```yaml
game_bridge:
  enabled: true
  rules_path: "examples/game_bridge/rules.xml"
  templates_path: "examples/game_bridge/templates.json"
  log_unmatched_events: false
  deduplication_enabled: true
  deduplication_max_size: 10000
  output:
    type: "udp"
    host: "127.0.0.1"
    port: 7000
    url: null
```

`rules_path` and `templates_path` are required only when `game_bridge.enabled` is `true`.

## CLI usage

Stdout output:

```bash
python scripts/listen_live.py https://www.tiktok.com/@someuser/live \
  --game-bridge-enabled \
  --game-rules-path ./games/towerwar/rules.xml \
  --game-templates-path ./games/towerwar/templates.json \
  --game-output-type stdout
```

UDP output to a local game application:

```bash
python scripts/listen_live.py https://www.tiktok.com/@someuser/live \
  --game-bridge-enabled \
  --game-rules-path ./games/towerwar/rules.xml \
  --game-templates-path ./games/towerwar/templates.json \
  --game-output-type udp \
  --game-output-host 127.0.0.1 \
  --game-output-port 7000
```

Your game app should listen on UDP `127.0.0.1:7000` and decode one JSON object per packet.


## Compact XML rules

The bridge also supports a simplified compact XML format. This is the recommended format when the XML should only decide the action and the JSON template should carry the real event data.

```xml
<rules>
    <rule event_type="gift" event_value="Rose" action="spawn" />
    <rule event_type="gift" event_value="Football" action="heal" />
    <rule event_type="comment" event_value="!attack" action="attack" />
    <rule event_type="like" event_value="*" action="like_bonus" />
    <rule event_type="follow" event_value="*" action="follow_bonus" />
</rules>
```

Compact rule attributes:

- `event_type`: the normalized event type, such as `gift`, `comment`, `like`, `follow`, or `join`
- `event_value`: the value to match
- `action`: the action name that will be exposed to the JSON template as `{{rule.action}}`
- `template`: optional template name, defaults to `game_event`

For compact rules, the bridge extracts `event_value` automatically:

- `gift` -> `event.payload.gift_name`
- `comment` -> `event.payload.comment`
- `like`, `follow`, `join` -> `*`

Compact templates can use both rule placeholders and event placeholders:

```json
{
  "templates": {
    "game_event": {
      "message_type": "game_event",
      "rule": {
        "event_type": "{{rule.event_type}}",
        "event_value": "{{rule.event_value}}",
        "action": "{{rule.action}}"
      },
      "event": {
        "id": "{{event.event_id}}",
        "type": "{{event.event_type}}",
        "timestamp": "{{event.timestamp}}"
      },
      "user": {
        "id": "{{event.user.user_id}}",
        "username": "{{event.user.username}}",
        "display_name": "{{event.user.display_name}}"
      },
      "payload": {
        "gift_name": "{{event.payload.gift_name}}",
        "comment": "{{event.payload.comment}}",
        "like_count": "{{event.payload.like_count}}",
        "repeat_count": "{{event.payload.repeat_count}}"
      }
    }
  }
}
```

Example files are available at:

```text
examples/game_bridge/rules_compact.xml
examples/game_bridge/templates_compact.json
```

## XML rules

Example:

```xml
<rules>
    <rule id="rose_spawn_blue" event_type="gift">
        <condition field="payload.gift_name" equals="Rose" />
        <message template="game_action">
            <param name="action" value="spawn" />
            <param name="team" value="red" />
            <param name="target" value="blue" />
            <param name="amount" from="payload.repeat_count" type="int" default="1" />
        </message>
    </rule>

    <rule id="comment_attack" event_type="comment">
        <condition field="payload.comment" equals="!attack" ignore_case="true" />
        <message template="game_action">
            <param name="action" value="spawn" />
            <param name="team" value="red" />
            <param name="target" value="blue" />
            <param name="amount" value="1" type="int" />
        </message>
    </rule>
</rules>
```

Supported condition operators:

- `equals`
- `not_equals`
- `contains`
- `starts_with`
- `greater_than`
- `less_than`
- `exists`

Supported condition attributes:

- `field`
- one operator attribute
- `ignore_case`
- `type`

Supported param attributes:

- `name`
- `value`
- `from`
- `default`
- `type`

Supported param types:

- `str`
- `int`
- `float`
- `bool`

## JSON templates

Example:

```json
{
  "templates": {
    "game_action": {
      "message_type": "game_action",
      "action": "{{params.action}}",
      "team": "{{params.team}}",
      "target": "{{params.target}}",
      "amount": "{{params.amount}}",
      "metadata": {
        "source": "tiktok",
        "event_id": "{{event.event_id}}",
        "event_type": "{{event.event_type}}",
        "username": "{{event.user.username}}",
        "display_name": "{{event.user.display_name}}",
        "timestamp": "{{event.timestamp}}"
      }
    }
  }
}
```

If a placeholder is the whole string, the renderer preserves the original JSON type. For example, an integer parameter remains an integer in the output JSON.

## Adding a new game

To add a new game, do not change Python code. Add a game directory like:

```text
games/my_game/
  rules.xml
  templates.json
```

Then start the listener with those paths through config or CLI.
