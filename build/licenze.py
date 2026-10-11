import os, re, tarfile
SRC='/mnt/lfs/sources'
OUT='/root/magi-overlay/usr/share/licenses'
EXT=re.compile(r'\.(tar\.(gz|xz|bz2)|tgz|tbz2|txz)$')
PAT=re.compile(r'^(COPYING|LICENSE|LICENCE|COPYRIGHT|UNLICENSE)([._-].*)?$',re.I)
TIPI=[('AGPL','GNU AFFERO'),('LGPL','GNU LESSER GENERAL PUBLIC'),('LGPL','GNU LIBRARY GENERAL PUBLIC'),('MPL','Mozilla Public'),('Apache-2.0','Apache License'),('MIT','Permission is hereby granted, free of charge'),('BSD','Redistribution and use in source and binary forms'),('ISC','Permission to use, copy, modify, and/or distribute')]
TIPI+=[("zlib","as-is"),("HPND","sell this software"),("libpng","PNG Reference Library"),("FTL","FreeType License"),("Vim","VIM LICENSE"),("Tcl","copyrighted by the Regents"),("AFL","Academic Free License"),("curl","COPYRIGHT AND PERMISSION NOTICE"),("lsof","Purdue University"),("Perl","Artistic License"),("PD","public domain")]
def tipi(t):
    t=" ".join(t.split())
    r=[]
    if 'GNU GENERAL PUBLIC LICENSE' in t:
        r.append('GPL-3' if 'Version 3, 29 June 2007' in t else 'GPL-2')
    for n,k in TIPI:
        if k in t and n not in r: r.append(n)
    return r
righe=[]
for f in sorted(os.listdir(SRC)):
    if not EXT.search(f): continue
    nome=EXT.sub('',f)
    found={}
    try:
        with tarfile.open(os.path.join(SRC,f)) as tf:
            for m in tf:
                p=m.name.split('/')
                if m.isfile() and len(p)<=2 and PAT.match(p[-1]) and m.size<200000:
                    found[p[-1]]=tf.extractfile(m).read()
    except Exception as e:
        righe.append(nome+': ERRORE '+str(e)[:40])
        continue
    t=''.join(v.decode('utf-8','replace')[:30000] for v in found.values())
    tp=tipi(t)
    righe.append('%s: %s%s' % (nome, '/'.join(tp) if tp else 'DA VERIFICARE', '' if found else ' [nessun file di licenza]'))
    if found:
        d=os.path.join(OUT,nome)
        os.makedirs(d,exist_ok=True)
        for k,v in found.items():
            open(os.path.join(d,k),'wb').write(v)
os.makedirs(OUT,exist_ok=True)
open(os.path.join(OUT,'LICENZE.txt'),'w').write('Maginat - licenze dei pacchetti (rilevate automaticamente, da verificare)\n\n'+'\n'.join(righe)+'\n')
print(len(righe))
print(sum(1 for r in righe if 'DA VERIFICARE' in r or 'ERRORE' in r))
