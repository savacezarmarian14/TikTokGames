"""Setuptools compatibility shim for editable installs.

The canonical project metadata lives in ``pyproject.toml``. This file exists so
older local tooling can still perform ``pip install -e ".[dev]"``.
"""

from setuptools import find_packages, setup


INSTALL_REQUIRES = [
    "pydantic>=2.7,<3",
    "pydantic-settings>=2.2,<3",
    "PyYAML>=6.0.1,<7",
    "websockets>=12,<14",
    "structlog>=24.1,<25",
    "typer>=0.12,<0.13",
]

DEV_REQUIRES = [
    "black>=24.4,<25",
    "pytest>=8.2,<9",
    "pytest-asyncio>=0.23,<1",
    "ruff>=0.6,<0.7",
    "mypy>=1.10,<1.12",
]


setup(
    name="tiktok-event-gateway",
    version="0.1.0",
    description=(
        "Reusable asyncio middleware for normalizing TikTok LIVE events and "
        "broadcasting them over WebSockets."
    ),
    package_dir={"": "src"},
    packages=find_packages("src"),
    package_data={"tiktok_event_gateway": ["py.typed"]},
    python_requires=">=3.10",
    install_requires=INSTALL_REQUIRES,
    extras_require={
        "dev": DEV_REQUIRES,
        "tiktok": ["TikTokLive>=6,<7"],
    },
    entry_points={
        "console_scripts": [
            "tiktok-event-gateway=tiktok_event_gateway.cli:main",
        ]
    },
)
