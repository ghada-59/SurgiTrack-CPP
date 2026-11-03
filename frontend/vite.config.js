import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

const root = path.dirname(fileURLToPath(import.meta.url));
const resultsDir = path.resolve(root, "../results");
const resultFiles = [
  "results.json",
  "points.json",
  "mc.csv",
  "ctest_release.txt",
  "tre_vs_distance.svg",
  "tre_histogram_n6_fle0p25.svg",
];

function contentType(name) {
  if (name.endsWith(".json")) return "application/json; charset=utf-8";
  if (name.endsWith(".csv")) return "text/csv; charset=utf-8";
  if (name.endsWith(".svg")) return "image/svg+xml";
  return "text/plain; charset=utf-8";
}

function surgitrackResults() {
  return {
    name: "surgitrack-results",
    configureServer(server) {
      server.middlewares.use("/data", (req, res) => {
        const name = decodeURIComponent((req.url || "/").split("?")[0].replace(/^\//, ""));
        const filePath = path.join(resultsDir, name);
        if (!resultFiles.includes(name) || !fs.existsSync(filePath)) {
          res.statusCode = 404;
          res.end("not found");
          return;
        }
        res.setHeader("Content-Type", contentType(name));
        fs.createReadStream(filePath).pipe(res);
      });
    },
    closeBundle() {
      const destination = path.resolve(root, "dist/data");
      fs.mkdirSync(destination, { recursive: true });
      for (const name of resultFiles) {
        const source = path.join(resultsDir, name);
        if (fs.existsSync(source)) {
          fs.copyFileSync(source, path.join(destination, name));
        }
      }
    },
  };
}

export default defineConfig({
  plugins: [react(), surgitrackResults()],
  server: { host: "127.0.0.1" },
});
