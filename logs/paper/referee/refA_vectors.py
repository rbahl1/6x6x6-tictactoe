# refA_vectors.py -- referee 1a: the LEAF test vectors of Table 11 (Appendix A.6) with my own
# literal implementation of Algorithm 2 (refA_lib.leaf_alg2), and the L1 exit.
import sys, time
from refA_geom import *
from refA_lib import *
t0 = time.time()
T7 = [0,46,92,123,169,180,215]; T8 = [0,35,46,92,123,169,180,215]
V = [(7, T7, [], (False, 'L3')), (8, T8, [], (True, '')), (9, T8, [], (False, 'L3'))]
for h in [5, 30, 43, 61, 64, 86, 87, 93, 122, 128, 129, 151, 154, 172, 185, 210]:
    V.append((8, T8, [h], (True, '')))
V += [(8, [0,35,43,46,92,123,169,180], [215], (True, '')),
      (8, [0,5,35,43,46,61,86,92], [123,129,169,172,180,210,215], (False, 'L2')),
      (9, [0,5,35,43,46,61,86,87,92], [123,129,154,169,172,180,210,215], (True, '')),
      (10, [0,5,30,35,43,46,61,86,87,92], [123,128,129,154,169,172,180,210,215], (True, '')),
      (9, [0,8,35,46,92,123,169,180,215], [44,80,116,152,188], (True, '')),
      (8, [0,8,46,92,123,169,180,215], [44,80,116,152,188], (False, 'L3')),
      (0, [], [], (False, 'L2'))]
bad = 0
for k, B, F, want in V:
    got = leaf_alg2(mask(B), mask(F), k, want_reason=True)
    if got != want:
        bad += 1
    print(k, B, F, '->', got, 'paper:', want, '' if got == want else '  <-- MISMATCH'); sys.stdout.flush()
# L1 exit: five cells of a live line
print('L1:', leaf_alg2(0, mask([0, 1, 2, 3, 4]), 5, want_reason=True), leaf_alg2(mask([5]), mask([0, 1, 2, 3, 4]), 5, want_reason=True))
print('mismatches:', bad, ' (%.1f s)' % (time.time() - t0))
