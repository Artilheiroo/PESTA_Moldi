CREATE DATABASE IF NOT EXISTS esp32_gateway
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

USE esp32_gateway;

CREATE TABLE IF NOT EXISTS device_readings (
  id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  device_id VARCHAR(64) NOT NULL,
  metric VARCHAR(32) NOT NULL,
  value DOUBLE NOT NULL,
  unit VARCHAR(16) NOT NULL,
  ts DATETIME(3) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_device_ts (device_id, ts),
  KEY idx_metric_ts (metric, ts)
);
