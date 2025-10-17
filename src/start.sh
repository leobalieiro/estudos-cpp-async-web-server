#!/bin/bash
set -e

# Usa SERVER_PORT do Docker ou porta padrão 8080
SERVER_PORT=${SERVER_PORT:-8080}

compile_and_run() {
    echo "🛠️  Compilando /app/server.cpp..."
    
    if g++ -std=c++11 -Wall -O2 -o /app/server /app/server.cpp -lcpprest -lboost_system -lssl -lcrypto -lpthread; then
        echo "✅ Compilação bem-sucedida!"
        echo "🚀 Iniciando servidor na porta $SERVER_PORT..."
        echo "========================================"
        # Executa em primeiro plano
        exec /app/server
    else
        echo "❌ Erro na compilação. Aguardando alterações..."
    fi
}

# Primeira execução
compile_and_run

# Loop de watch (se a compilação falhar)
while inotifywait -e close_write /app/server.cpp; do
    echo "📁 Arquivo alterado. Recompilando..."
    compile_and_run
done