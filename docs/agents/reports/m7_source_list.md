# M7 source list

Source oracle: `sdl/Makefile.zau`, `NP2OBJ`.

M7 builds only the assembly-free PC-98 core libraries requested by
`docs/agents/tasks/M7_cmake_core.md`: `vaeg_common` and `vaeg_core`.
The SDL1 frontend and embedded menu objects present in `NP2OBJ` are
recorded below but are not built in M7 because this milestone has no
executable and no SDL target.

## Built by M7

| File | Target lib | Origin |
|---|---|---|
| common/strres.c | vaeg_common | sdl/Makefile.zau NP2OBJ `strres.o` |
| common/milstr.c | vaeg_common | sdl/Makefile.zau NP2OBJ `milstr.o` |
| common/_memory.c | vaeg_common | sdl/Makefile.zau NP2OBJ `_memory.o` |
| common/textfile.c | vaeg_common | sdl/Makefile.zau NP2OBJ `textfile.o` |
| common/profile.c | vaeg_common | sdl/Makefile.zau NP2OBJ `profile.o` |
| common/rect.c | vaeg_common | sdl/Makefile.zau NP2OBJ `rect.o` |
| common/lstarray.c | vaeg_common | sdl/Makefile.zau NP2OBJ `lstarray.o` |
| common/bmpdata.c | vaeg_common | sdl/Makefile.zau NP2OBJ `bmpdata.o` |
| common/codecnv.c | vaeg_common | sdl/Makefile.zau NP2OBJ `codecnv.o` |
| common/parts.c | vaeg_common | sdl/Makefile.zau NP2OBJ `parts.o` |
| common/resize.c | vaeg_common | sdl/Makefile.zau NP2OBJ `resize.o` |
| generic/cmjasts.c | vaeg_common | sdl/Makefile.zau NP2OBJ `cmjasts.o` |
| generic/cmver.c | vaeg_common | sdl/Makefile.zau NP2OBJ `cmver.o` |
| i286c/i286c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c.o` |
| i286c/i286c_mn.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_mn.o` |
| i286c/i286c_ea.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_ea.o` |
| i286c/i286c_0f.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_0f.o` |
| i286c/i286c_8x.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_8x.o` |
| i286c/i286c_sf.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_sf.o` |
| i286c/i286c_f6.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_f6.o` |
| i286c/i286c_fe.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_fe.o` |
| i286c/i286c_rp.c | vaeg_core | sdl/Makefile.zau NP2OBJ `i286c_rp.o` |
| i286c/v30patch.c | vaeg_core | sdl/Makefile.zau NP2OBJ `v30patch.o` |
| i286c/memory.c | vaeg_core | sdl/Makefile.zau NP2OBJ `memory.o` |
| i286c/egcmem.c | vaeg_core | sdl/Makefile.zau NP2OBJ `egcmem.o` |
| i286c/dmap.c | vaeg_core | sdl/Makefile.zau NP2OBJ `dmap.o` |
| io/iocore.c | vaeg_core | sdl/Makefile.zau NP2OBJ `iocore.o` |
| io/artic.c | vaeg_core | sdl/Makefile.zau NP2OBJ `artic.o` |
| io/cgrom.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cgrom.o` |
| io/cpuio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cpuio.o` |
| io/crtc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `crtc.o` |
| io/dipsw.c | vaeg_core | sdl/Makefile.zau NP2OBJ `dipsw.o` |
| io/dmac.c | vaeg_core | sdl/Makefile.zau NP2OBJ `dmac.o` |
| io/egc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `egc.o` |
| io/epsonio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `epsonio.o` |
| io/emsio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `emsio.o` |
| io/fdc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fdc.o` |
| io/fdd320.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fdd320.o` |
| io/gdc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gdc.o` |
| io/gdc_sub.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gdc_sub.o` |
| io/gdc_pset.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gdc_pset.o` |
| io/mouseif.c | vaeg_core | sdl/Makefile.zau NP2OBJ `mouseif.o` |
| io/necio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `necio.o` |
| io/nmiio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `nmiio.o` |
| io/np2sysp.c | vaeg_core | sdl/Makefile.zau NP2OBJ `np2sysp.o` |
| io/pic.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pic.o` |
| io/pit.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pit.o` |
| io/printif.c | vaeg_core | sdl/Makefile.zau NP2OBJ `printif.o` |
| io/serial.c | vaeg_core | sdl/Makefile.zau NP2OBJ `serial.o` |
| io/sysport.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sysport.o` |
| io/upd4990.c | vaeg_core | sdl/Makefile.zau NP2OBJ `upd4990.o` |
| cbus/cbuscore.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cbuscore.o` |
| cbus/sasiio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sasiio.o` |
| cbus/scsiio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `scsiio.o` |
| cbus/scsicmd.c | vaeg_core | sdl/Makefile.zau NP2OBJ `scsicmd.o` |
| cbus/pc9861k.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pc9861k.o` |
| cbus/mpu98ii.c | vaeg_core | sdl/Makefile.zau NP2OBJ `mpu98ii.o` |
| cbus/board14.c | vaeg_core | sdl/Makefile.zau NP2OBJ `board14.o` |
| cbus/board26k.c | vaeg_core | sdl/Makefile.zau NP2OBJ `board26k.o` |
| cbus/board86.c | vaeg_core | sdl/Makefile.zau NP2OBJ `board86.o` |
| cbus/boardx2.c | vaeg_core | sdl/Makefile.zau NP2OBJ `boardx2.o` |
| cbus/board118.c | vaeg_core | sdl/Makefile.zau NP2OBJ `board118.o` |
| cbus/boardspb.c | vaeg_core | sdl/Makefile.zau NP2OBJ `boardspb.o` |
| cbus/amd98.c | vaeg_core | sdl/Makefile.zau NP2OBJ `amd98.o` |
| cbus/pcm86io.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pcm86io.o` |
| cbus/cs4231io.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cs4231io.o` |
| sound/vermouth/midiout.c | vaeg_core | sdl/Makefile.zau NP2OBJ `midiout.o` |
| sound/vermouth/midimod.c | vaeg_core | sdl/Makefile.zau NP2OBJ `midimod.o` |
| sound/vermouth/midinst.c | vaeg_core | sdl/Makefile.zau NP2OBJ `midinst.o` |
| sound/vermouth/midvoice.c | vaeg_core | sdl/Makefile.zau NP2OBJ `midvoice.o` |
| sound/vermouth/midtable.c | vaeg_core | sdl/Makefile.zau NP2OBJ `midtable.o` |
| sound/getsnd/getsnd.c | vaeg_core | sdl/Makefile.zau NP2OBJ `getsnd.o` |
| sound/getsnd/getwave.c | vaeg_core | sdl/Makefile.zau NP2OBJ `getwave.o` |
| sound/getsnd/getsmix.c | vaeg_core | sdl/Makefile.zau NP2OBJ `getsmix.o` |
| bios/bios.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios.o` |
| bios/bios09.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios09.o` |
| bios/bios0c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios0c.o` |
| bios/bios12.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios12.o` |
| bios/bios13.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios13.o` |
| bios/bios18.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios18.o` |
| bios/bios19.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios19.o` |
| bios/bios1a.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios1a.o` |
| bios/bios1b.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios1b.o` |
| bios/bios1c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios1c.o` |
| bios/bios1f.c | vaeg_core | sdl/Makefile.zau NP2OBJ `bios1f.o` |
| bios/sxsibios.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sxsibios.o` |
| sound/sound.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sound.o` |
| sound/sndcsec.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sndcsec.o` |
| sound/fmboard.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fmboard.o` |
| sound/fmtimer.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fmtimer.o` |
| sound/beepc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `beepc.o` |
| sound/beepg.c | vaeg_core | sdl/Makefile.zau NP2OBJ `beepg.o` |
| sound/tms3631c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `tms3631c.o` |
| sound/tms3631g.c | vaeg_core | sdl/Makefile.zau NP2OBJ `tms3631g.o` |
| sound/opngenc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `opngenc.o` |
| sound/opngeng.c | vaeg_core | sdl/Makefile.zau NP2OBJ `opngeng.o` |
| sound/psggenc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `psggenc.o` |
| sound/psggeng.c | vaeg_core | sdl/Makefile.zau NP2OBJ `psggeng.o` |
| sound/rhythmc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `rhythmc.o` |
| sound/adpcmc.c | vaeg_core | sdl/Makefile.zau NP2OBJ `adpcmc.o` |
| sound/adpcmg.c | vaeg_core | sdl/Makefile.zau NP2OBJ `adpcmg.o` |
| sound/pcm86c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pcm86c.o` |
| sound/pcm86g.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pcm86g.o` |
| sound/cs4231c.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cs4231c.o` |
| sound/cs4231g.c | vaeg_core | sdl/Makefile.zau NP2OBJ `cs4231g.o` |
| sound/soundrom.c | vaeg_core | sdl/Makefile.zau NP2OBJ `soundrom.o` |
| sound/s98.c | vaeg_core | sdl/Makefile.zau NP2OBJ `s98.o` |
| vram/vram.c | vaeg_core | sdl/Makefile.zau NP2OBJ `vram.o` |
| vram/scrndraw.c | vaeg_core | sdl/Makefile.zau NP2OBJ `scrndraw.o` |
| vram/sdraw.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sdraw.o` |
| vram/sdrawq16.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sdrawq16.o` |
| vram/dispsync.c | vaeg_core | sdl/Makefile.zau NP2OBJ `dispsync.o` |
| vram/palettes.c | vaeg_core | sdl/Makefile.zau NP2OBJ `palettes.o` |
| vram/maketext.c | vaeg_core | sdl/Makefile.zau NP2OBJ `maketext.o` |
| vram/maketgrp.c | vaeg_core | sdl/Makefile.zau NP2OBJ `maketgrp.o` |
| vram/makegrph.c | vaeg_core | sdl/Makefile.zau NP2OBJ `makegrph.o` |
| vram/scrnbmp.c | vaeg_core | sdl/Makefile.zau NP2OBJ `scrnbmp.o` |
| fdd/diskdrv.c | vaeg_core | sdl/Makefile.zau NP2OBJ `diskdrv.o` |
| fdd/newdisk.c | vaeg_core | sdl/Makefile.zau NP2OBJ `newdisk.o` |
| fdd/fddfile.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fddfile.o` |
| fdd/fdd_xdf.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fdd_xdf.o` |
| fdd/fdd_d88.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fdd_d88.o` |
| fdd/fdd_mtr.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fdd_mtr.o` |
| fdd/sxsi.c | vaeg_core | sdl/Makefile.zau NP2OBJ `sxsi.o` |
| lio/lio.c | vaeg_core | sdl/Makefile.zau NP2OBJ `lio.o` |
| lio/gscreen.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gscreen.o` |
| lio/gpset.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gpset.o` |
| lio/gline.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gline.o` |
| lio/gcircle.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gcircle.o` |
| lio/gput1.c | vaeg_core | sdl/Makefile.zau NP2OBJ `gput1.o` |
| font/font.c | vaeg_core | sdl/Makefile.zau NP2OBJ `font.o` |
| font/fontdata.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontdata.o` |
| font/fontpc88.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontpc88.o` |
| font/fontpc98.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontpc98.o` |
| font/fontv98.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontv98.o` |
| font/fontfm7.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontfm7.o` |
| font/fontx1.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontx1.o` |
| font/fontx68k.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontx68k.o` |
| font/fontmake.c | vaeg_core | sdl/Makefile.zau NP2OBJ `fontmake.o` |
| pccore.c | vaeg_core | sdl/Makefile.zau NP2OBJ `pccore.o` |
| nevent.c | vaeg_core | sdl/Makefile.zau NP2OBJ `nevent.o` |
| calendar.c | vaeg_core | sdl/Makefile.zau NP2OBJ `calendar.o` |
| timing.c | vaeg_core | sdl/Makefile.zau NP2OBJ `timing.o` |
| keystat.c | vaeg_core | sdl/Makefile.zau NP2OBJ `keystat.o` |
| statsave.c | vaeg_core | sdl/Makefile.zau NP2OBJ `statsave.o` |
| debugsub.c | vaeg_core | sdl/Makefile.zau NP2OBJ `debugsub.o` |

