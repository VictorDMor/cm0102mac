"""Validate the Croatian Cup authoring file and emit its immutable C++ profiles."""
import argparse
import datetime
import json
from pathlib import Path
import re

# Game label IDs and weekday encoding are part of the native calendar adapter.
STAGES = {"preliminary": 230, "round_of_32": 10, "round_of_16": 20,
          "quarterfinal": 120, "semifinal": 130, "final": 150}
WEEKDAYS = "Mon Tue Wed Thu Fri Sat Sun".split()
EDITIONS = ("cm0001_389", "cm0102", "si_3968")
ROUND_KEYS = {"new_entrants", "match_flags", "fixture_rule", "legs", "gap", "draw", "fixture"}


def unique_object(items):
    result = {}
    for key, value in items:
        if key in result:
            raise ValueError(f"duplicate field {key!r}")
        result[key] = value
    return result


def fields(obj, required, optional=()):
    if not isinstance(obj, dict):
        raise ValueError("expected an object")
    missing = set(required) - obj.keys()
    extra = obj.keys() - set(required) - set(optional)
    if missing or extra:
        raise ValueError(f"missing fields {sorted(missing)}, unknown fields {sorted(extra)}")


def integer(obj, key, maximum, minimum=0):
    value = obj[key]
    if type(value) is not int or not minimum <= value <= maximum:
        raise ValueError(f"{key} must be an integer from {minimum} to {maximum}")
    return value


def date(value):
    match = re.fullmatch(r"(\+?)(\d{2})-(\d{2}) ([A-Z][a-z]{2})", value) if isinstance(value, str) else None
    if not match or match[4] not in WEEKDAYS:
        raise ValueError(f"invalid calendar anchor {value!r}")
    offset, month, day = int(bool(match[1])), int(match[2]), int(match[3])
    # Annual competitions must have valid anchors even in non-leap years.
    datetime.date(2001, month, day)
    weekday = WEEKDAYS.index(match[4])
    # Cover a full Gregorian cycle. Different anchors can land on the same
    # weekday or reverse order once the native nearest-weekday rule is applied.
    resolved = []
    for season in range(2000, 2400):
        anchor = datetime.date(season + offset, month, day)
        resolved.append(anchor + datetime.timedelta(days=(weekday - anchor.weekday() + 3) % 7 - 3))
    return resolved, f"{{{day}, {month - 1}, {offset}, {weekday}}}"


def build_profile(data, profile, default_prize_pool, default_seeds):
    fields(profile, ("first_places", "second_places", "lower_places", "lower_pool_limit",
                     "second_division", "second_order"), ("round_overrides", "prize_pool", "seed_range"))
    first = integer(profile, "first_places", 32767)
    second = integer(profile, "second_places", 32767)
    lower = integer(profile, "lower_places", 32767)
    limit = integer(profile, "lower_pool_limit", 32767, lower)
    division, order = profile["second_division"], profile["second_order"]
    if division not in ("regional_groups", "national_table") or order not in ("database", "seed_then_reputation"):
        raise ValueError("unknown second division or seed ordering policy")
    overrides = profile.get("round_overrides", {})
    fields(overrides, (), STAGES)
    if not isinstance(data["rounds"], list) or [row["stage"] for row in data["rounds"]] != list(STAGES):
        raise ValueError("expected the six Croatian Cup stages in bracket order")

    entrants = winners = 0
    first_matches = previous_end = None
    rows = []
    for base in data["rounds"]:
        stage = base["stage"]
        fields(base, ROUND_KEYS | {"stage"})
        patch = overrides.get(stage, {})
        fields(patch, (), ROUND_KEYS)
        row = {**base, **patch}
        join = integer(row, "new_entrants", 32767)
        clubs = winners + join
        if clubs < 2 or clubs > 32767 or clubs % 2:
            raise ValueError(f"{stage} cannot pair {clubs} clubs")
        flags = integer(row, "match_flags", 65535)
        rule = integer(row, "fixture_rule", 255)
        legs = integer(row, "legs", 2, 1)
        gap = integer(row, "gap", 255)
        if (legs == 2) != (gap > 0):
            raise ValueError(f"{stage} leg count and interval disagree")
        draws, draw = date(row["draw"])
        matches, fixture = date(row["fixture"])
        if any(d > m for d, m in zip(draws, matches)):
            raise ValueError(f"{stage} can start before its draw")
        if previous_end and any(m <= end for m, end in zip(matches, previous_end)):
            raise ValueError(f"{stage} can overlap the preceding round")
        if first_matches is None:
            first_matches = matches
        previous_end = [m + datetime.timedelta(days=gap) for m in matches]
        rows.append(f"        {{{STAGES[stage]}, {clubs}, {join}, {entrants if join else 0}, "
                    f"{flags}, {rule}, {legs}, {gap}, {draw}, {fixture}}}")
        entrants += join
        winners = clubs // 2

    # The next Gregorian cycle repeats after exactly 146097 days.
    next_openings = first_matches[1:] + [first_matches[0] + datetime.timedelta(days=146097)]
    if any(end >= opening for end, opening in zip(previous_end, next_openings)):
        raise ValueError("final can overlap the next edition")
    if winners != 1 or entrants != first + second + lower or entrants > 32767:
        raise ValueError("bracket must admit every entrant once and produce one winner")
    prize_pool = integer(profile, "prize_pool", 2**31 - 1) if "prize_pool" in profile else default_prize_pool
    seeds = integer(profile, "seed_range", 128, 1) if "seed_range" in profile else default_seeds
    return (f"{{{prize_pool}, {seeds}, {first}, {second}, {lower}, {limit}, "
            f"SecondDivision::{division}, SecondOrder::{order}, {{{{\n" + ",\n".join(rows) + "\n    }}}")


def compile_profiles(data):
    fields(data, ("editions", "prize_pool", "seed_range", "rounds", "profiles"))
    fields(data["editions"], EDITIONS)
    prize_pool = integer(data, "prize_pool", 2**31 - 1)
    # The guest reads seeds as signed bytes; negative values include sentinels.
    seeds = integer(data, "seed_range", 128, 1)
    profiles = data["profiles"]
    if not isinstance(profiles, dict) or not profiles:
        raise ValueError("expected at least one profile")
    entries = ["// Generated from the Croatian Cup rules. Do not edit."]
    profiles_by_name = {}
    for index, (name, profile) in enumerate(profiles.items()):
        try:
            body = build_profile(data, profile, prize_pool, seeds)
        except (KeyError, TypeError, ValueError) as error:
            raise ValueError(f"profile {name}: {error}") from error
        profiles_by_name[name] = f"profile_{index}"
        entries.append(f"constexpr Profile profile_{index}{body};")
    for edition in EDITIONS:
        name = data["editions"][edition]
        if not isinstance(name, str) or name not in profiles_by_name:
            raise ValueError(f"unknown profile for edition {edition}: {name!r}")
        entries.append(f"const Profile {edition} = {profiles_by_name[name]};")
    return "\n".join(entries) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        source = json.loads(args.source.read_text(), object_pairs_hook=unique_object)
        result = compile_profiles(source)
    except (ValueError, TypeError, KeyError) as error:
        parser.exit(1, f"{args.source}: {error}\n")
    args.output.write_text(result)
