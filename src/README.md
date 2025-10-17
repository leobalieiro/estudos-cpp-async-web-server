**Compila codigo:**

```
g++ -std=c++11 -Wall -O2 -o /app/server /app/server.cpp -lcpprest -lboost_system -lssl -lcrypto -lpthread;
```


**Roda servidor:**

```
exec /app/server
```


**Mata processo:**

```
pkill server && echo "✅ Servidor parado" || echo "❌ Servidor não estava rodando"
```



```
# No container Ubuntu/Debian
docker exec -it estudo_cpp_web_server_app apt-get update
docker exec -it estudo_cpp_web_server_app apt-get install -y apache2-utils

# Agora funciona:
docker exec -it estudo_cpp_web_server_app ab -n 1000 -c 100 http://localhost:8080/api/


# Instalar wrk
docker exec -it estudo_cpp_web_server_app apt-get update
docker exec -it estudo_cpp_web_server_app apt-get install -y wrk

# Teste com wrk - equivalente a ~100.000 requests
docker exec -it estudo_cpp_web_server_app wrk -t12 -c100 -d30s --latency http://localhost:8080/api/

# Teste mais longo
docker exec -it estudo_cpp_web_server_app wrk -t8 -c200 -d60s --latency http://localhost:8080/api/


```
