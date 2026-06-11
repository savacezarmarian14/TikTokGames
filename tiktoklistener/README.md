# TikTok Event Gateway

TikTok Event Gateway is a reusable Python middleware service that connects to a TikTok LIVE source, normalizes incoming events, and broadcasts stable JSON messages to one or more external applications over WebSockets.

The gateway is intentionally application-agnostic. It does not map gifts, comments, joins, or follows to game commands. Client applications consume the normalized JSON and decide what those events mean for their own domain.

## Goals

- Run on Python 3.11+ with `asyncio`.
- Keep TikTokLive-specific objects isolated inside the connector layer.
- Provide a stable Pydantic event schema for downstream applications.
- Support multiple event providers through an abstract connector interface.
- Include a mock connector for local development and automated tests.
- Broadcast normalized events to multiple WebSocket clients.
- Use internal `asyncio.Queue` instances between ingestion, normalization, and broadcasting.
- Load runtime settings from YAML.
- Emit structured logs suitable for production deployment.

## Architecture

```text
TikTok LIVE or mock source
        |
        v
Connector layer
        |
        | raw provider events, never exposed outside the layer
        v
Normalizer
        |
        | GatewayEvent Pydantic models
        v
Broker
        |
        | JSON payloads
        v
WebSocket transport
        |
        v
External applications
```

Runtime wiring is split deliberately:

- `app.py` builds configured components.
- `runtime.py` starts, stops, and connects those components.
- `broker/` isolates each subscriber behind its own queue so slow clients do
  not block the gateway.
- `transports/` contains protocol adapters and subscription parsing.

## Project Structure

```text
.
├── config/
│   └── example.yaml
├── demo_client/
│   ├── __init__.py
│   └── websocket_printer.py
├── docs/
│   ├── EVENT_FORMAT.md
│   ├── DEVELOPMENT.md
│   ├── ROADMAP.md
│   ├── TIKTOKLIVE_CONNECTOR.md
│   └── WEBSOCKET_PROTOCOL.md
├── examples/
│   ├── __init__.py
│   ├── clients/
│   │   ├── __init__.py
│   │   ├── README.md
│   │   ├── generic_game_adapter.py
│   │   ├── gift_subscriber.py
│   │   ├── terminal_dashboard.py
│   │   └── websocket_printer.py
│   └── events/
│       ├── comment.json
│       ├── connect.json
│       ├── disconnect.json
│       ├── error.json
│       ├── follow.json
│       ├── gift.json
│       ├── heartbeat.json
│       ├── join.json
│       └── like.json
├── src/
│   └── tiktok_event_gateway/
│       ├── __init__.py
│       ├── app.py
│       ├── cli.py
│       ├── runtime.py
│       ├── broker/
│       │   ├── __init__.py
│       │   └── event_broker.py
│       ├── config/
│       │   ├── __init__.py
│       │   ├── loader.py
│       │   └── settings.py
│       ├── connectors/
│       │   ├── __init__.py
│       │   ├── base.py
│       │   ├── mock.py
│       │   └── tiktok_live.py
│       ├── logging/
│       │   ├── __init__.py
│       │   └── setup.py
│       ├── models/
│       │   ├── __init__.py
│       │   ├── enums.py
│       │   └── events.py
│       ├── normalization/
│       │   ├── __init__.py
│       │   └── normalizer.py
│       └── transports/
│           ├── __init__.py
│           └── websocket_server.py
├── tests/
│   ├── integration/
│   │   ├── __init__.py
│   │   └── test_gateway_pipeline.py
│   └── unit/
│       ├── __init__.py
│       ├── test_connectors.py
│       ├── test_models.py
│       └── test_normalizer.py
└── pyproject.toml
```

## Event Flow

1. A connector receives provider-specific events from TikTokLive or another future source.
2. The connector adapts those events into internal raw event envelopes and pushes them onto an ingestion queue.
3. The normalizer reads from the ingestion queue and converts each raw envelope into a stable `GatewayEvent` model.
4. The broker receives normalized events and handles fan-out to subscribers.
5. The WebSocket server serializes each event as JSON and sends it to connected client applications.

