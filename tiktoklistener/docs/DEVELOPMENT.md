# Development Guide

This project is structured so the gateway core stays reusable and
application-agnostic. Connector code produces raw provider events, normalizers
convert those into stable Pydantic models, the broker fans out normalized
events, and transports deliver JSON to external applications.

## Module Responsibilities

- `app.py`: composition root that builds connectors, normalizers, brokers,
  transports, and optional event log writers from validated settings.
- `runtime.py`: lifecycle orchestration for live mode and replay mode.
- `connectors/`: raw event producers. Provider SDKs must stay inside their
  connector module.
- `normalization/`: conversion from raw provider-like events to stable gateway
  event models.
- `broker/`: fan-out to independent subscriber queues.
- `transports/`: protocol adapters such as WebSocket. Subscription parsing is
  intentionally separate from connection/session handling.
- `models/`: stable Pydantic schemas and JSON serialization helpers.

## Setup

Create a virtual environment and install development dependencies:

```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e ".[dev]"
```

Install the optional TikTokLive dependency only when working on the TikTokLive
connector:

```bash
python -m pip install -e ".[dev,tiktok]"
```

Mock mode must work without TikTokLive installed.

## Common Commands

Validate configuration:

```bash
tiktok-event-gateway validate-config --config config/mock.yaml
```

Run mock mode:

```bash
tiktok-event-gateway run --config config/mock.yaml
```

Run tests:

```bash
pytest
```

Run linting and formatting checks:

```bash
ruff check src tests examples demo_client
black --check src tests examples demo_client
mypy
```

Format code:

```bash
black src tests examples demo_client
ruff check --fix src tests examples demo_client
```

## Testing Strategy

- Unit tests cover event models, normalizers, connectors, configuration, event
  logging, subscription parsing, and replay parsing.
- Integration tests cover MockConnector -> Normalizer -> Broker -> WebSocket,
  subscriptions, heartbeat behavior, malformed events, and replay delivery.
- TikTokLive tests must use fake clients/events and must not require a real
  TikTok LIVE.

## Boundaries

- Do not import TikTokLive outside `connectors/tiktok_live.py`.
- Do not add game-specific mappings inside `src/tiktok_event_gateway`.
- Put app-specific behavior under `examples/clients`, `examples/apps`, or a
  separate downstream project.
- Keep normalized event schemas stable. Provider SDK changes should be handled
  in connector/normalizer adapters.
- Keep blocking I/O out of async flows. Use `asyncio.to_thread()` for file or
  SDK calls that may block.

## CI

GitHub Actions runs:

- install dependencies
- Black formatting check
- Ruff lint
- Mypy type check
- Pytest
