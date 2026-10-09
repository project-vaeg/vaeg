# SGP256S / SGP256T

`SGP256S.COM` is the 8-bpp parallel of the 16-color pseudo-sprite demo. Both
graphics screens use the same logical 320x200 mode:

* Graphic 0 is a CPU-written 8-bpp checkerboard background;
* Graphic 1 is the sprite layer, with two 320x200 hidden/display pages;
* Graphic 1 color zero is transparent, so the background remains visible;
* every frame clears and redraws only the selected hidden Graphic 1 page with
  one SGP command list, then exchanges FB1 DSA at VBLANK; and
* `FPSnnn Cnnnn` is transferred as transparent 8-bpp SGP BITBLTs.

FB1 is configured as a 320x400 backing surface with a 320-byte line pitch and
a 320x200 display window. The two 64,000-byte pages are selected by DSA1:
page A uses SGP address `0220000h` / DSA `0020000h`, and page B uses SGP
address `022fa00h` / DSA `002fa00h`. The logical and displayed geometry is
always 320x200. The 16 HSV sphere bitmaps are generated offline by
`generate_raytrace.py`: each 24x24 pixel traces an orthographic ray against a
sphere and combines ambient, diffuse, and specular lighting before directly
rounding the result to VA 8-bpp `GGGRRRBB` direct color. There is no retained
16-bpp source or startup conversion loop. Zero pixels remain transparent, and
nonzero samples that quantize to zero use a dark neutral fallback so shadow
pixels do not acquire a blue cast.
UP/DOWN (or `+`/`-`) changes the active ball
count from 1 through 128; ESC exits and restores the previous video mode.

Build with:

```sh
NASM=/opt/local/bin/nasm sh demos/sgp-pseudo-sprite/256/build.sh /tmp/SGP256S.COM
```

The generated COM and any D88 test image are disposable artifacts and remain
outside the repository.

Regenerate the deterministic ray-traced source include with:

```sh
python3 demos/sgp-pseudo-sprite/256/generate_raytrace.py \
  demos/sgp-pseudo-sprite/256/orb_raytrace8_24.inc
```

`SGP256T.COM` uses the same sprite and framebuffer path, but redraws the
Graphic 0 checkerboard as three independently phased horizontal bands. The
internal phases advance by 3, 7, and 11 byte units and snap to 16-byte
(16-dot) checker boundaries before each row is written. The sprite count,
FPS/C glyphs, 8-bpp format, and two-page Graphic 1 exchange are unchanged.

Build the scrolling variant with:

```sh
NASM=/opt/local/bin/nasm sh demos/sgp-pseudo-sprite/256/build-scroll.sh /tmp/SGP256T.COM
```
