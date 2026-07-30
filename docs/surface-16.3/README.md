# Import surface — League 16.3 x86_64 slice

Primary porting target (GL renderer still present — see ../AUDIT-VERSIONS.md).
Same format and regeneration commands as [../surface-16.15/README.md](../surface-16.15/README.md);
point `BIN` at `~/Game/16.3/LeagueofLegends.app/Contents/MacOS/LeagueofLegends`.

Notable vs 16.15: `OpenGL.txt` (111 syms — ~100 `gl*` mapping 1:1 to host libGL, plus
CGL* needing stubs) and NSOpenGL* classes in `objc-classes.txt`.
