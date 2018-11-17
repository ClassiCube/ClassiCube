@echo off
for /f %%i in ('git rev-parse HEAD')do set SHA=%%i
<nul set/p=%SHA:~1,7%