## Not built in M7

These `sdl/Makefile.zau` objects are SDL1 frontend or embedded menu
objects. M7 records them but does not create an executable target.

| Object | Reason |
|---|---|
| vramhdl.o | embed/menu helper; deferred to SDL2 frontend work |
| vrammix.o | embed/menu helper; deferred to SDL2 frontend work |
| menustr.o | embed/menu helper; deferred to SDL2 frontend work |
| filesel.o | embed/menu helper; deferred to SDL2 frontend work |
| dlgcfg.o | embed/menu helper; deferred to SDL2 frontend work |
| dlgscr.o | embed/menu helper; deferred to SDL2 frontend work |
| dlgabout.o | embed/menu helper; deferred to SDL2 frontend work |
| menubase.o | embed/menu helper; deferred to SDL2 frontend work |
| menuvram.o | embed/menu helper; deferred to SDL2 frontend work |
| menuicon.o | embed/menu helper; deferred to SDL2 frontend work |
| menusys.o | embed/menu helper; deferred to SDL2 frontend work |
| menudlg.o | embed/menu helper; deferred to SDL2 frontend work |
| menumbox.o | embed/menu helper; deferred to SDL2 frontend work |
| menures.o | embed/menu helper; deferred to SDL2 frontend work |
| np2.o | SDL1 frontend; M7 has no executable |
| dosio.o | SDL1 frontend; M7 has no executable |
| trace.o | SDL1 frontend; M7 has no executable |
| commng.o | SDL1 frontend; M7 has no executable |
| fontmng.o | SDL1 frontend; M7 has no executable |
| inputmng.o | SDL1 frontend; M7 has no executable |
| joymng.o | SDL1 frontend; M7 has no executable |
| mousemng.o | SDL1 frontend; M7 has no executable |
| scrnmng.o | SDL1 frontend; M7 has no executable |
| soundmng.o | SDL1 frontend; M7 has no executable |
| sysmng.o | SDL1 frontend; M7 has no executable |
| taskmng.o | SDL1 frontend; M7 has no executable |
| timemng.o | SDL1 frontend; M7 has no executable |
| sdlkbd.o | SDL1 frontend; M7 has no executable |
| ini.o | SDL1 frontend; M7 has no executable |
| sysmenu.o | SDL1 frontend; M7 has no executable |
