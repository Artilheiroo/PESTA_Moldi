FROM python:3.11-alpine

WORKDIR /app

# Instalar dependências necessárias
RUN pip install --no-cache-dir mysql-connector-python

# Copiar o script da ponte
COPY db_bridge.py .

# Expor a porta que o ESP32 vai usar (por padrão 5000)
EXPOSE 5000

# Executar a aplicação
CMD ["python", "db_bridge.py"]
