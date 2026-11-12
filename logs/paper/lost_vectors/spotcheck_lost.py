#!/usr/bin/env python3
# spotcheck_lost.py -- evaluates LOST(B,F,t) for lines "t B F" with the function lost() of the second
# verifier of the archive (verify/spotcheck.py), separately for the patterns E2 and E3.
#   python3 logs/paper/lost_vectors/spotcheck_lost.py logs/paper/lost_vectors/vectors.txt
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "verify"))
import spotcheck as sc
for line in open(sys.argv[1]):
    if not line.strip():
        continue
    t, B, F = line.split()
    t = int(t)
    Bm = sc.mask(sc.parse_set(B)[0])
    Fm = sc.mask(sc.parse_set(F)[0])
    e2 = bool(sc.lost(Bm, Fm, t, e2=True, e3=False))
    e3 = bool(sc.lost(Bm, Fm, t, e2=False, e3=True))
    print("%d %s %s e2=%d e3=%d lost=%s" % (t, B, F, e2, e3, "true" if (e2 or e3) else "false"))
