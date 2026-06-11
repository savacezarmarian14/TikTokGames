"""TikTok-specific string helpers.

These helpers are deliberately dependency-free. They normalize user input before
it reaches TikTokLive so URLs and unique ids never accidentally contain values
such as ``@@username``.
"""

from __future__ import annotations


def normalize_tiktok_username(username: str) -> str:
    """Return a TikTok username without whitespace or leading ``@`` characters.

    The gateway stores usernames internally without ``@``. Code that needs a
    display name or a TikTokLive unique id can add exactly one ``@`` at the
    boundary where that representation is required.
    """
    return username.strip().lstrip("@").strip()


def build_tiktok_live_url(username: str) -> str:
    """Build a canonical TikTok LIVE URL with exactly one ``@``."""
    normalized_username = normalize_tiktok_username(username)
    if not normalized_username:
        msg = "TikTok username is required"
        raise ValueError(msg)
    return f"https://www.tiktok.com/@{normalized_username}/live"
