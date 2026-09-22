#!/usr/bin/env python3
"""meeting_query.py — query docs/MEETING_LOG.md entries.

Entry format (strict, see PRINCIPLES.md section 4):
    ## [YYYY-MM-DD HH:MM] ROLE
    body lines...

Usage:
    py scripts/meeting_query.py --recent 5
    py scripts/meeting_query.py -s A
    py scripts/meeting_query.py -s A,B "HNSW"
    py scripts/meeting_query.py --since 2026-09-22 --until 2026-09-23
    py scripts/meeting_query.py --list-speakers
"""

import argparse
import re
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

HEADER_RE = re.compile(
    r"^##\s*\[(\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2})(?::\d{2})?\]\s*(.+?)\s*$"
)


@dataclass
class Entry:
    line_no: int
    when: datetime
    speaker: str
    tag: str  # optional trailing note, e.g. "补录" in "A（补录）"
    body: list = field(default_factory=list)

    @property
    def header(self) -> str:
        suffix = f"（{self.tag}）" if self.tag else ""
        return f"## [{self.when:%Y-%m-%d %H:%M}] {self.speaker}{suffix}"

    @property
    def text(self) -> str:
        return self.header + "\n" + "\n".join(self.body)


TAG_RE = re.compile(r"^(.*?)\s*[（(](.+?)[)）]$")


def parse_entries(path: Path) -> list:
    entries = []
    current = None
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except FileNotFoundError:
        sys.exit(f"error: meeting log not found: {path}")
    for idx, line in enumerate(lines, start=1):
        m = HEADER_RE.match(line)
        if m:
            when = datetime.fromisoformat(f"{m.group(1)}T{m.group(2)}")
            raw = m.group(3)
            tm = TAG_RE.match(raw)
            speaker, tag = (tm.group(1), tm.group(2)) if tm else (raw, "")
            current = Entry(idx, when, speaker, tag)
            entries.append(current)
        elif current is not None:
            current.body.append(line)
    return entries


def parse_date(s: str) -> datetime:
    for fmt in ("%Y-%m-%d %H:%M", "%Y-%m-%d"):
        try:
            return datetime.strptime(s, fmt)
        except ValueError:
            pass
    sys.exit(f"error: bad date (want YYYY-MM-DD[ HH:MM]): {s}")


def main() -> None:
    if sys.stdout.encoding and sys.stdout.encoding.lower() not in ("utf-8", "utf8"):
        sys.stdout.reconfigure(encoding="utf-8")

    ap = argparse.ArgumentParser(description="query MEETING_LOG.md entries")
    ap.add_argument("keywords", nargs="*", help="all keywords must appear (AND)")
    ap.add_argument("-s", "--speakers", default="", help="comma-separated roles, e.g. A,AI-A")
    ap.add_argument("--since", help="YYYY-MM-DD[ HH:MM]")
    ap.add_argument("--until", help="YYYY-MM-DD[ HH:MM]")
    ap.add_argument("-n", "--recent", type=int, default=0, help="only the last N matches")
    ap.add_argument("--compact", action="store_true", help="headers only")
    ap.add_argument("--list-speakers", action="store_true", help="print speaker tally")
    args = ap.parse_args()

    log = Path(__file__).resolve().parent.parent / "docs" / "MEETING_LOG.md"
    entries = parse_entries(log)

    if args.list_speakers:
        tally = {}
        for e in entries:
            tally[e.speaker] = tally.get(e.speaker, 0) + 1
        for speaker, count in sorted(tally.items()):
            print(f"{count:4d}  {speaker}")
        return

    wanted = {s.strip() for s in args.speakers.split(",") if s.strip()}
    since = parse_date(args.since) if args.since else None
    until = parse_date(args.until) if args.until else None

    hits = []
    for e in entries:
        if wanted and e.speaker not in wanted:
            continue
        if since and e.when < since:
            continue
        if until and e.when > until:
            continue
        if args.keywords:
            blob = e.text.lower()
            if not all(k.lower() in blob for k in args.keywords):
                continue
        hits.append(e)

    if args.recent > 0:
        hits = hits[-args.recent:]

    if not hits:
        print("no matching entries.")
        return

    for e in hits:
        if args.compact:
            print(f"L{e.line_no:<5} {e.header}")
        else:
            print(f"----- L{e.line_no} -----")
            print(e.text.rstrip())
            print()


if __name__ == "__main__":
    main()
