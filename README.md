# Sudoku Solver — Deployment Guide

Full-stack Sudoku solver with a **C++ backend** (httplib + OpenCV) and a **vanilla JS frontend** hosted on GitHub Pages.

---

## Repository Structure

```
sudoku-solver/
├── frontend/           ← GitHub Pages site
│   ├── index.html
│   ├── style.css
│   ├── script.js
│   └── config.js       ← ✏️  Edit this to point to your cloud backend
├── backend/            ← C++ server
│   ├── include/
│   │   ├── httplib.h
│   │   ├── sudoku_solver.h
│   │   ├── image_intake_service.h
│   │   └── vision_pipeline.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── server.cpp
│   │   ├── sudoku_solver.cpp
│   │   ├── image_intake_service.cpp
│   │   └── vision_pipeline.cpp
│   ├── assets/
│   │   └── models/
│   │       └── mnist.onnx   ← optional (image solving only)
│   └── CMakeLists.txt
├── Dockerfile
└── README.md
```

---

## API Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET    | `/health` | Health check — returns `{"status":"ok"}` |
| POST   | `/solve-manual` | Solve a manually-entered puzzle |
| POST   | `/solve-image` | Solve from an uploaded image |

### POST /solve-manual

**Request body (JSON):**
```json
{
  "board": [
    [5,3,0,0,7,0,0,0,0],
    [6,0,0,1,9,5,0,0,0],
    ...
  ]
}
```
Use `0` for empty cells.

**Response (JSON):**
```json
{
  "detected": [[5,3,4,...], ...],
  "solved":   [[5,3,4,...], ...]
}
```

---

## 1. Run Locally

### Prerequisites
- CMake ≥ 3.15
- C++17 compiler (GCC / Clang / MSVC)
- OpenCV 4.x with `core`, `imgcodecs`, `imgproc`, `dnn`

### Build & run
```bash
cd backend
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/SudokuBackend          # listens on :8080
```

Open `frontend/index.html` directly in your browser — it already calls `http://localhost:8080`.

---

## 2. Deploy Backend to Render (free tier)

1. Push this repo to GitHub.
2. Go to [render.com](https://render.com) → **New → Web Service**.
3. Connect your GitHub repo.
4. Set:
   - **Environment:** Docker
   - **Dockerfile path:** `Dockerfile`
   - **Port:** `8080`
5. Click **Create Web Service**.

Render builds the Docker image and gives you a URL like:
```
https://sudoku-backend-xxxx.onrender.com
```

---

## 3. Deploy Backend to Railway

1. Push this repo to GitHub.
2. Go to [railway.app](https://railway.app) → **New Project → Deploy from GitHub repo**.
3. Railway auto-detects the `Dockerfile`.
4. Set environment variable: `PORT=8080` (Railway usually injects this automatically).
5. After deployment you get a URL like:
   ```
   https://sudoku-backend-production.up.railway.app
   ```

---

## 4. Deploy Frontend to GitHub Pages

1. In your repo settings → **Pages** → Source: **Deploy from a branch**.
2. Set the branch to `main` and folder to `/frontend`.
3. Your site will be live at:
   ```
   https://<your-username>.github.io/<repo-name>/
   ```

---

## 5. Connect Frontend to Backend

Edit `frontend/config.js`:
```javascript
window.BACKEND_URL = "https://sudoku-backend-xxxx.onrender.com";
```
Push to GitHub — GitHub Pages redeploys automatically.

---

## 6. Image Solving (optional CNN model)

The `/solve-image` endpoint uses OpenCV to detect the Sudoku grid.
Digit recognition works best with the ONNX model:

1. Download or train a CNN on the MNIST digit dataset.
2. Export it to ONNX format (`mnist.onnx`).
3. Place it at `backend/assets/models/mnist.onnx`.
4. Rebuild — the model is copied to the build output automatically.

Without the model the pipeline still detects the grid but cannot read digits reliably; the frontend will display a partial extraction for the user to correct manually.

---

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PORT`   | `8080`  | Port the server listens on (injected by Render/Railway) |
