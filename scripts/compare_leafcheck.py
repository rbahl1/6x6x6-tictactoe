#!/usr/bin/env python3
"""Compare two LEAF implementations (bin/phicheck and bin/treecheck --leafcheck) line by line.
usage: compare_leafcheck.py A.jsonl B.jsonl [--expect-all-true]   -> prints one JSON line, exit 0 iff no difference"""
import json, sys
from collections import Counter
a = [json.loads(l) for l in open(sys.argv[1])]
b = [json.loads(l) for l in open(sys.argv[2])]
diff = [i for i, (x, y) in enumerate(zip(a, b)) if x['leaf'] != y['leaf'] or x['reason'] != y['reason']]
ok = len(a) == len(b) and not diff
if '--expect-all-true' in sys.argv: ok = ok and all(x['leaf'] for x in a)
print(json.dumps({"ok": ok, "instances": len(a), "leaf_true": sum(x['leaf'] for x in a), "differences": len(diff),
                  "reasons": dict(sorted(Counter(str(x['reason']) for x in a).items()))}))
sys.exit(0 if ok else 1)
