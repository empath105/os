@echo off
chcp 65001 >nul
echo Обновление кода из гитхаба...
git pull origin main
echo Сборка и компиляция...
if not exist build mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
hello.exe
cd ..
