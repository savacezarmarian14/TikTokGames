# WebSocket Protocol

External applications connect to TikTok Event Gateway over WebSockets and
receive normalized JSON strings. The gateway does not send application-specific
commands; each client decides how to interpret events.

## Connection URL

Default local URL:

```text
ws://127.0.0.1:8765/events
```

Event filters can be supplied in the query string:

```text
ws://127.0.0.1:8765/events?event_types=gift,comment,join
```

If no filter is provided, the client receives all normalized event types.

## Subscription Message

Clients can update their subscription after connecting by sending a JSON
message:

```json
{
  "event_types": ["gift", "comment", "join"]
}
```

To receive all events again:

```json
{
  "all": true
}
```

Supported event types:

- `comment`
- `join`
- `gift`
- `like`
- `follow`
- `connect`
- `disconnect`
- `error`
- `heartbeat`

## Event JSON Format

Every event is sent as one JSON string with this envelope:

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

`user` is present when the event has a user. `payload` is event-specific.

## Heartbeat Format

When a client is idle, the server sends heartbeat events using the same event
envelope:

```json
{
  "version": "1.0",
  "event_id": "4bbecfd0-6f91-4fd0-8f9a-f84b240fb1a4",
  "event_type": "heartbeat",
  "timestamp": "2026-01-02T03:04:13Z",
  "source": {
    "platform": "gateway",
    "connector": "websocket-server"
  },
  "payload": {
    "sequence": 42
  }
}
```

## Error Event Format

Gateway and connector errors are sent as normalized `error` events:

```json
{
  "version": "1.0",
  "event_id": "evt-error-001",
  "event_type": "error",
  "timestamp": "2026-01-02T03:04:12Z",
  "source": {
    "platform": "gateway",
    "connector": "runtime"
  },
  "payload": {
    "message": "connector disconnected unexpectedly",
    "code": "connector_disconnected",
    "recoverable": true
  }
}
```

Protocol or subscription errors may also close the WebSocket connection with a
WebSocket close reason, for example `invalid event subscription`.
