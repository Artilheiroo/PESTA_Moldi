require("dotenv").config();
const express = require("express");
const readingsRouter = require("./routes/readings");
const { checkDbConnection } = require("./db/mysql");

const app = express();
const port = Number(process.env.PORT || 3000);

app.use(express.json({ limit: "32kb" }));

app.get("/health", async (_req, res) => {
  try {
    await checkDbConnection();
    return res.status(200).json({ ok: true, db: "up" });
  } catch (error) {
    return res.status(500).json({ ok: false, db: "down", error: error.message });
  }
});

app.use("/api", readingsRouter);

app.use((err, _req, res, _next) => {
  console.error("Unhandled error:", err);
  res.status(500).json({ ok: false, error: "Internal server error." });
});

app.listen(port, () => {
  console.log(`API listening on http://localhost:${port}`);
});
