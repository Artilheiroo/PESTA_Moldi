const express = require("express");
const { getPool } = require("../db/mysql");

const router = express.Router();

function isValidIsoDate(value) {
  if (typeof value !== "string") {
    return false;
  }

  const parsed = new Date(value);
  return !Number.isNaN(parsed.getTime());
}

router.post("/readings", async (req, res) => {
  const { device_id: deviceId, metric, value, unit, ts } = req.body || {};

  if (
    typeof deviceId !== "string" ||
    typeof metric !== "string" ||
    typeof unit !== "string" ||
    typeof value !== "number" ||
    !isValidIsoDate(ts)
  ) {
    return res.status(400).json({
      ok: false,
      error: "Invalid payload. Expected {device_id, metric, value, unit, ts}."
    });
  }

  try {
    await getPool().execute(
      `INSERT INTO device_readings (device_id, metric, value, unit, ts)
       VALUES (?, ?, ?, ?, ?)`,
      [deviceId, metric, value, unit, new Date(ts)]
    );

    return res.status(201).json({ ok: true });
  } catch (error) {
    console.error("Failed to insert reading:", error.message);
    return res.status(500).json({ ok: false, error: "Database insert failed." });
  }
});

module.exports = router;
