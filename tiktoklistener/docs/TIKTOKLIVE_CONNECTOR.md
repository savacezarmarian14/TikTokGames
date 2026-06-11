# TikTokLive Connector

The TikTokLive connector integrates the gateway with the optional
`TikTokLive` Python library.

`TikTokLive` is an unofficial third-party library. TikTok may change internal
LIVE behavior or web APIs without notice, and the library may change its event
object shapes between releases. For that reason, all imports and SDK-specific
handling are isolated in `src/tiktok_event_gateway/connectors/tiktok_live.py`.

The rest of the gateway must continue to depend only on:

- raw event dictionaries emitted by connectors
- the normalizer layer
- stable Pydantic gateway event models
- normalized JSON sent over WebSockets

## Configuration

```yaml
connector:
  provider: "tiktok"
  tiktok:
    live_username: "example_user"
    reconnect_enabled: true
    reconnect_delay_seconds: 5.0
    include_raw_event_debug: false
    enabled_event_types:
      - "comment"
      - "join"
      - "gift"
      - "like"
      - "follow"
      - "connect"
      - "disconnect"
      - "error"
```

## Supported Events

The connector adapts TikTokLive events into provider-like raw dictionaries for:

- `comment`
- `join`
- `gift`
- `like`
- `follow`
- `connect`
- `disconnect`
- `error`

Those dictionaries are then normalized by `TikTokEventNormalizer`. TikTokLive
objects must not leave the connector module.

## Running

Install the optional dependency:

```bash
python -m pip install -e ".[tiktok]"
```

Run the connector:

```bash
tiktok-event-gateway --connector tiktok --tiktok-live-username example_user
```

The mock connector remains the default and does not require TikTokLive:

```bash
tiktok-event-gateway --connector mock --limit 5
```

## Maintenance Boundary

If TikTokLive changes event class names or object fields, update only the
connector adapter where possible. Do not change the normalized event schema to
mirror TikTokLive internals.
