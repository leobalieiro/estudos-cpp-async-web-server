#!/bin/bash
set -e

# Script simples para recompilar e rodar automaticamente
echo "🛠️  Compilando server.cpp..."
g++ -Wall -O2 -o server server.cpp

echo "🚀 Executando..."
./server

# Mantém o container vivo (para não fechar logo)
# e permite recompilar se mudar o código.
# Se quiser recompilar em tempo real, pode adicionar um loop (abaixo):
# while inotifywait -e close_write main.cpp; do clear; g++ -o main main.cpp && ./main; done