## Connectors

Connectors are async raw-event producers. They expose `start()`, `stop()`,
`is_running()`, and an async event stream. The gateway can then pass that stream
through a provider-specific normalizer.

The mock connector emits deterministic TikTok-like dictionaries for local
development and tests. It supports comments, joins, gifts, likes, and follows,
with configurable interval, enabled event types, fake username, and fake room.

## WebSockets

External applications connect to the gateway WebSocket endpoint and receive
normalized event JSON strings. Clients receive all event types by default.

Clients can subscribe to specific event types by query string:

```text
ws://127.0.0.1:8765/events?event_types=gift,comment
```

Or by sending a JSON subscription message after connecting:

```json
{ "event_types": ["gift", "comment", "join"] }
```

Each client has an isolated internal queue. Slow clients may drop old queued
events without blocking the connector, normalizer, broker, or other clients.

## Event Schema

All normalized events share the same envelope:

- `version`: schema version string.
- `event_id`: gateway event identifier.
- `event_type`: provider-neutral event type such as `comment`, `gift`, or `heartbeat`.
- `timestamp`: UTC timestamp.
- `source`: platform and connector metadata.
- `user`: provider-neutral user identity when the event has a user.
- `payload`: event-specific JSON object.
- `raw_event`: optional connector debug data, omitted by default from normal serialization helpers.

Example wire payloads live in `examples/events/`.

The full normalized schema is documented in `docs/EVENT_FORMAT.md`.
The optional TikTokLive connector is documented in `docs/TIKTOKLIVE_CONNECTOR.md`.

## How To Use Pas Cu Pas

Acest ghid porneste de la zero si iti arata cum verifici gateway-ul local cu
mock events, apoi cum il conectezi la un TikTok LIVE real.

### 1. Instaleaza proiectul

Ai nevoie de Python 3.11+.

Pe Linux/macOS:

```bash
cd /root/projects/TikTokGames/tiktoklistener
python -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e ".[dev]"
```

Pe Windows, in PowerShell:

```powershell
cd C:\path\catre\tiktoklistener
py -3.11 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -e ".[dev]"
```

Pe Windows, daca PowerShell blocheaza activarea venv-ului, ruleaza o singura data:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

Inchide si redeschide PowerShell, apoi ruleaza din nou:

```powershell
.\.venv\Scripts\Activate.ps1
```

Daca vrei sa conectezi un TikTok LIVE real, instaleaza si extra-ul optional:

```bash
python -m pip install -e ".[tiktok]"
```

Mock mode nu are nevoie de TikTokLive instalat.

Pe Windows, comenzile de mai jos sunt aceleasi dupa ce ai activat `.venv`.
Daca `tiktok-event-gateway` nu este gasit, foloseste varianta cu modul Python:

```powershell
python -m tiktok_event_gateway.cli validate-config --config config/mock.yaml
python -m tiktok_event_gateway.cli run --config config/mock.yaml
```

### 2. Verifica daca un config este valid

```bash
tiktok-event-gateway validate-config --config config/mock.yaml
```

Daca totul e bine, ar trebui sa vezi un mesaj de tip:

```text
Config is valid: config/mock.yaml
Provider: mock
WebSocket: ws://127.0.0.1:8765/events
```

### 3. Porneste gateway-ul in mock mode

Mock mode genereaza evenimente false: comments, joins, gifts, likes, follows.
Este cel mai bun mod de testare fara TikTok.

Terminal 1:

```bash
tiktok-event-gateway run --config config/mock.yaml
```

Gateway-ul porneste WebSocket server-ul pe:

```text
ws://127.0.0.1:8765/events
```

Pe Windows poti rula aceeasi comanda in PowerShell:

```powershell
tiktok-event-gateway run --config config/mock.yaml
```

Sau, daca entrypoint-ul nu este gasit:

```powershell
python -m tiktok_event_gateway.cli run --config config/mock.yaml
```

### 4. Conecteaza un client care printeaza toate evenimentele

Terminal 2:

```bash
python -m examples.clients.websocket_printer --pretty
```

