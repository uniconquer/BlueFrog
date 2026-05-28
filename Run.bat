@echo off
rem Launches FL with the correct working directory so Assets/ paths
rem (Models, Scenes, Prefabs, Textures, Audio) resolve relative to the
rem project root instead of the build output folder.
cd /d "%~dp0BlueFrog"
start "" "x64\Debug\FL.exe"
