"""TikTok Event Gateway package.

The package exposes a reusable event middleware service that ingests livestream
events, normalizes them into stable schemas, and broadcasts them to external
applications without embedding application-specific behavior.
"""

__all__ = ["__version__"]

__version__ = "0.1.0"
