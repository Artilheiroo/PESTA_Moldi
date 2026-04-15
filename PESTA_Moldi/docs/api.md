# API Contract

## Endpoint

`POST /api/readings`

## Request JSON

```json
{
  "device_id": "esp32-gateway-01",
  "metric": "temperature",
  "value": 25.42,
  "unit": "C",
  "ts": "2026-04-15T11:40:00Z"
}
```

## Success response

- Status: `201 Created`
- Body:

```json
{
  "ok": true
}
```

## Error responses

- `400 Bad Request` when payload is invalid.
- `500 Internal Server Error` when database insert fails.

## Validation rules

- `device_id`: string
- `metric`: string
- `value`: number
- `unit`: string
- `ts`: valid ISO8601 datetime string
