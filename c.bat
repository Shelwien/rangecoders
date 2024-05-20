@echo off

set incs=-DSTRICT -DNDEBUG -DWIN32

set opts=-fwhole-program -fstrict-aliasing -fomit-frame-pointer -fno-stack-protector -fno-stack-check -fno-check-new ^
-flto -ffat-lto-objects -Wl,-flto -fuse-linker-plugin -Wl,-O -Wl,--sort-common -Wl,--as-needed -ffunction-sections

set gcc=C:\MinGW820x\bin\g++.exe -march=k8
set path=%gcc%\..\

del *.exe *.o

for %%a in (*.cpp) do (
  echo [%%a]
  %gcc% -s -std=gnu++1z -O9 %incs% %opts% -static %%a -o %%~na.exe
  %gcc% -s -std=gnu++1z -O9 -DRESCALE_64k %incs% %opts% -static %%a -o %%~na_r.exe
)
