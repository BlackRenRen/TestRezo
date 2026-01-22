@echo off
set "UE=D:\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "PROJ=C:\Users\Renaud\Downloads\EOS-Getting-Started-main\EOS-Getting-Started-main\OnlineSubsystemEOS\EOS_OSS_Tutorial.uproject"
set "MAP=/Game/ThirdPerson/Maps/GameMap"
set "LOG=%~dp0ClientLog.txt"

"%UE%" "%PROJ%" %MAP% -game -log -stdout -FullStdOutLogOutput -abslog="%LOG%" ^
 -AUTH_TYPE=developer -AUTH_LOGIN=127.0.0.1:8081 -AUTH_PASSWORD=Johan ^
 -epicapp=Client
