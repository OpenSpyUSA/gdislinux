# GDIS Test Examples

These files are copied from the upstream `models/` directory into a smaller
set for quick smoke testing.

Recommended first tests:

- `methane.gin`: very small GULP input, fast first launch test
- `water.car`: simple molecule in BIOSYM/CAR format
- `adp1.cif`: crystal structure CIF example
- `deoxy.pdb`: PDB-style molecule/protein example
- `calc2d.xtl`: compact XTL example
- `vasprun.xml`: larger VASP XML output example

Quick start:

```bash
cd /home/hym/Desktop/gdislinux
./run-example.sh methane
```

You can also open a file directly:

```bash
cd /home/hym/Desktop/gdislinux
./bin/gdis examples/adp1.cif
```
