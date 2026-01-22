@echo off
set "UE=D:\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJ=C:\Users\Renaud\Downloads\EOS-Getting-Started-main\EOS-Getting-Started-main\OnlineSubsystemEOS\EOS_OSS_Tutorial.uproject"
set "MAP=/Game/ThirdPerson/Maps/GameMap"
set "LOG=%~dp0ServerLog.txt"

"%UE%" "%PROJ%" %MAP% -server -log -stdout -FullStdOutLogOutput -abslog="%LOG%" ^
 -epicapp=Server ^
 -AUTH_TYPE=devtool -AUTH_LOGIN=localhost:8081 -AUTH_PASSWORD=ServerUser