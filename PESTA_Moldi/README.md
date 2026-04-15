# ESP32-GATEWAY + API + MySQL (MVP)

Projeto MVP para ESP32 (ESP-IDF, em C) que envia leituras de temperatura simuladas para uma API HTTP, com persistencia em MySQL.

## Estrutura

- `firmware/`: firmware ESP-IDF (Wi-Fi + HTTP POST periodico)
- `backend/`: API Node.js/Express com insercao em MySQL
- `infra/sql/init.sql`: schema e tabela `device_readings`
- `docs/api.md`: contrato do endpoint

## 1) Preparar MySQL

1. Criar a base/tabela:

   ```sql
   SOURCE infra/sql/init.sql;
   ```

2. Confirmar que a tabela existe:

   ```sql
   USE esp32_gateway;
   SHOW TABLES;
   ```

## 2) Configurar e correr backend

1. Copiar variaveis de ambiente:

   - Copiar `backend/.env.example` para `backend/.env`
   - Ajustar credenciais MySQL

2. Instalar dependencias e arrancar:

   ```bash
   cd backend
   npm install
   npm start
   ```

3. Testar saude da API:

   ```bash
   curl http://localhost:3000/health
   ```

## 3) Testar endpoint sem ESP32 (teste de integracao inicial)

```bash
curl -X POST http://localhost:3000/api/readings \
  -H "Content-Type: application/json" \
  -d "{\"device_id\":\"esp32-gateway-01\",\"metric\":\"temperature\",\"value\":25.8,\"unit\":\"C\",\"ts\":\"2026-04-15T11:40:00Z\"}"
```

Esperado:
- HTTP `201`
- Body `{"ok":true}`

Confirmar no MySQL:

```sql
USE esp32_gateway;
SELECT * FROM device_readings ORDER BY id DESC LIMIT 10;
```

## 4) Configurar firmware ESP-IDF

1. Entrar na pasta firmware e configurar:

   ```bash
   cd firmware
   idf.py set-target esp32
   idf.py menuconfig
   ```

2. Definir os parametros em `ESP32 Gateway configuration`:
- `WIFI_SSID`
- `WIFI_PASSWORD`
- `API_URL` (ex.: `http://<ip_do_pc>:3000/api/readings`)
- `DEVICE_ID`
- `SEND_INTERVAL_SECONDS`
- `HTTP_TIMEOUT_MS`

3. Build/flash/monitor:

   ```bash
   idf.py build
   idf.py -p <PORTA_SERIAL> flash monitor
   ```

## Comportamento implementado no firmware

- Conecta ao Wi-Fi e tenta reconectar automaticamente.
- Gera leitura simulada de temperatura.
- Envia JSON periodicamente para a API.
- Em falha de envio, faz um retry simples apos 3 segundos.

## Notas de seguranca e evolucao

- MVP usa HTTP para simplicidade local.
- Proximo passo recomendado: HTTPS + autenticacao por token/API key.
- Tambem recomendado: buffer local (store-and-forward) para periodos offline.
