# Qbox pseudopotentials (XML) for GDIS

This tree supports Qbox species XML paths per element in `Tools > Computation > Qbox > Potentials`.

## Fast path (already generated)

A ready set exists at:

- `external/pseudos/qbox-xml-oncv-sr`

This bundle contains 70 scalar-relativistic ONCV PBE XML files (one per element in the source set):

`Ag Al Ar As Au B Ba Be Bi Br C Ca Cd Cl Co Cr Cs Cu F Fe Ga Ge H He Hf Hg I In Ir K Kr Li Mg Mn Mo N Na Nb Ne Ni O Os P Pb Pd Po Pt Rb Re Rh Rn Ru S Sb Sc Se Si Sn Sr Ta Tc Te Ti Tl V W Xe Y Zn Zr`

Elements not present in this ONCV source set (48 symbols):

`Ac Am At Bh Bk Ce Cf Cm Cn Db Ds Dy Er Es Eu Fl Fm Fr Gd Ho Hs La Lr Lu Lv Mc Md Mt Nd Nh No Np Og Pa Pm Pr Pu Ra Rf Rg Sg Sm Tb Th Tm Ts U Yb`

## Regenerate from UPF files

Use the included converter helper:

```bash
./generate-qbox-xml-from-upf.sh \
  --input-dir /path/to/upf-dir \
  --output-dir /path/to/xml-dir \
  --rcut 8.0
```

Notes:

- Requires Qbox `upf2qso` (auto-built from `.localdeps/qbox-public` if present).
- Only norm-conserving UPF files are supported by `upf2qso`.
- Ultrasoft/PAW UPF files are not convertible with this tool.

## Qbox GUI auto-fill lookup order

`Use Demo Potentials` searches in this order:

1. Legacy bundled test potentials (`.localdeps/qbox-public/test/potentials`) for C/H/O/Si.
2. Custom directory from environment variable `GDIS_QBOX_POTENTIAL_DIR`
3. Packaged demo directory `/usr/share/gdislinux/qbox/potentials`
4. `external/pseudos/qbox-xml-oncv-sr`
5. `external/pseudos/qbox-xml-oncv`

If your XML files live elsewhere, set:

```bash
export GDIS_QBOX_POTENTIAL_DIR=/absolute/path/to/qbox-xml
```
