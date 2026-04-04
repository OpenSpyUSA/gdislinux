set cell    1.889725988    0.000000000    0.000000000    0.000000000    1.889725988    0.000000000    0.000000000    0.000000000    1.889725988
species C    /home/hym/Desktop/gdislinux/.localdeps/qbox-public/test/potentials/C_HSCV_PBE-1.0.xml
species H    /home/hym/Desktop/gdislinux/.localdeps/qbox-public/test/potentials/H_HSCV_PBE-1.0.xml
atom C        C       0.000000000    0.000000000    0.000000000
atom H        H       1.190000000    1.190000000    1.190000000
atom H        H       1.190000000    0.699725988    0.699725988
atom H        H       0.699725988    1.190000000    0.699725988
atom H        H       0.699725988    0.699725988    1.190000000
set ecut 35.0
set xc PBE
randomize_wf
set wf_dyn PSDA
set ecutprec 5.0
save /home/hym/Desktop/gdislinux/tmp/qbox-gui/qbox_methane.xml
quit
