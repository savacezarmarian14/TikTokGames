"""WebSocket broadcast server.

This transport will accept client application connections and send normalized
gateway events as JSON. It should not know about TikTokLive or application
domain behavior.
"""

from __future__ import annotations

import asyncio
import json
from dataclasses import dataclass
from urllib.parse import urlparse
from uuid import uuid4

import structlog
from websockets.exceptions import ConnectionClosed
from websockets.server import WebSocketServer, WebSocketServerProtocol, serve

from tiktok_event_gateway.broker import BrokerSubscription, EventBroker
from tiktok_event_gateway.models import (
    GatewayEvent,
    HeartbeatEvent,
    HeartbeatPayload,
    Source,
    SourcePlatform,
)
from tiktok_event_gateway.transports.subscriptions import (
    event_type_values,
    event_types_from_message,
    event_types_from_path,
)

logger = structlog.get_logger(__name__)


@dataclass
class WebSocketClientSession:
    """State for one connected WebSocket client."""

    client_id: str
    websocket: WebSocketServerProtocol
    subscription: BrokerSubscription
    heartbeat_sequence: int = 0


class WebSocketEventServer:
    """WebSocket transport for normalized gateway events."""

    def __init__(
        self,
        *,
        broker: EventBroker,
        host: str = "127.0.0.1",
        port: int = 8765,
        path: str = "/events",
        heartbeat_interval_seconds: float = 15.0,
        send_timeout_seconds: float = 5.0,
    ) -> None:
        self.broker = broker
        self.host = host
        self.port = port
        self.path = path
        self.heartbeat_interval_seconds = heartbeat_interval_seconds
        self.send_timeout_seconds = send_timeout_seconds
        self._server: WebSocketServer | None = None

    async def start(self) -> None:
        """Start accepting WebSocket clients."""
        if self._server is not None:
            return
        self._server = await serve(self._handle_client, self.host, self.port)
        logger.info(
            "websocket server started",
            host=self.host,
            port=self.port,
            path=self.path,
        )

    async def stop(self) -> None:
        """Stop accepting clients and close the WebSocket server."""
        server = self._server
        if server is None:
            return
        server.close()
        await server.wait_closed()
        self._server = None
        logger.info("websocket server stopped")

    @property
    def url(self) -> str:
        """Return the WebSocket URL for this server."""
        return f"ws://{self.host}:{self.bound_port}{self.path}"

    @property
    def bound_port(self) -> int:
        """Return the actual bound port, including when configured as zero."""
        if self._server is None or not self._server.sockets:
            return self.port
        socket_name = self._server.sockets[0].getsockname()
        return int(socket_name[1])

    async def _handle_client(
        self,
        websocket: WebSocketServerProtocol,
        path: str | None = None,
    ) -> None:
        request_path = path or getattr(websocket, "path", self.path)
        if urlparse(request_path).path != self.path:
            await websocket.close(code=1008, reason="unsupported websocket path")
            return

        client_id = str(uuid4())
        try:
            event_types = event_types_from_path(request_path)
        except ValueError as exc:
            logger.warning(
                "invalid subscription query",
                client_id=client_id,
                path=request_path,
                error=str(exc),
            )
            await websocket.close(code=1008, reason="invalid event subscription")
            return

        subscription = self.broker.subscribe(
            event_types=event_types,
            subscription_id=client_id,
        )
        session = WebSocketClientSession(
            client_id=client_id,
            websocket=websocket,
            subscription=subscription,
        )

        logger.info(
            "client connected",
            client_id=client_id,
            event_types=event_type_values(subscription.event_types),
        )

        reader = asyncio.create_task(self._read_client_messages(session))
        writer = asyncio.create_task(self._write_client_events(session))
        try:
            done, pending = await asyncio.wait(
                {reader, writer},
                return_when=asyncio.FIRST_COMPLETED,
            )
            for task in done:
                task.result()
            for task in pending:
                task.cancel()
                try:
                    await task
                except asyncio.CancelledError:
                    pass
        except ConnectionClosed:
            pass
        finally:
            self.broker.unsubscribe(subscription.subscription_id)
            logger.info(
                "client disconnected",
                client_id=client_id,
                dropped_events=subscription.dropped_events,
            )

    async def _read_client_messages(self, session: WebSocketClientSession) -> None:
        async for message in session.websocket:
            try:
                event_types = event_types_from_message(message)
            except (json.JSONDecodeError, UnicodeDecodeError, ValueError) as exc:
                logger.warning(
                    "invalid subscription message",
                    client_id=session.client_id,
                    error=str(exc),
                )
                continue
            self.broker.update_subscription(session.subscription.subscription_id, event_types)
            logger.info(
                "client subscription updated",
                client_id=session.client_id,
                event_types=event_type_values(event_types),
            )

    async def _write_client_events(self, session: WebSocketClientSession) -> None:
        while True:
            try:
                event = await asyncio.wait_for(
                    session.subscription.queue.get(),
                    timeout=self.heartbeat_interval_seconds,
                )
            except asyncio.TimeoutError:
                session.heartbeat_sequence += 1
                event = HeartbeatEvent(
                    source=Source(platform=SourcePlatform.GATEWAY, connector="websocket-server"),
                    payload=HeartbeatPayload(sequence=session.heartbeat_sequence),
                )

            if not await self._send_event(session, event):
                break

    async def _send_event(self, session: WebSocketClientSession, event: GatewayEvent) -> bool:
        payload = event.to_json()
        try:
            await asyncio.wait_for(
                session.websocket.send(payload),
                timeout=self.send_timeout_seconds,
            )
        except (asyncio.TimeoutError, ConnectionClosed, OSError) as exc:
            logger.warning(
                "send failure",
                client_id=session.client_id,
                event_id=event.event_id,
                event_type=event.event_type.value,
                error=str(exc),
            )
            return False

        logger.info(
            "event sent",
            client_id=session.client_id,
            event_id=event.event_id,
            event_type=event.event_type.value,
        )
        return True
