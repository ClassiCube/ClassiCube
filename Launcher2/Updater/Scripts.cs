// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;

namespace Launcher.Updater {
	
	public static class Scripts {
		
		public const string BatchFile =
			@"@echo off
echo Waiting for launcher to exit..
echo 5..
sleep 1
echo 4..
sleep 1
echo 3..
sleep 1
echo 2..
sleep 1
echo 1..
sleep 1

set root=%CD%
echo Extracting files from CS_Update folder
for /f ""tokens=*"" %%f in ('dir /b ""%root%\CS_Update""') do move ""%root%\CS_Update\%%f"" ""%root%\%%f""
rmdir ""%root%\CS_Update""

echo Starting launcher again
if exist Launcher.exe (
start Launcher.exe
) else (
start Launcher2.exe
)
exit";
		
		public const string BashFile =
			@"#!/bin/bash
echo Waiting for launcher to exit..
echo 5..
sleep 1
echo 4..
sleep 1
echo 3..
sleep 1
echo 2..
sleep 1
echo 1..
sleep 1

echo Extracting files from CS_Update folder
UPDATEDIR=""`pwd`/CS_Update/""
find ""$UPDATEDIR"" -name '*.*' | xargs cp -t `pwd`

rm -r ""$UPDATEDIR""
echo Starting launcher again
if [ -f ""Launcher.exe"" ];
then
mono Launcher.exe
else
mono Launcher2.exe
fi
";
	}
}
