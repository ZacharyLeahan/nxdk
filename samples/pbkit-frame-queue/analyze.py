#!/usr/bin/env python3
"""Summarize an Xbox pbkit queue trace and print a short ordering witness."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


def load(path: Path) -> tuple[dict[str, str], list[dict[str, str]]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    split = lines.index("")
    metadata = dict(csv.reader(lines[1:split]))
    events = list(csv.DictReader(lines[split + 1 :]))
    return metadata, events


def describe(event: dict[str, str]) -> str:
    flags = []
    if event["repeated"] == "1":
        flags.append("repeated-refresh")
    if event["premature"] == "1":
        flags.append("PREMATURE")
    suffix = f" [{' '.join(flags)}]" if flags else ""
    return (
        f"seq={event['sequence']} refresh={event['refresh']} "
        f"{event['event']} frame={event['frame']} slot={event['slot']} "
        f"buffer={event['buffer']} scanout={event['scanout']} "
        f"target={event['target']}{suffix}"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    metadata, events = load(args.trace)

    keys = (
        "variant",
        "stock_commit",
        "patched_commit",
        "compiler",
        "video_mode",
        "observed_refreshes",
        "frames_queued",
        "queue_removals",
        "premature_removals",
        "repeated_refreshes",
        "deferred_removals",
        "stall_episodes",
        "stall_total_us",
        "stall_max_us",
        "selected_interval_p95_us",
        "selected_interval_max_us",
        "trace_overflow",
    )
    for key in keys:
        print(f"{key}: {metadata[key]}")

    failures = [i for i, event in enumerate(events) if event["premature"] == "1"]
    if not failures:
        print("ordering witness: none (stock did not reproduce in this run)" if metadata["variant"] == "stock" else "ordering witness: none")
        return 0

    index = failures[0]
    failed = events[index]
    related = [
        event
        for event in events[: index + 1]
        if event["slot"] == failed["slot"] and event["frame"] == failed["frame"]
    ][-4:]
    print("ordering witness:")
    for event in related:
        print(f"  {describe(event)}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
