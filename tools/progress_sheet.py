#!/usr/bin/env python3
"""The local recompilation progress workbook, docs/recompilation-progress.xlsx.

  progress_sheet.py check     print the summary figures and fail on an inconsistent row
  progress_sheet.py refresh   rebuild the "Visão Geral" tab from "Módulos C++", then check

The workbook is a copy of Victor's Google Sheet (taken 2026-11-09) and the
record the agent maintains. Edit module rows with openpyxl, keep the formulas,
then run `refresh`. openpyxl stores no formula results and this machine has no
LibreOffice, so `check` computes the "Resumo" figures here; a spreadsheet
program recalculates them when it opens the file.

"Visão Geral" is a Google SORT(FILTER(...)) by status, then module. The xlsx
export keeps each cell as IFERROR(__xludf.DUMMYFUNCTION(...), value): Google
restores the live formula on import, and other programs show the value.
"""
import sys
from pathlib import Path

import openpyxl

PATH = Path(__file__).resolve().parent.parent / "docs/recompilation-progress.xlsx"
MODULES, OVERVIEW, SUMMARY = "Módulos C++", "Visão Geral", "Resumo"
SORT = "SORT(FILTER('Módulos C++'!A2:M1000, 'Módulos C++'!A2:A1000<>\"\"\"\"), 2, 1, 1, 1)"
DONE = "Concluído"


def literal(value):
    """An Excel literal; strings in pieces of at most 255 characters."""
    if value is None:
        return '""'
    if isinstance(value, (int, float)):
        return repr(value)
    text = str(value).replace('"', '""')
    pieces, start = [], 0
    while start < len(text) or not pieces:
        end = min(start + 255, len(text))
        if text[end - 1:end] == '"' and text[:end].count('"') % 2:  # never split a doubled quote
            end -= 1
        pieces.append(f'"{text[start:end]}"')
        start = end
    return "&".join(pieces)


def collation(text):
    """Google's order for these names: case ignored, and "_" before "."."""
    return text.casefold().replace("_", "\x01")


def module_rows(book):
    """Module rows with the percentage formula replaced by its value."""
    rows = []
    for cells in book[MODULES].iter_rows(min_row=2, max_col=13):
        row = [cell.value for cell in cells]
        if row[0] in (None, ""):
            continue
        new, total = row[3], row[4]
        measured = isinstance(new, (int, float)) and isinstance(total, (int, float)) and total > 0
        row[2] = new / total if measured else None
        rows.append(row)
    return rows


def overview_cell(index, column, value):
    """The cell as Google exports it. An empty source cell exports empty; an
    unmeasured percentage (the formula's "") exports as ""."""
    if value is None and column != 3:
        return None
    source = SORT if (index, column) == (2, 1) else '""COMPUTED_VALUE""'
    return f'=IFERROR(__xludf.DUMMYFUNCTION("{source}"),{literal(value)})'


def refresh(book):
    rows = sorted(module_rows(book), key=lambda row: (collation(row[1]), collation(row[0])))
    sheet = book[OVERVIEW]
    for index, row in enumerate(rows, start=2):
        for column, value in enumerate(row, start=1):
            sheet.cell(index, column).value = overview_cell(index, column, value)
    for index in range(len(rows) + 2, sheet.max_row + 1):
        for column in range(1, 14):
            sheet.cell(index, column).value = None


def check(book):
    rows = module_rows(book)
    problems = []
    for row in rows:
        name, status, _, new, total = row[:5]
        if status == DONE and not (isinstance(new, (int, float)) and new == total and total > 0):
            problems.append(f"{name}: {DONE} with {new}/{total}")
        if status != DONE and isinstance(new, (int, float)) and new == total:
            problems.append(f"{name}: {new}/{total} but status is {status}")
    statuses = {}
    for row in rows:
        statuses[row[1]] = statuses.get(row[1], 0) + 1
    done = [row for row in rows if row[1] == DONE]
    print(f"modules {len(rows)}, {DONE} {len(done)}, routines in them {sum(row[3] for row in done):g}")
    for status, count in sorted(statuses.items()):
        print(f"  {count:4} {status}")
    overview = [cells[0].value for cells in book[OVERVIEW].iter_rows(min_row=2, max_col=1) if cells[0].value]
    if len(overview) != len(rows):
        problems.append(f"{OVERVIEW} has {len(overview)} rows for {len(rows)} modules: run refresh")
    for problem in problems:
        print("PROBLEM", problem)
    return not problems


def main():
    command = sys.argv[1] if len(sys.argv) == 2 else ""
    if command not in ("check", "refresh"):
        sys.exit(__doc__)
    book = openpyxl.load_workbook(PATH)
    if command == "refresh":
        refresh(book)
        book.calculation.fullCalcOnLoad = True
        book.save(PATH)
    sys.exit(0 if check(book) else 1)


if __name__ == "__main__":
    main()
