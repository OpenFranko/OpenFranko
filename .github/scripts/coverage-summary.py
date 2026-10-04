import json
import sys
from collections import defaultdict

summary = json.load(open(sys.argv[1]))
groups = defaultdict(lambda: [0, 0, 0, 0])
for entry in summary['files']:
    parts = entry['filename'].split('/')
    depth = 3 if parts[0] == 'src' and len(parts) > 3 else 2
    group = '/'.join(parts[:min(depth, len(parts) - 1)])
    totals = groups[group]
    totals[0] += entry['line_covered']
    totals[1] += entry['line_total']
    totals[2] += entry['function_covered']
    totals[3] += entry['function_total']


def percent(covered, total):
    return f'{100.0 * covered / total:.1f}%' if total else '-'


print('## Test coverage')
print()
print('| | Lines | Functions | Branches |')
print('|---|---|---|---|')
print(f"| **Total** | {percent(summary['line_covered'], summary['line_total'])}"
      f" ({summary['line_covered']}/{summary['line_total']})"
      f" | {percent(summary['function_covered'], summary['function_total'])}"
      f" | {percent(summary['branch_covered'], summary['branch_total'])} |")
print()
print('| Component | Lines | Functions |')
print('|---|---|---|')
for group in sorted(groups):
    lines, lineTotal, functions, functionTotal = groups[group]
    print(f'| {group} | {percent(lines, lineTotal)} ({lines}/{lineTotal})'
          f' | {percent(functions, functionTotal)} |')
