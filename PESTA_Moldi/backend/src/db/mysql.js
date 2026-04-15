const mysql = require("mysql2/promise");

let pool;

function getPool() {
  if (!pool) {
    pool = mysql.createPool({
      host: process.env.DB_HOST || "127.0.0.1",
      port: Number(process.env.DB_PORT || 3306),
      user: process.env.DB_USER || "root",
      password: process.env.DB_PASSWORD || "",
      database: process.env.DB_NAME || "esp32_gateway",
      connectionLimit: Number(process.env.DB_CONNECTION_LIMIT || 10),
      waitForConnections: true,
      queueLimit: 0
    });
  }

  return pool;
}

async function checkDbConnection() {
  const connection = await getPool().getConnection();
  connection.release();
}

module.exports = {
  getPool,
  checkDbConnection
};
