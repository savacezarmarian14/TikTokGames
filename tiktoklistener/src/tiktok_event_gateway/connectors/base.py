"""Abstract connector contracts.

Connectors are the only layer allowed to talk to provider-specific libraries.
They should convert provider events into internal raw event envelopes before
anything reaches the rest of the gateway.
"""

from __future__ import annotations

import asyncio
from abc import ABC, abstractmethod
from collections.abc import AsyncIterator
from contextlib import suppress
from typing import Any, Final

from tiktok_event_gateway.normalization import RawEvent

_STOP: Final = object()


class ConnectorRuntimeError(RuntimeError):
    """Raised when a connector producer task fails unexpectedly."""


class BaseConnector(ABC):
    """Base class for asynchronous raw event source connectors.

    Subclasses implement ``_run`` and call ``emit`` whenever a provider event is
    available. Consumers read raw events from ``events`` and pass them to a
    provider-specific normalizer.
    """

    def __init__(self, *, name: str, max_queue_size: int = 1000) -> None:
        if max_queue_size <= 0:
            msg = "max_queue_size must be greater than zero"
            raise ValueError(msg)
        self.name = name
        self._max_queue_size = max_queue_size
        self._queue: asyncio.Queue[RawEvent | object] = asyncio.Queue(maxsize=max_queue_size)
        self._task: asyncio.Task[None] | None = None
        self._task_exception: BaseException | None = None
        self._stop_requested = asyncio.Event()

    async def start(self) -> None:
        """Start reading events from the external provider."""
        if self.is_running():
            return
        self._queue = asyncio.Queue(maxsize=self._max_queue_size)
        self._task_exception = None
        self._stop_requested.clear()
        self._task = asyncio.create_task(self._run(), name=f"{self.name}-connector")
        self._task.add_done_callback(self._on_task_done)

    async def stop(self) -> None:
        """Stop reading events and release provider resources."""
        self._stop_requested.set()
        task = self._task
        if task is not None and not task.done():
            task.cancel()
            with suppress(asyncio.CancelledError):
                await task
        self._task = None
        self._wake_event_consumers()

    def is_running(self) -> bool:
        """Return whether the connector producer task is currently active."""
        return self._task is not None and not self._task.done()

    async def events(self) -> AsyncIterator[RawEvent]:
        """Yield raw events produced by the connector until it stops."""
        while True:
            item = await self._queue.get()
            if item is _STOP:
                if self._task_exception is not None:
                    msg = f"connector {self.name!r} failed"
                    raise ConnectorRuntimeError(msg) from self._task_exception
                break
            yield item

    def __aiter__(self) -> AsyncIterator[RawEvent]:
        """Allow ``async for raw_event in connector`` consumption."""
        return self.events()

    async def emit(self, raw_event: RawEvent) -> None:
        """Publish one raw event to connector consumers."""
        await self._queue.put(raw_event)

    def stop_requested(self) -> bool:
        """Return whether shutdown has been requested."""
        return self._stop_requested.is_set()

    async def wait_until_stopped(self, timeout: float | None = None) -> bool:
        """Sleep until shutdown is requested; return true if it happened."""
        try:
            await asyncio.wait_for(self._stop_requested.wait(), timeout=timeout)
        except asyncio.TimeoutError:
            return False
        return True

    def _wake_event_consumers(self) -> None:
        try:
            self._queue.put_nowait(_STOP)
        except asyncio.QueueFull:
            with suppress(asyncio.QueueEmpty):
                self._queue.get_nowait()
            with suppress(asyncio.QueueFull):
                self._queue.put_nowait(_STOP)

    def _on_task_done(self, task: asyncio.Task[None]) -> None:
        if task.cancelled():
            self._wake_event_consumers()
            return
        self._task_exception = task.exception()
        self._wake_event_consumers()

    @abstractmethod
    async def _run(self) -> None:
        """Produce raw events until cancellation or stop is requested."""


EventConnector = BaseConnector
