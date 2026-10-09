# Demo distribution disks

This directory is the canonical location for reproducible, non-bootable
`.d88.xz` distribution images produced by demo `build-d88.sh` scripts.

The allow-listed distribution images are:

```text
glass-orbit.d88.xz
neon3-distribution.d88.xz
neon4-distribution.d88.xz
sgp-pseudo-sprite.d88.xz
sgp-wireframe.d88.xz
zundamon-orbit.d88.xz
all-demos.d88.xz
```

The builders accept a caller-selected raw D88 output path for local use, but
write the compressed companion here. Raw D88 files, bootable validation disks,
source templates, and private media remain outside the repository.

To make one non-bootable distribution disk containing every demo, use
[`tools/pc88va/build-all-demos-distribution-disk.py`](../../tools/pc88va/build-all-demos-distribution-disk.py):

```sh
python3 tools/pc88va/build-all-demos-distribution-disk.py \
  --source "/path/to/pcengine110-bootonly.d88" \
  --output /private/tmp/vaeg-all-demos.d88
```

This writes the raw image outside Git and the compressed companion as
`demos/disks/all-demos.d88.xz`.  The aggregate archive is built from the six
component distribution archives; it is not an input to the bootable builder.

To make one local bootable disk containing every component distribution, use
[`tools/pc88va/build-all-demos-bootable-disk.py`](../../tools/pc88va/build-all-demos-bootable-disk.py)
with a user-supplied PC-Engine system disk:

```sh
python3 tools/pc88va/build-all-demos-bootable-disk.py \
  --source "/path/to/PC-Engine 1.1.d88" \
  --output /private/tmp/vaeg-all-demos-bootable.d88
```

The builder extracts the six component `.d88.xz` images and installs them as
`A:\GLASS`, `A:\NEON3`, `A:\NEON4\16`, `A:\NEON4\65536`,
`A:\SPRITE\16`, `A:\SPRITE\256`, `A:\SPRITE\65536`, `A:\WIRE\16`,
`A:\WIRE\256`, `A:\WIRE\65536`, and `A:\ZUNDAMON`.
The supplied system disk provides the IPL and boot files, so the result is a
bootable PC-Engine D88. The source and raw output remain local artifacts and
are not committed.
