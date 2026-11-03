"""CONTRACT_SCREENS.CPP: original inventory and C++23 routing.

The previous module's static initializer returns before 0x10093a50.
The news callback there and the next entry both reference contract screen
callbacks, preceding the first load of the source pool. DATABASE.CPP begins
at 0x1009efc0. The module has 15 transition vectors and one 45-slot switch.
"""
from pathlib import Path
import re

from tactics_audit import digest, routine_records

ROOT = Path(__file__).resolve().parent.parent
MODULE = (0x10093a50, 0x1009efc0)
CONTRACTS = (
    (0x10093a50, 0x10093ad8, 34, "9a371569e556b3af"),
    (0x10093ae0, 0x10093bd0, 60, "d8dde50ffeb286a2"),
    (0x10093bd0, 0x10093e54, 161, "c05ae860b67832eb"),
    (0x10093e60, 0x10094560, 448, "e5eb4cb0138813bc"),
    (0x10094560, 0x10094ad8, 350, "061e7c8cca5c235c"),
    (0x10094ae0, 0x100952a4, 496, "3c3021bbad760be8"),
    (0x100952b0, 0x10095da8, 700, "78bad96bdd73d167"),
    (0x10095db0, 0x100960cc, 199, "a80eb8aa97a0fa99"),
    (0x100960d0, 0x100965bc, 315, "7fa79f19b4793f48"),
    (0x100965c0, 0x10096c58, 422, "5abcf857ce75987f"),
    (0x10096c60, 0x10097b68, 962, "d547debcd349a930"),
    (0x10097b70, 0x100980c4, 341, "10efe22e0eb63d0d"),
    (0x100980d0, 0x10098124, 21, "36a44e9dc84145dc"),
    (0x10098130, 0x1009a434, 2240, "bd526df8ff651d98"),
    (0x1009a440, 0x1009b614, 1141, "dd67c03073208b1c"),
    (0x1009b620, 0x1009c018, 638, "6a23938862db98db"),
    (0x1009c020, 0x1009c86c, 531, "18d67e9694dbed11"),
    (0x1009c870, 0x1009d404, 740, "ec62ca981fd50455"),
    (0x1009d410, 0x1009daa4, 421, "8502c8b6856ecda7"),
    (0x1009dab0, 0x1009dbf8, 82, "c0765791fe0f8b47"),
    (0x1009dc00, 0x1009e28c, 419, "cc255500a2184fc3"),
    (0x1009e290, 0x1009e874, 376, "060f776adc69f128"),
    (0x1009e880, 0x1009e8ec, 27, "7950ace35a580e7e"),
    (0x1009e8f0, 0x1009eb24, 141, "d565666d8bcf79f8"),
    (0x1009eb30, 0x1009ed20, 124, "4d39aaa432e84169"),
    (0x1009ed20, 0x1009edfc, 55, "e7fb6f7e8abe47f3"),
    (0x1009ee00, 0x1009ef2c, 75, "70a534f8b1c64894"),
    (0x1009ef30, 0x1009ef54, 9, "14051150b2a214f9"),
    (0x1009ef60, 0x1009ef90, 12, "b2160c05fee1bdbb"),
    (0x1009ef90, 0x1009efb4, 9, "57e6302a8122607a"),
)
UNREACHABLE_NOPS = {0x10094c5c, 0x100953b8, 0x100959a8, 0x1009a018, 0x1009d288, 0x1009e7ac}
ROUTINES = ROOT / "native/contract_screens_routines.inc"
ROUTINE = re.compile(r'CONTRACT_SCREENS_ROUTINE\(\s*(0x[0-9a-f]+),')
CONTRACT_SCREENS_ENTRIES = tuple(int(entry, 16) for entry in ROUTINE.findall(ROUTINES.read_text()))


def require_contract_screens_index(db):
    starts = {entry for entry, _, _, _ in CONTRACTS}
    if set(CONTRACT_SCREENS_ENTRIES) - starts or len(set(CONTRACT_SCREENS_ENTRIES)) != len(CONTRACT_SCREENS_ENTRIES):
        raise ValueError("Invalid or duplicate CONTRACT_SCREENS.CPP owner")
    for entry, end, count, expected in CONTRACTS:
        body = routine_records(db, entry, end)
        addresses = [address for address in range(entry, end, 4) if address not in UNREACHABLE_NOPS]
        if [r["address"] for r in body] != addresses or len(body) != count or digest(body) != expected:
            raise ValueError(f"CONTRACT_SCREENS.CPP original changed at {entry:#x}")


def contract_screens_hook(entry):
    return f"if (try_modern_contract_screens(s, 0x{entry:x})) return true;"


def require_contract_screens_hooks(page, instructions, code):
    addresses = {r["address"] for r in instructions}
    for entry in CONTRACT_SCREENS_ENTRIES:
        if entry >> 12 != page or entry not in addresses:
            continue
        fragment = code.split(f"i_{entry:x}:", 1)[1].split(f"i_{entry + 4:x}:", 1)[0]
        hook, step = contract_screens_hook(entry), f"s.step(0x{entry:x});"
        if fragment.count(hook) != 1 or step not in fragment or fragment.index(hook) > fragment.index(step):
            raise ValueError(f"Missing or misplaced CONTRACT_SCREENS.CPP owner at {entry:#x}")
