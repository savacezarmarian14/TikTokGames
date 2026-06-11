# Normalized Event Format

TikTok Event Gateway broadcasts provider-neutral JSON events. Client
applications should depend on this schema instead of TikTokLive objects or any
other provider SDK shape.

## Envelope

Every event uses the same top-level envelope:

```json
{
  "version": "1.0",
  "event_id": "evt-comment-001",
  "event_type": "comment",
  "timestamp": "2026-01-02T03:04:05Z",
  "source": {
    "platform": "tiktok",
    "connector": "tiktok-live",
    "room_id": "room-123"
  },
  "user": {
    "user_id": "user-123",
    "username": "viewer_one",
    "display_name": "Viewer One"
  },
  "payload": {
    "comment": "hello stream"
  }
}
```

Fields:

- `version`: schema version.
- `event_id`: gateway-generated event identifier.
- `event_type`: one of `comment`, `join`, `gift`, `like`, `follow`, `connect`, `disconnect`, `error`, or `heartbeat`.
- `timestamp`: ISO 8601 UTC timestamp.
- `source`: provider and connector metadata.
- `user`: normalized user identity when the event has a user.
- `payload`: event-specific JSON object.
- `raw_event`: optional debug snapshot. Normal serialization omits it by default.

## Payloads

Comment:

```json
{ "comment": "hello stream" }
```

Join:

```json
{ "viewer_count": 128 }
```

Gift:

```json
{
  "gift_id": "5655",
  "gift_name": "Rose",
  "repeat_count": 3,
  "diamond_count": 1,
  "repeat_end": true
}
```

Like:

```json
{
  "like_count": 5,
  "total_like_count": 2048
}
```

Follow:

```json
{ "followed": true }
```

Connect:

```json
{
  "connection_id": "conn-001",
  "message": "connected to event source"
}
```

Disconnect:

```json
{
  "reason": "normal shutdown",
  "code": "shutdown"
}
```

Error:

```json
{
  "message": "connector disconnected unexpectedly",
  "code": "connector_disconnected",
  "recoverable": true
}
```

Heartbeat:

```json
{ "sequence": 42 }
```

## Normalizer Behavior

The TikTok normalizer accepts TikTok-like dictionaries or objects, but it does
not import TikTokLive. It reads stable field names, converts timestamps to UTC,
and builds Pydantic event models.

Bad input should not crash the gateway. If a raw event is unknown or missing
required fields, the normalizer returns an `error` event with
`code: "normalization_error"` and logs the failure.

Unknown provider fields are ignored. When debug raw events are enabled on the
normalizer, a JSON-safe snapshot is stored in `raw_event`; serialization helpers
still omit that field unless `include_raw=True` is requested.
