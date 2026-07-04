#!/usr/bin/env python3
"""Create a temporary Caedral API key for SDK-C integration tests."""

from __future__ import annotations

import base64
import os
import secrets
import sys
import uuid
from pathlib import Path

try:
    import bcrypt
    import psycopg
except ImportError:
    sys.stderr.write("Install test deps: pip install psycopg[binary] bcrypt\n")
    sys.exit(1)

API_KEY_PREFIX = "cd_live_"


def load_root_env() -> None:
    root = Path(__file__).resolve().parents[1] / ".env"
    if not root.exists():
        return
    for line in root.read_text(encoding="utf-8").splitlines():
        trimmed = line.strip()
        if not trimmed or trimmed.startswith("#") or "=" not in trimmed:
            continue
        key, value = trimmed.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        os.environ.setdefault(key, value)


def main() -> int:
    load_root_env()
    existing = os.environ.get("CAEDRAL_TEST_API_KEY")
    if existing:
        print(existing)
        return 0

    database_url = os.environ.get("DATABASE_URL")
    if not database_url:
        sys.stderr.write("DATABASE_URL is required to create a test API key\n")
        return 1

    user_id = str(uuid.uuid4())
    api_key_id = str(uuid.uuid4())
    sub_id = str(uuid.uuid4())
    raw_key = API_KEY_PREFIX + base64.urlsafe_b64encode(secrets.token_bytes(24)).decode().rstrip("=")
    key_prefix = raw_key[:16]
    key_hash = bcrypt.hashpw(raw_key.encode(), bcrypt.gensalt(rounds=10)).decode()
    email = f"sdk-c-test-{user_id}@example.com"

    with psycopg.connect(database_url, autocommit=False) as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                INSERT INTO "user" (id, name, email, email_verified, balance_cents, account_status)
                VALUES (%s, %s, %s, %s, %s, %s)
                """,
                (user_id, "SDK C Test", email, True, 0, "active"),
            )
            cur.execute(
                """
                INSERT INTO subscriptions (
                  id, user_id, plan, status, weekly_pool_limit, weekly_pool_used,
                  overage_enabled, overage_used_cents
                )
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                """,
                (sub_id, user_id, "pro", "active", 1_000_000, 0, False, 0),
            )
            cur.execute(
                """
                INSERT INTO api_keys (id, user_id, name, key_prefix, key_hash)
                VALUES (%s, %s, %s, %s, %s)
                """,
                (api_key_id, user_id, "SDK C test key", key_prefix, key_hash),
            )
        conn.commit()

    print(raw_key)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
