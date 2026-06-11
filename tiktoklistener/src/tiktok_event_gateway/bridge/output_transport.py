"""Output transports for generated game JSON messages."""

from __future__ import annotations

import asyncio
import json
import socket
from typing import Protocol


class GameBridgeOutputTransport(Protocol):
    """Async output transport interface used by the game bridge."""

    async def start(self) -> None: ...

    async def send(self, message: dict[str, object]) -> None: ...

    async def stop(self) -> None: ...


class StdoutGameBridgeTransport:
    """Print generated game messages to stdout, useful for debugging."""

    async def start(self) -> None:
        return None

    async def send(self, message: dict[str, object]) -> None:
        print(json.dumps(message, ensure_ascii=False, separators=(",", ":")), flush=True)

    async def stop(self) -> None:
        return None


class UdpGameBridgeTransport:
    """Send each generated game JSON message as one UDP packet."""

    def __init__(self, *, host: str, port: int) -> None:
        self.host = host
        self.port = port
        self._socket: socket.socket | None = None

    async def start(self) -> None:
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._socket.setblocking(False)

    async def send(self, message: dict[str, object]) -> None:
        if self._socket is None:
            await self.start()
        assert self._socket is not None
        payload = json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        loop = asyncio.get_running_loop()
        await loop.sock_sendto(self._socket, payload, (self.host, self.port))

    async def stop(self) -> None:
        if self._socket is not None:
            self._socket.close()
            self._socket = None


def build_output_transport(*, output_type: str, host: str, port: int) -> GameBridgeOutputTransport:
    """Create an output transport from validated settings."""
    normalized = output_type.strip().lower()
    if normalized == "stdout":
        return StdoutGameBridgeTransport()
    if normalized == "udp":
        return UdpGameBridgeTransport(host=host, port=port)
    msg = f"unsupported game bridge output transport: {output_type}"
    raise ValueError(msg)
