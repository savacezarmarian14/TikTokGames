import asyncio
import json
import socket
from datetime import datetime

from TikTokLive import TikTokLiveClient
from TikTokLive.events import ConnectEvent, CommentEvent, LikeEvent, FollowEvent, GiftEvent


# =========================
# CONFIG
# =========================
TIKTOK_USERNAME = "tower.war_team.red"
GAME_HOST = "127.0.0.1"
GAME_PORT = 5005

# Moduri posibile:
# - "single_color": toate acțiunile merg pe o singură culoare
# - "gift_color": culoarea depinde de gift
MODE = "single_color"
ACTIVE_COLOR = "red"

# Gift -> acțiune în joc
# Modifică liber după cum vrei tu
GIFT_MAPPINGS = {
    "Rose": {
        "action": "spawn_unit",
        "team": "red",
        "power": 1
    },
    "Finger Heart": {
        "action": "spawn_unit",
        "team": "blue",
        "power": 2
    },
    "Perfume": {
        "action": "heal_tower",
        "team": "green",
        "power": 5
    },
    "Galaxy": {
        "action": "ultimate",
        "team": "red",
        "power": 20
    }
}

# Comentarii speciale
COMMENT_COMMANDS = {
    "red": {
        "action": "vote_team",
        "team": "red",
        "power": 1
    },
    "blue": {
        "action": "vote_team",
        "team": "blue",
        "power": 1
    },
    "green": {
        "action": "vote_team",
        "team": "green",
        "power": 1
    },
    "heal": {
        "action": "heal_tower",
        "team": "red",
        "power": 2
    }
}


# =========================
# UDP SENDER
# =========================
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


def send_to_game(payload: dict) -> None:
    data = json.dumps(payload).encode("utf-8")
    sock.sendto(data, (GAME_HOST, GAME_PORT))
    print(f"[SEND -> GAME] {json.dumps(payload, ensure_ascii=False)}")


def build_base_payload(event_type: str, user: str) -> dict:
    return {
        "type": event_type,
        "user": user,
        "timestamp": datetime.utcnow().isoformat() + "Z"
    }


def resolve_team(default_team: str | None = None) -> str:
    if MODE == "single_color":
        return ACTIVE_COLOR
    return default_team or ACTIVE_COLOR


# =========================
# TIKTOK CLIENT
# =========================
client = TikTokLiveClient(unique_id=TIKTOK_USERNAME)


@client.on(ConnectEvent)
async def on_connect(event: ConnectEvent):
    print(f"[CONNECTED] LIVE conectat la @{client.unique_id}")


@client.on(CommentEvent)
async def on_comment(event: CommentEvent):
    user = event.user.nickname if event.user else "unknown"
    text = (event.comment or "").strip()

    print(f"[COMMENT] {user}: {text}")

    key = text.lower()
    if key in COMMENT_COMMANDS:
        cmd = COMMENT_COMMANDS[key]

        payload = build_base_payload("comment_command", user)
        payload.update({
            "source_text": text,
            "action": cmd["action"],
            "team": resolve_team(cmd.get("team")),
            "power": cmd.get("power", 1)
        })

        send_to_game(payload)


@client.on(LikeEvent)
async def on_like(event: LikeEvent):
    user = event.user.nickname if event.user else "unknown"

    like_count = getattr(event, "count", 1)
    total_like_count = getattr(event, "total", None)

    print(f"[LIKE] {user} | count={like_count} total={total_like_count}")

    payload = build_base_payload("like", user)
    payload.update({
        "action": "add_energy",
        "team": resolve_team("red"),
        "power": like_count,
        "like_count": like_count,
        "total_like_count": total_like_count
    })

    send_to_game(payload)


@client.on(FollowEvent)
async def on_follow(event: FollowEvent):
    user = event.user.nickname if event.user else "unknown"

    print(f"[FOLLOW] {user}")

    payload = build_base_payload("follow", user)
    payload.update({
        "action": "spawn_unit",
        "team": resolve_team("red"),
        "power": 2
    })

    send_to_game(payload)


@client.on(GiftEvent)
async def on_gift(event: GiftEvent):
    user = event.user.nickname if event.user else "unknown"

    gift_name = getattr(event.gift, "name", "UnknownGift")
    repeat_count = getattr(event, "repeat_count", 1)

    # Unele gift-uri vin în streak.
    # Pentru non-streak, tratăm direct.
    # Pentru streak, tratăm când streak-ul s-a terminat.
    if getattr(event.gift, "streakable", False):
        if not getattr(event, "streaking", False):
            process_gift(user, gift_name, repeat_count)
    else:
        process_gift(user, gift_name, repeat_count)


def process_gift(user: str, gift_name: str, repeat_count: int):
    print(f"[GIFT] {user} -> {gift_name} x{repeat_count}")

    if gift_name not in GIFT_MAPPINGS:
        payload = build_base_payload("gift_unmapped", user)
        payload.update({
            "gift": gift_name,
            "count": repeat_count,
            "action": "unknown_gift",
            "team": resolve_team("red"),
            "power": repeat_count
        })
        send_to_game(payload)
        return

    mapping = GIFT_MAPPINGS[gift_name]

    payload = build_base_payload("gift", user)
    payload.update({
        "gift": gift_name,
        "count": repeat_count,
        "action": mapping["action"],
        "team": resolve_team(mapping.get("team")),
        "power": mapping.get("power", 1) * repeat_count
    })

    send_to_game(payload)


async def main():
    print(f"[START] Pornesc TikTok listener pentru @{TIKTOK_USERNAME}")
    print(f"[START] Trimit comenzi spre joc la {GAME_HOST}:{GAME_PORT}")
    await client.start()


if __name__ == "__main__":
    asyncio.run(main())