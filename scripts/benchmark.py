#!/usr/bin/env python3
import argparse,csv,hashlib,json,pathlib,statistics,subprocess,time
P=pathlib.Path(__file__).resolve().parents[1]
a=argparse.ArgumentParser();a.add_argument("--n",type=int,default=401);a.add_argument("--repeats",type=int,default=3);a.add_argument("--variants",nargs="+",default=["baseline","optimized"]);a.add_argument("--out",default="results/reproduction");x=a.parse_args()
bins={"baseline":P/"baseline/sandpile_baseline.exe","optimized":P/"sandpile.exe","batch":P/"sandpile_batch.exe","queue":P/"sandpile_queue.exe","forward":P/"sandpile_forward.exe"};rows=[]
for name in x.variants:
 for run in range(1,x.repeats+1):
  t=time.perf_counter();subprocess.run([str(bins[name]),str(x.n)],cwd=P,check=True,stdout=subprocess.DEVNULL);dt=time.perf_counter()-t;h=hashlib.sha256((P/"identity.png").read_bytes()).hexdigest();rows.append({"N":x.n,"variant":name,"run":run,"wall_seconds":dt,"sha256":h})
out=P/x.out;out.parent.mkdir(parents=True,exist_ok=True)
with out.with_suffix(".csv").open("w",newline="") as f:w=csv.DictWriter(f,fieldnames=rows[0]);w.writeheader();w.writerows(rows)
s={v:{"median_seconds":statistics.median([r["wall_seconds"] for r in rows if r["variant"]==v]),"hashes":sorted(set(r["sha256"] for r in rows if r["variant"]==v))} for v in x.variants};out.with_suffix(".json").write_text(json.dumps(s,indent=2));print(json.dumps(s,indent=2))
