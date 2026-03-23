import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/login': 'http://localhost:9080',
      '/signup': 'http://localhost:9080',
      '/me': 'http://localhost:9080',
      '/assignment-hours/total': 'http://localhost:9080'
    }
  }
})