Ar trebui sa vezi JSON-uri normalizate, de forma:

```json
{
  "version": "1.0",
  "event_id": "...",
  "event_type": "comment",
  "timestamp": "...",
  "source": {
    "platform": "tiktok",
    "connector": "mock"
  },
  "user": {
    "user_id": "mock-user-001-001",
    "username": "mock_viewer"
  },
  "payload": {
    "comment": "Mock comment #1"
  }
}
```

### 5. Testeaza subscription/filtering

Client doar pentru gifts:

```bash
python -m examples.clients.gift_subscriber
```

Client generic de tip game adapter, in afara gateway core:

```bash
python -m examples.clients.generic_game_adapter
```

Dashboard in terminal cu numaratoare pe tipuri de evenimente:

```bash
python -m examples.clients.terminal_dashboard
```

### 6. Porneste pe un TikTok LIVE real

Varianta cea mai simpla este scriptul one-command. Ii dai link-ul de live,
profilul sau username-ul, iar scriptul porneste gateway-ul cu toate event types
active si logging DEBUG in JSONL.

```bash
python scripts/listen_live.py "https://www.tiktok.com/@nume_live/live"
```

Pe Windows PowerShell:

```powershell
python .\scripts\listen_live.py "https://www.tiktok.com/@nume_live/live"
```

Merge si cu username direct:

```bash
python scripts/listen_live.py "@nume_live"
python scripts/listen_live.py "nume_live"
```

Pe Windows:

```powershell
python .\scripts\listen_live.py "@nume_live"
python .\scripts\listen_live.py "nume_live"
```

Scriptul foloseste implicit `config/tiktoklive-full-logs.yaml`, activeaza raw
debug events si salveaza evenimentele in:

```text
logs/nume_live-events.jsonl
```

In alt terminal, vezi toate evenimentele:

```bash
python -m examples.clients.websocket_printer --pretty
```

Sau doar gifts:

```bash
python -m examples.clients.gift_subscriber
```

Daca portul default este ocupat:

```bash
python scripts/listen_live.py "@nume_live" --port 8766
python -m examples.clients.websocket_printer --url ws://127.0.0.1:8766/events --pretty
```

Pe Windows:

```powershell
python .\scripts\listen_live.py "@nume_live" --port 8766
python -m examples.clients.websocket_printer --url ws://127.0.0.1:8766/events --pretty
```

Pentru log intr-un fisier ales de tine:

```bash
python scripts/listen_live.py "@nume_live" --event-log logs/test-alt-live.jsonl
```

Varianta manuala ramane disponibila daca vrei sa editezi config-ul:

Editeaza `config/tiktoklive.yaml` si schimba:

```yaml
connector:
  provider: "tiktok"
  tiktok:
    live_username: "example_user"
```

Inlocuieste `example_user` cu username-ul live-ului pe care vrei sa il testezi,
de exemplu:

```yaml
live_username: "nume_live_real"
```

Apoi ruleaza:

```bash
tiktok-event-gateway validate-config --config config/tiktoklive.yaml
tiktok-event-gateway run --config config/tiktoklive.yaml
```

In alt terminal, conecteaza un client:

```bash
python -m examples.clients.websocket_printer --pretty
```

Pentru gifts only:

```bash
python -m examples.clients.gift_subscriber
```

Important: TikTokLive este o librarie neoficiala. Daca TikTok schimba ceva in
API-ul intern, connector-ul poate avea nevoie de ajustari, dar schema JSON a
gateway-ului trebuie sa ramana stabila.

### 7. Logare evenimente in JSONL

In `config/tiktoklive.yaml` sau `config/mock.yaml`, activeaza:

```yaml
logging:
  event_log:
    enabled: true
    path: "logs/events.jsonl"
    include_raw_event: false
```

Apoi porneste gateway-ul:

```bash
tiktok-event-gateway run --config config/mock.yaml
```

Evenimentele normalizate vor fi salvate in `logs/events.jsonl`.

### 8. Replay la evenimente salvate

Dupa ce ai un fisier JSONL cu evenimente:

