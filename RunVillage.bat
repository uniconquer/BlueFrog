@echo off
rem Launches FL starting in the village hub (Phase I-1 entry point).
rem `--scene` overrides the default arena_trial.json that Run.bat uses.
cd /d "%~dp0BlueFrog"
start "" "x64\Debug\FL.exe" --scene Assets/Scenes/village.json
