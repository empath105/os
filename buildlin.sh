#!/bin/bash
set -e
echo "Обновление кода из гитхаба..."
git pull origin main
echo "Сборка и компиляция..."
mkdir -p build
cd build
cmake ..
make
./hello
cd ..
