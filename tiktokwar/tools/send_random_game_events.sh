#!/usr/bin/env bash

HOST="127.0.0.1"
PORT="7000"
INTERVAL="1"

GIFTS=(
    "Rose|1|spawn Red Blue"
    "Pop|1|spawn Red Purple"
    "Heart Me|1|heal Red"

    "TikTok|1|spawn Blue Red"
    "You're awesome|1|spawn Blue Purple"
    "Love you so much|1|heal Blue"

    "GG|1|spawn Purple Red"
    "Oldies|1|spawn Purple Blue"
    "Ice Cream Cone|1|heal Purple"
)

while [[ $# -gt 0 ]]; do
    case "$1" in
        --host)
            HOST="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --interval)
            INTERVAL="$2"
            shift 2
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

send_udp() {
    local message="$1"

    if command -v nc >/dev/null 2>&1; then
        printf "%s" "$message" | nc -u -w1 "$HOST" "$PORT"
    else
        echo "Error: nc/netcat is not installed."
        exit 1
    fi
}

echo "Sending random game events to udp://$HOST:$PORT every ${INTERVAL}s"
echo "Press Ctrl+C to stop."

while true; do
    GIFT_ENTRY="${GIFTS[$RANDOM % ${#GIFTS[@]}]}"

    IFS="|" read -r GIFT_NAME COIN_VALUE ACTION <<< "$GIFT_ENTRY"

    REPEAT_COUNT="$((RANDOM % 5 + 1))"
    EVENT_ID="evt_$(date +%s)_$RANDOM"
    USER_ID="$((RANDOM % 9000 + 1000))"
    VIEWER_NO="$((RANDOM % 100 + 1))"
    TIMESTAMP="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

    JSON=$(cat <<EOF
{
  "message_type": "game_event",
  "action": "$ACTION",
  "event": {
    "id": "$EVENT_ID",
    "type": "gift",
    "timestamp": "$TIMESTAMP"
  },
  "user": {
    "id": "$USER_ID",
    "username": "viewer_$VIEWER_NO",
    "display_name": "Viewer $VIEWER_NO"
  },
  "payload": {
    "gift_name": "$GIFT_NAME",
    "coin_value": $COIN_VALUE,
    "repeat_count": $REPEAT_COUNT,
    "is_streak_final": true
  }
}
EOF
)

    echo "Sending: $ACTION | gift=$GIFT_NAME | repeat=$REPEAT_COUNT"
    send_udp "$JSON"

    sleep "$INTERVAL"
done