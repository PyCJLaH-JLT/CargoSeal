#!/usr/bin/env python3
"""Run all host groups against one instrumented copy of the portable modules."""
import json, re, subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[1]
build = root / "tests/coverage"
build.mkdir(parents=True, exist_ok=True)
for f in build.glob("*"):
    if f.is_file(): f.unlink()
modules = ["Sensors/mpu6050", "Sensors/light", "Policy/container", "Policy/commands"]
objects = []
for name in modules:
    obj = build / (Path(name).name + ".o")
    subprocess.run(["gcc","-std=c11","-Wall","-Wextra","-Werror","-O0","-g","--coverage","-c",
                    str(root / ("firmware/" + name + ".c")),"-o",str(obj)],check=True)
    objects.append(str(obj))
for group in ("sensors","light","policy","commands","acceptance"):
    obj=build / ("test_"+group+".o")
    subprocess.run(["gcc","-std=c11","-Wall","-Wextra","-Werror","-g","-Dmain=run_"+group,
                    "-c",str(root / ("tests/test_"+group+".c")),"-o",str(obj)],check=True)
    objects.append(str(obj))
binary=build / "all"
subprocess.run(["gcc","--coverage",str(root/"tests/test_all.c"),*objects,"-lm","-o",str(binary)],check=True)
subprocess.run([str(binary)],check=True)
rows=[]
for module in modules:
    subprocess.run(["gcov","-b","-c","-o",str(build),str(root/("firmware/"+module+".c"))],
                   cwd=build,check=True,stdout=subprocess.PIPE,text=True)
    path=build/(Path(module).name+".c.gcov")
    lines=path.read_text().splitlines()
    executable=covered=branches=taken=0
    for line in lines:
        match=re.match(r"\s*([^:]+):\s*\d+:",line)
        if match and match[1].strip()!="-":
            executable+=1
            if re.fullmatch(r"\d+\*?",match[1].strip()) and int(match[1].strip().rstrip("*"))>0:covered+=1
        b=re.match(r"branch\s+\d+\s+(?:taken\s+(\d+)|never executed)",line)
        if b:
            branches+=1
            if b[1] and int(b[1])>0:taken+=1
    row={"module":module,"lines":covered,"total_lines":executable,"line_pct":100*covered/executable,
         "taken_branches":taken,"total_branches":branches,"branch_pct":100*taken/branches if branches else 100}
    rows.append(row)
    print(f"{module}: lines {covered}/{executable}={row['line_pct']:.2f}%, branch taken {taken}/{branches}={row['branch_pct']:.2f}%")
(build/"summary.json").write_text(json.dumps(rows,indent=2)+"\n")
if any(x["line_pct"]<90 or x["branch_pct"]<80 for x in rows):raise SystemExit("Coverage target not reached")
print("PASS: portable coverage >=90% lines and >=80% branch directions taken")

