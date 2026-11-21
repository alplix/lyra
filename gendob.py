# 11'li (merkez) baslangic x icin, 27'lik desene gore uyumlu kalanlar x mod 37#
import sys
pt=[0,6,12,36,90,96,102,120,132,162,180,186,210,216,222,246,252,270,300,312,330,336,342,396,420,426,432]
primes=[2,3,5,7,11,13,17,19,23,29,31,37]
OFF=132
res=[0];M=1
for p in primes:
    bad={(OFF-a)%p for a in pt}
    ok=[r for r in range(p) if r not in bad]
    inv=pow(M,-1,p)
    new=[]
    for r in res:
        for o in ok:
            # x = r + M*t, x%p=o
            t=((o-r)*inv)%p
            new.append(r+M*t)
    res=new;M*=p
    print(p,len(ok),len(res),file=sys.stderr)
res.sort()
assert M==7420738134810
import struct
open('dob11.bin','wb').write(b''.join(struct.pack('<Q',r) for r in res))
