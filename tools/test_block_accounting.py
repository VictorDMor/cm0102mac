"""Block accounting: one step per straight-line run instead of per instruction."""
import json
from pathlib import Path
import re
import sqlite3
import unittest
from compile_recomp import HOST_OBSERVED_INSTRUCTIONS, account_blocks, ends_run, generate_page

ROOT = Path(__file__).resolve().parent.parent
INPUT = ROOT / '.local/current/export'
GENERATED = ROOT / '.local/current/generated'
LABEL = re.compile(r'^i_([0-9a-f]+): //')
CASE = re.compile(r'^case 0x([0-9a-f]+): (?:(?:s|counter)\.step\(0x\1\);(?: (?:s|counter)\.charge\((\d+)\);)? )?goto i_\1;$')
STEP = re.compile(r'^(?:s|counter)\.step\(0x([0-9a-f]+)\);(?: (?:s|counter)\.charge\((\d+)\);)?$')


def charges(code):
    """Instructions charged on entry at each address, through the switch and at its label."""
    switch, label, current = {}, {}, None
    for line in code:
        if match := LABEL.match(line):
            current = int(match.group(1), 16)
        elif current is None and (match := CASE.match(line)):
            if 'step' in line:
                switch[int(match.group(1), 16)] = 1 + int(match.group(2) or 0)
        elif current is not None and (match := STEP.match(line)) and int(match.group(1), 16) == current:
            label[current] = 1 + int(match.group(2) or 0)
    return switch, label


class SyntheticPage(unittest.TestCase):
    def page(self):
        def item(address, flow='FALL_THROUGH', flows=(), ops=()):
            return {'address': address, 'size': 4, 'flow': flow, 'flows': list(flows), 'ops': list(ops)}
        instructions = [item(0x1000), item(0x1004),
                        item(0x1008, 'CONDITIONAL_JUMP', [0x1004], [['CBRANCH', None, ['ram', '4100', 4], ['unique', '0', 1]]]),
                        item(0x100c), item(0x1010), item(0x1014, 'TERMINATOR', [], [['RETURN', None, ['register', '4128', 4]]])]
        code = ['switch (s.pc) {'] + [f"case 0x{i['address']:x}: goto i_{i['address']:x};" for i in instructions]
        code += ['default: return false;', '}']
        for i in instructions:
            a = i['address']
            code += [f'i_{a:x}: // x', f's.step(0x{a:x});', 'goto i_1014;' if a == 0x100c else f'goto i_{a + 4:x};']
        return instructions, code

    def test_runs_start_at_targets_after_branches_at_hooks_and_at_non_fall_through_gotos(self):
        instructions, code = self.page()
        switch, label = charges(account_blocks(code, instructions, False, {0x1010}))
        # Runs: [1000] [1004 1008] [100c] [1010] [1014].
        self.assertEqual(label, {0x1000: 1, 0x1004: 2, 0x100c: 1, 0x1010: 1, 0x1014: 1})
        self.assertEqual(switch, {0x1008: 1})

    def test_an_isolated_instruction_runs_alone(self):
        instructions, code = self.page()
        switch, label = charges(account_blocks(code, instructions, False, set(), {0x1010}))
        # Runs: [1000] [1004 1008] [100c] [1010] [1014]; 1010 alone, 1014 after it.
        self.assertEqual(label, {0x1000: 1, 0x1004: 2, 0x100c: 1, 0x1010: 1, 0x1014: 1})
        switch, label = charges(account_blocks(code, instructions, False, set(), {0x1000}))
        self.assertEqual(label[0x1000], 1)
        self.assertIn(0x1004, label)

    def test_turbo_pages_charge_through_their_counter(self):
        instructions, code = self.page()
        code = [line.replace('s.step', 'counter.step') for line in code]
        result = '\n'.join(account_blocks(code, instructions, True, set()))
        self.assertIn('counter.step(0x1004); counter.charge(1);', result)
        self.assertNotIn('s.step', result)
        self.assertNotIn('s.charge', result)

    def test_control_transfer_ends_a_run(self):
        self.assertTrue(ends_run({'flow': 'UNCONDITIONAL_CALL', 'ops': []}))
        self.assertTrue(ends_run({'flow': 'FALL_THROUGH', 'ops': [['RETURN', None, ['register', '4128', 4]]]}))
        self.assertFalse(ends_run({'flow': 'FALL_THROUGH', 'ops': [['BRANCH', None, ['const', '2', 4]]]}))


