@echo off

del book1*

for %%a in (*.exe) do (
  echo [%%~na]
  %%a c book1 book1.%%~na
  %%a d book1.%%~na book1.unp
  fc /b book1 book1.unp
)


