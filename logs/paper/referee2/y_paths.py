#!/usr/bin/env python3
# referee2 checker Y: every file path printed in the paper sources (\fn, \path, \texttt, \code) must exist in the unpacked archive
import re,sys,os,glob
root=sys.argv[1]
allfiles=[]
for d,_,fs in os.walk(root):
    for f in fs: allfiles.append(os.path.relpath(os.path.join(d,f),root))
base={}
for f in allfiles: base.setdefault(os.path.basename(f),[]).append(f)
seen={}
for tex in sorted(glob.glob('/home/user/ttt6/paper/*.tex')):
    for i,line in enumerate(open(tex),1):
        if line.lstrip().startswith('%'): continue
        for m in re.finditer(r'\\(fn|path|texttt|code)\{([^{}]*)\}',line):
            s=m.group(2).replace('\\_','_').strip()
            seen.setdefault(s,[]).append('%s:%d'%(os.path.basename(tex),i))
ok=miss=0
for s,where in sorted(seen.items()):
    if not re.search(r'[./]',s) or s.startswith('--') or ' ' in s and '/' not in s: 
        if not re.search(r'\.(cpp|py|md|json|jsonl|log|txt|sh|hpp|tbl|sha256|gz|out)$',s): continue
    if s.startswith('"') or '=' in s: continue
    cand=s.rstrip('/')
    if os.path.exists(os.path.join(root,cand)) : ok+=1; continue
    if glob.glob(os.path.join(root,cand)): ok+=1; continue
    b=os.path.basename(cand)
    if b in base: print('BASENAME-ONLY %-45s -> %s   [%s]'%(s,base[b][:2],where[0])); continue
    print('MISSING       %-45s [%s]'%(s,', '.join(where[:3]))); miss+=1
print('ok',ok,'missing',miss)