@unittest.skipUnless((INPUT / 'instructions.sqlite').exists(), 'Needs the private export')
class ExportedPages(unittest.TestCase):
    def test_every_entry_charges_exactly_the_rest_of_its_run(self):
        userops = json.loads((INPUT / 'program.json').read_text())['userops']
        with sqlite3.connect(f"file:{INPUT / 'instructions.sqlite'}?mode=ro", uri=True) as db:
            for page in (0x100c7, 0x10363, 0x1071a, 0x10768, 0x10343, 0x10147):
                records = [json.loads(row[0]) for row in db.execute(
                    'SELECT body FROM instructions WHERE address>=? AND address<? ORDER BY address',
                    (page << 12, (page + 1) << 12))]
                selected = {r['address'] for r in records}
                code = generate_page(page, records, selected, userops=userops, block_accounting=True).split('\n')
                switch, label = charges(code)
                addresses = [r['address'] for r in records]
                self.assertEqual(set(switch) | set(label), set(addresses), f'{page:x}: an instruction has no entry charge')
                self.assertFalse(set(switch) & set(label), f'{page:x}: a leader is also charged in the switch')
                # Entering at an address charges it and everything after it up
                # to the next leader; a run never continues past a transfer.
                for n, address in enumerate(addresses):
                    run = 1
                    while (n + run < len(addresses) and addresses[n + run] not in label and
                           addresses[n + run] == addresses[n + run - 1] + 4 and not ends_run(records[n + run - 1])):
                        run += 1
                    self.assertEqual(switch.get(address, label.get(address)), run, f'{address:x}')
                self.assertEqual(sum(label.values()), len(addresses), f'{page:x}: runs do not cover the page once')
                # Host-watched instructions and those with code after their body run alone.
                for address in HOST_OBSERVED_INSTRUCTIONS & selected:
                    self.assertEqual(label.get(address), 1, f'{address:x} does not run alone')
                if page == 0x10147:
                    self.assertEqual(label.get(0x10147cf8), 1, 'The speed getter does not run alone')
                # A goto other than the fall-through lands on a leader.
                current = None
                for line in code:
                    if match := LABEL.match(line):
                        current = int(match.group(1), 16)
                    elif current is not None:
                        for target in map(lambda t: int(t, 16), re.findall(r'goto i_([0-9a-f]+);', line)):
                            if target != current + 4 and target in selected:
                                self.assertIn(target, label, f'{current:x} jumps into the middle of a run at {target:x}')


@unittest.skipUnless((GENERATED / 'program.cpp').exists(), 'Needs generated pages')
class GeneratedPages(unittest.TestCase):
    def test_every_observed_instruction_runs_alone_in_the_current_pages(self):
        """Fails when host code or a check starts watching an exact PC and the
        pages were not regenerated (tools/dev.sh generate)."""
        checked = 0
        for address in sorted(HOST_OBSERVED_INSTRUCTIONS):
            page = GENERATED / f'page_{address >> 12:x}.cpp'
            if not page.exists():
                continue
            code = page.read_text().split('\n')
            switch, label = charges(code)
            if address not in switch and address not in label:
                continue  # not an instruction
            checked += 1
            self.assertEqual(label.get(address), 1, f'{address:x} does not run alone; regenerate the pages')
            if address + 4 in switch or address + 4 in label:
                self.assertIn(address + 4, label, f'{address:x} shares a run with the next instruction')
        self.assertGreater(checked, 50)


if __name__ == '__main__':
    unittest.main()
