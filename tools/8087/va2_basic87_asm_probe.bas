1 ' Copyright (c) 2026 Nakata Maho
2 ' Redistribution and use in source and binary forms, with or without
3 ' modification, are permitted provided that the following conditions are met:
4 ' 1. Redistributions of source code must retain the above copyright notice,
5 '    this list of conditions and the following disclaimer.
6 ' 2. Redistributions in binary form must reproduce the above copyright notice,
7 '    this list of conditions and the following disclaimer in the documentation
8 '    and/or other materials provided with the distribution.
9 ' VA2 /87 ASCII BASIC companion for va2_basic87_asm_probe.asm.
100 CLEAR ,&H7FF0
110 DEF SEG=&H7FF0
120 BLOAD "X87RAW.BIN",0
130 M=0
140 CALL M
150 PRINT PEEK(&H20);PEEK(&H21);PEEK(&H22);PEEK(&H23)
160 PRINT PEEK(&H30);PEEK(&H31);PEEK(&H32);PEEK(&H33)
170 PRINT PEEK(&H34);PEEK(&H35);PEEK(&H36);PEEK(&H37)
180 END
