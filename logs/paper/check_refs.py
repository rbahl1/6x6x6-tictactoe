#!/usr/bin/env python3
"""Cross-reference check of the paper (phase 2 of the revision).

Reads paper/paper.tex and every file it inputs (recursively) and checks that
  * every target of \\ref, \\eqref, \\pageref, \\cref, \\Cref, \\crefrange exists as a \\label or \\nllabel,
  * no label is defined twice,
  * every \\cite key is an entry of paper/refs.bib,
  * every file in paper/*.tex is input exactly once (no orphaned source file).
It also lists, for information, labels that are never referenced.
Usage:  python3 logs/paper/check_refs.py [paper-directory]      exit code 0 = all checks passed
"""
import pathlib, re, sys

root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parents[2] / 'paper')

def strip_comments(s):
    return '\n'.join(re.sub(r'(?<!\\)%.*', '', line) for line in s.split('\n'))

order, text = [], {}
def load(name):
    p = root / (name if name.endswith('.tex') else name + '.tex')
    if not p.exists():
        print(f'ERROR: \\input file not found: {p.name}'); sys.exit(1)
    if p.name in text:
        print(f'ERROR: {p.name} is input more than once'); sys.exit(1)
    s = strip_comments(p.read_text())
    text[p.name] = s
    order.append(p.name)
    for m in re.finditer(r'\\input\{([^}]+)\}', s):
        load(m.group(1))
load('paper.tex')

labels, dup = {}, []
for f in order:
    for m in re.finditer(r'\\(?:nl)?label\{([^}]+)\}', text[f]):
        k = m.group(1)
        if k in labels:
            dup.append((k, labels[k], f))
        labels[k] = f

refs = []
for f in order:
    for m in re.finditer(r'\\(?:[cC]ref|ref|eqref|pageref|autoref|nameref)\*?\{([^}]+)\}', text[f]):
        for k in m.group(1).split(','):
            refs.append((k.strip(), f))
    for m in re.finditer(r'\\[cC]refrange\{([^}]+)\}\{([^}]+)\}', text[f]):
        refs.append((m.group(1).strip(), f)); refs.append((m.group(2).strip(), f))
missing = sorted({(k, f) for k, f in refs if k not in labels})

bib = (root / 'refs.bib').read_text()
bibkeys = set(re.findall(r'@\w+\{\s*([^,\s]+)\s*,', bib))
cites = []
for f in order:
    for m in re.finditer(r'\\cite(?:\[[^\]]*\])*\{([^}]+)\}', text[f]):
        for k in m.group(1).split(','):
            cites.append((k.strip(), f))
badcite = sorted({(k, f) for k, f in cites if k not in bibkeys})

orphans = sorted(p.name for p in root.glob('*.tex') if p.name not in text)
used = {k for k, _ in refs}
unused = sorted(k for k in labels if k not in used)

print(f'files input (in order): {len(order)}: ' + ' '.join(order))
print(f'labels: {len(labels)}   references: {len(refs)} to {len(used)} distinct targets   '
      f'citations: {len(cites)} of {len(set(k for k, _ in cites))} distinct keys ({len(bibkeys)} entries in refs.bib)')
ok = True
for k, f1, f2 in dup:
    ok = False; print(f'DUPLICATE LABEL {k}: {f1} and {f2}')
for k, f in missing:
    ok = False; print(f'UNDEFINED REFERENCE {k} in {f}')
for k, f in badcite:
    ok = False; print(f'UNDEFINED CITATION {k} in {f}')
for n in orphans:
    ok = False; print(f'SOURCE FILE NOT INPUT: {n}')
print(f'labels never referenced ({len(unused)}): ' + ' '.join(unused))
print('RESULT: ' + ('all checks passed' if ok else 'FAILED'))
sys.exit(0 if ok else 1)
