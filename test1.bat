@echo off

type nul >0
set /p =a>1 <nul
set /p =aa>2 <nul
set /p =aaa>3 <nul
set /p =aaaa>4 <nul

for %%b in (0,1,2,3,4) do (
  for %%a in (*.exe) do (
    echo [%%~na]
    %%a c %%b %%b.%%~na
    %%a d %%b.%%~na %%b.unp
    fc /b %%b %%b.unp
  )
)


