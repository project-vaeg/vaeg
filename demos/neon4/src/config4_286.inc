; Copyright (c) 2026 Nakata Maho
; Ported By Maho Nakata.
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the BSD 2-Clause conditions apply.

; ---------------------------------------------------------------------------
; CONFIG4_286.INC - NEON4 Intel 80286 analog 16-colour edition
; Logical scene coordinates stay 640x400. Default physical mode is 640x200;
; /400 selects 640x400. The scene renderer itself is shared with the 16-colour
; edition so no visual element is removed for the 286 build.
; ---------------------------------------------------------------------------
%include "config4_256.inc"
%define NEON4_LOW_COLOR 1
%define NEON3_SURFACE_AUX_DOSMEM 0
%define NEON4_286 1

; EGC availability is determined by the functional VRAM-copy probe.
; A genuine failure keeps the conservative GRCG fallback.
; Default 200-line mode emits one polygon/solid-rect fill span per physical row.
; Comment this out for full 400-logical-scanline rasterisation even in /200.
%define N4_286_200_SCANLINE_THIN 1
%define VIDEO_H 200

; RASTER TRANSFER low-colour cache layout. 640x400 consumes 32000 bytes
; of each 32 KiB planar page, leaving exactly 768 bytes. CAT (48*6=288)
; plus raster (40*12=480) fills that tail exactly.
%define LOW_EGC_GUARD_OFFSET          31998
%define LOW_EGC_CAT_BASE              32000
%define LOW_EGC_CAT_ROW_BYTES             6
%define LOW_EGC_CAT_BYTES               288
%define LOW_EGC_RASTER_BASE           32288
%define LOW_EGC_RASTER_ROW_BYTES         12
%define LOW_EGC_RASTER_BYTES            480
%define LOW_EGC_CACHE_END             32768
; In 200-line mode the lower half of each 32 KiB page is off-screen, so cache
; exact precomposed CAT variants for even/odd destination Y.  This preserves the
; old fill -> frame -> image ordering while halving CAT row copies.
%define LOW_EGC_CAT200_EVEN_BASE       16000
%define LOW_EGC_CAT200_EVEN_ROWS          24
%define LOW_EGC_CAT200_ODD_BASE        16144
%define LOW_EGC_CAT200_ODD_ROWS           25
%define LOW_EGC_CAT200_END             16294
