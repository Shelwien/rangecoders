@echo off

del wcc386*

for %%a in (*.exe) do (
  echo [%%~na]
  %%a c wcc386 wcc386.%%~na
  %%a d wcc386.%%~na wcc386.unp
  fc /b wcc386 wcc386.unp
)


