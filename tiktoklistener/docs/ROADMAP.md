# Development Roadmap

## Phase 1: Core Contracts

- Define connector interfaces and lifecycle behavior.
- Finalize Pydantic event schemas and versioning strategy.
- Implement YAML configuration loading and validation.
- Establish structured logging setup.

## Phase 2: Local Pipeline

- Implement the mock connector.
- Implement raw-to-normalized event conversion.
- Implement queue-based runtime orchestration.
- Implement broker fan-out semantics.
- Add focused unit tests for models, config, and normalization.

## Phase 3: WebSocket Transport

- Implement the WebSocket server.
- Support multiple connected applications.
- Add connection lifecycle logging.
- Add integration tests for mock events reaching WebSocket clients.
- Build the demo client that prints received JSON events.

## Phase 4: TikTokLive Connector

- Implement TikTokLive integration inside the connector layer only.
- Convert TikTokLive event objects into internal raw envelopes.
- Add graceful reconnect and shutdown behavior.
- Document TikTokLive setup, credentials, and operational caveats.

## Phase 5: Production Hardening

- Add health/readiness endpoints or process probes if needed.
- Add metrics hooks.
- Add backpressure and overflow policies.
- Add Docker packaging.
- Add CI for linting, typing, and tests.
- Write deployment examples.
