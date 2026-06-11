"""Tests for TikTok string utility helpers."""

from __future__ import annotations

import pytest

from tiktok_event_gateway.utils.tiktok import build_tiktok_live_url, normalize_tiktok_username


@pytest.mark.parametrize(
    ("value", "expected"),
    [
        ("username", "username"),
        ("@username", "username"),
        ("@@username", "username"),
        ("  @username  ", "username"),
    ],
)
def test_normalize_tiktok_username(value: str, expected: str) -> None:
    assert normalize_tiktok_username(value) == expected


@pytest.mark.parametrize("value", ["username", "@username", "@@username", "  @username  "])
def test_build_tiktok_live_url_never_contains_double_at(value: str) -> None:
    url = build_tiktok_live_url(value)

    assert url == "https://www.tiktok.com/@username/live"
    assert "@@" not in url


def test_build_tiktok_live_url_rejects_empty_username() -> None:
    with pytest.raises(ValueError, match="TikTok username is required"):
        build_tiktok_live_url("@@  ")
