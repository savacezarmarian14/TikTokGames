"""Internal event broker.

The broker will receive normalized events from an asyncio queue and fan them out
to one or more subscribers, such as WebSocket client connections.
"""

from __future__ import annotations

import asyncio
from collections.abc import Iterable
from dataclasses import dataclass, field
from uuid import uuid4

from tiktok_event_gateway.models import EventType, GatewayEvent


@dataclass
class BrokerSubscription:
    """One broker subscriber with an isolated delivery queue."""

    subscription_id: str
    event_types: frozenset[EventType] | None = None
    max_queue_size: int = 100
    queue: asyncio.Queue[GatewayEvent] = field(init=False)
    dropped_events: int = 0

    def __post_init__(self) -> None:
        self.queue = asyncio.Queue(maxsize=self.max_queue_size)

    def matches(self, event: GatewayEvent) -> bool:
        """Return whether this subscriber should receive the event."""
        return self.event_types is None or event.event_type in self.event_types

    def update_filter(self, event_types: Iterable[EventType] | None) -> None:
        """Replace the subscriber event filter."""
        self.event_types = None if event_types is None else frozenset(event_types)

    def enqueue_nowait(self, event: GatewayEvent) -> None:
        """Queue an event without allowing this subscriber to block fan-out."""
        try:
            self.queue.put_nowait(event)
            return
        except asyncio.QueueFull:
            self.dropped_events += 1

        try:
            self.queue.get_nowait()
        except asyncio.QueueEmpty:
            pass

        try:
            self.queue.put_nowait(event)
        except asyncio.QueueFull:
            self.dropped_events += 1


class EventBroker:
    """Fan out normalized events to independent subscriber queues."""

    def __init__(self, *, subscriber_queue_size: int = 100) -> None:
        if subscriber_queue_size <= 0:
            msg = "subscriber_queue_size must be greater than zero"
            raise ValueError(msg)
        self.subscriber_queue_size = subscriber_queue_size
        self._subscriptions: dict[str, BrokerSubscription] = {}

    def subscribe(
        self,
        *,
        event_types: Iterable[EventType] | None = None,
        subscription_id: str | None = None,
    ) -> BrokerSubscription:
        """Create a subscription. ``None`` means receive all event types."""
        subscription = BrokerSubscription(
            subscription_id=subscription_id or str(uuid4()),
            event_types=None if event_types is None else frozenset(event_types),
            max_queue_size=self.subscriber_queue_size,
        )
        self._subscriptions[subscription.subscription_id] = subscription
        return subscription

    def unsubscribe(self, subscription_id: str) -> None:
        """Remove a subscription if it still exists."""
        self._subscriptions.pop(subscription_id, None)

    async def publish(self, event: GatewayEvent) -> None:
        """Publish one normalized event to all matching subscribers."""
        for subscription in tuple(self._subscriptions.values()):
            if subscription.matches(event):
                subscription.enqueue_nowait(event)

    async def publish_from(self, events: asyncio.Queue[GatewayEvent]) -> None:
        """Continuously publish normalized events from an internal queue."""
        while True:
            event = await events.get()
            await self.publish(event)

    def update_subscription(
        self,
        subscription_id: str,
        event_types: Iterable[EventType] | None,
    ) -> None:
        """Update an existing subscription filter."""
        self._subscriptions[subscription_id].update_filter(event_types)

    def subscription_count(self) -> int:
        """Return the current subscriber count."""
        return len(self._subscriptions)
