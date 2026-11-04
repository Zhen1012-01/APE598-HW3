#!/usr/bin/env python3
import hashlib, os, pathlib, struct, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
sizes=[1,2,3,4,5,10,31,101,201]
rows=[]
for n in sizes:
    subprocess.run([str(ROOT/"baseline/sandpile_baseline.exe"),str(n)],cwd=ROOT,check=True,stdout=subprocess.DEVNULL)
    base=hashlib.sha256((ROOT/"identity.png").read_bytes()).hexdigest()
    dump=ROOT/"tests"/f"grid_{n}.bin"
    env=os.environ.copy(); env["SANDPILE_DUMP"]=str(dump)
    subprocess.run([str(ROOT/"sandpile.exe"),str(n)],cwd=ROOT,check=True,stdout=subprocess.DEVNULL,env=env)
    opt=hashlib.sha256((ROOT/"identity.png").read_bytes()).hexdigest()
    if base != opt: raise SystemExit(f"mismatch at N={n}")
    data=struct.unpack(f"={n*n}i",dump.read_bytes())
    r=n//2
    for i in range(n):
        maxj=max(i,n-1-i)
        for j in range(n):
            x=i-r; y=j-r
            inside=j<=maxj and x*x+y*y<=r*r
            if inside and not (0 <= data[i*n+j] <= 3):
                raise SystemExit(f"unstable domain cell N={n} ({i},{j})={data[i*n+j]}")
    dump.unlink()
    rows.append((n,base))
out=ROOT/"results"/"correctness_hashes.csv"
out.write_text("N,sha256\n"+"".join(f"{n},{h}\n" for n,h in rows))
print(f"PASS exact PNG and domain stability for {len(sizes)} sizes: {sizes}")