```bash
tiktok-event-gateway replay-events logs/events.jsonl --config config/mock.yaml --speed 1.0
```

Pentru replay mai rapid:

```bash
tiktok-event-gateway replay-events logs/events.jsonl --config config/mock.yaml --speed 3.0
```

Clientii WebSocket se conecteaza la fel ca in live mode:

```bash
python -m examples.clients.websocket_printer --pretty
```

### 9. Troubleshooting rapid

- Daca `validate-config` pica, verifica indentarea YAML si campurile obligatorii.
- Daca TikTokLive nu porneste, verifica daca ai instalat `python -m pip install -e ".[tiktok]"`.
- Daca nu vezi evenimente reale, verifica daca userul TikTok este live in momentul testului.
- Daca portul `8765` este ocupat, schimba `websocket.port` in config.
- Pe Windows, daca `.venv\Scripts\Activate.ps1` este blocat, ruleaza `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`.
- Pe Windows, daca `py -3.11` nu merge, instaleaza Python 3.11+ de pe python.org si bifeaza `Add python.exe to PATH`.
- Pe Windows, daca `tiktok-event-gateway` nu este gasit, ruleaza comenzile cu `python -m tiktok_event_gateway.cli`.
- Mock mode trebuie sa functioneze mereu fara TikTokLive.

## Development

Install the gateway in a virtual environment:

```bash
python -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install -e ".[dev]"
```

Install TikTokLive support only when needed:

```bash
python -m pip install -e ".[dev,tiktok]"
```

Validate a config file:

```bash
tiktok-event-gateway validate-config --config config/mock.yaml
```

Run the gateway with the mock connector and WebSocket server:

```bash
tiktok-event-gateway run --config config/mock.yaml
```

Run with TikTokLive:

```bash
tiktok-event-gateway run --config config/tiktoklive.yaml
```

Connect a client:

```bash
python -m examples.clients.websocket_printer --pretty
```

Replay a normalized JSONL event log through WebSockets:

```bash
tiktok-event-gateway replay-events logs/events.jsonl --config config/mock.yaml --speed 2.0
```

Generate an example config:

```bash
tiktok-event-gateway generate-example-config --mode mock --output config/generated-mock.yaml
```

Run the demo client:

```bash
python -m examples.clients.websocket_printer
```

Run the demo client with a subscription filter:

```bash
python -m examples.clients.websocket_printer --event-types gift,comment
```

Run the gift-only example:

```bash
python -m examples.clients.gift_subscriber
```

Run the generic external game adapter:

```bash
python -m examples.clients.generic_game_adapter
```

Run the terminal dashboard:

```bash
python -m examples.clients.terminal_dashboard
```

External app connection details are documented in `docs/WEBSOCKET_PROTOCOL.md`.
More example client notes live in `examples/clients/README.md`.

Run tests:

```bash
pytest
```

Example configuration files live in `config/`:

- `config/mock.yaml`
- `config/tiktoklive.yaml`
- `config/websocket.yaml`
- `config/logging.yaml`

Contributor workflow details are in `docs/DEVELOPMENT.md`.

## Design Boundaries

- The gateway core depends on connector abstractions, not TikTokLive directly.
- TikTokLive raw event objects must not leave `connectors/tiktok_live.py`.
- Normalized events are stable Pydantic models.
- Client applications own all domain-specific behavior.
- New platforms should be added by implementing the connector interface and a corresponding normalization adapter.


## RuleBasedGameBridge

The listener can optionally convert normalized TikTok events into game-specific JSON messages before sending them to a local game application. Each game provides its own `rules.xml` and `templates.json`; paths are supplied through YAML config or CLI arguments, not hardcoded in Python.

Local UDP example:

```bash
python scripts/listen_live.py https://www.tiktok.com/@someuser/live \
  --game-bridge-enabled \
  --game-rules-path ./games/towerwar/rules.xml \
  --game-templates-path ./games/towerwar/templates.json \
  --game-output-type udp \
  --game-output-host 127.0.0.1 \
  --game-output-port 7000
```

See `docs/GAME_BRIDGE.md` for the XML rule format, JSON template format, and local stdout/UDP output modes.
