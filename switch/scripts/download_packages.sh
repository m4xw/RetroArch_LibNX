#!/bin/sh

if [ -z "$1" ]; then
    echo "Package develop or master?"
    exit 1
fi

cd "$(dirname "$0")"

BRANCH=$1
JOB="package"
if [ $BRANCH = "develop" ]; then
    JOB="nightly"
fi

mkdir tmp_package
cd tmp_package/
wget https://git.m4xw.net/Switch/RetroArch/VBA-Next/-/jobs/artifacts/$BRANCH/download?job=$JOB -O vba-next.zip
wget https://git.m4xw.net/Switch/RetroArch/Genesis-Plus-GX/-/jobs/artifacts/$BRANCH/download?job=$JOB -O genesis_plus_gx.zip
wget https://git.m4xw.net/Switch/RetroArch/snes9x/-/jobs/artifacts/$BRANCH/download?job=$JOB -O snes9x.zip
wget https://git.m4xw.net/Switch/RetroArch/libretro-snes9x2010/-/jobs/artifacts/$BRANCH/download?job=$JOB -O snes9x2010.zip
wget https://git.m4xw.net/Switch/RetroArch/4do-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O 4do.zip
wget https://git.m4xw.net/Switch/RetroArch/virtualjaguar-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O virtualjaguar.zip
wget https://git.m4xw.net/Switch/RetroArch/gambatte-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O gambatte.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-vb-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O beetle-vb.zip
wget https://git.m4xw.net/Switch/RetroArch/stella-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O stella.zip
wget https://git.m4xw.net/Switch/RetroArch/libretro-o2em/-/jobs/artifacts/$BRANCH/download?job=$JOB -O o2em.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-pce-fast-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O beetle-pce-fast.zip
wget https://git.m4xw.net/Switch/RetroArch/libretro-vecx/-/jobs/artifacts/$BRANCH/download?job=$JOB -O vecx.zip
wget https://git.m4xw.net/Switch/RetroArch/nxengine-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O nxengine.zip
wget https://git.m4xw.net/Switch/RetroArch/prosystem-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O prosystem.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-ngp-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O beetle-ngp.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-supergrafx-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O beetle-supergrafx.zip
wget https://git.m4xw.net/Switch/RetroArch/libretro-handy/-/jobs/artifacts/$BRANCH/download?job=$JOB -O handy.zip
wget https://git.m4xw.net/Switch/RetroArch/libretro-fceumm/-/jobs/artifacts/$BRANCH/download?job=$JOB -O fceumm.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-wswan-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O wswan.zip
wget https://git.m4xw.net/Switch/RetroArch/blueMSX-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O bluemsx.zip
wget https://git.m4xw.net/Switch/RetroArch/fbalpha/-/jobs/artifacts/$BRANCH/download?job=$JOB -O fbalpha.zip
wget https://git.m4xw.net/Switch/RetroArch/mgba/-/jobs/artifacts/$BRANCH/download?job=$JOB -O mgba.zip
wget https://git.m4xw.net/Switch/RetroArch/beetle-psx/-/jobs/artifacts/$BRANCH/download?job=$JOB -O beetle-psx.zip
wget https://git.m4xw.net/Switch/RetroArch/mame2003-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O mame2003.zip
wget https://git.m4xw.net/Switch/RetroArch/mame2003-plus-libretro/-/jobs/artifacts/$BRANCH/download?job=$JOB -O mame2003-plus.zip
wget https://git.m4xw.net/Switch/RetroArch/pcsx_rearmed/-/jobs/artifacts/$BRANCH/download?job=$JOB -O pcsx_rearmed.zip
