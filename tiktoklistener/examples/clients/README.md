# Example WebSocket Clients

These are small example applications that consume TikTok Event Gateway events.
They are intentionally outside `src/tiktok_event_gateway` so the gateway remains
application-agnostic.

Start the gateway with the mock connector:

```bash
tiktok-event-gateway run --config config/mock.yaml
```

Print all events:

```bash
python -m examples.clients.websocket_printer --pretty
```

Subscribe only to gift events:

```bash
python -m examples.clients.gift_subscriber
```

Run a generic game adapter:

```bash
python -m examples.clients.generic_game_adapter
```

The adapter maps `payload.gift_name` values to its own generic actions. This is
the correct place for app-specific behavior: outside the gateway core.

Run the terminal dashboard:

```bash
python -m examples.clients.terminal_dashboard
```

All examples accept `--url ws://host:port/path` when the gateway is not running
on the default `ws://127.0.0.1:8765/events`.
