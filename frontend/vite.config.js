import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";

// Overridden to the "backend" service name in docker-compose.yml, since 127.0.0.1
// inside the frontend container doesn't reach the backend container.
const backendUrl = process.env.VITE_BACKEND_URL || "http://127.0.0.1:8000";

export default defineConfig({
  plugins: [react()],
  server: {
    host: true, // bind 0.0.0.0 so the dev server is reachable from outside the container
    port: 5173,
    proxy: {
      "/api": backendUrl,
    },
  },
});
