<div align="center">

# 🎯 Sudoku

**A fully interactive, browser-based Sudoku game built with vanilla JavaScript, HTML5, and CSS3 — containerized with Docker and deployed via GitHub Pages.**

[![GitHub Pages](https://img.shields.io/badge/Live%20Demo-GitHub%20Pages-brightgreen?style=for-the-badge&logo=github)](https://rohanrathodonline.github.io/Sudoku)
[![Docker](https://img.shields.io/badge/Docker-Ready-blue?style=for-the-badge&logo=docker)](./Dockerfile)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge)](./LICENSE)
[![Commits](https://img.shields.io/github/commit-activity/t/RohanRathodOnline/Sudoku?style=for-the-badge)](https://github.com/RohanRathodOnline/Sudoku/commits/main)

</div>

---

## 📸 Preview

> A clean, minimal Sudoku interface with real-time input validation, difficulty levels, and a responsive grid — playable directly in the browser with zero dependencies.

---

## 🚀 Live Demo

🔗 **[Play Now → rohanrathodonline.github.io/Sudoku](https://rohanrathodonline.github.io/Sudoku)**

---

## ✨ Features

- 🧩 **Interactive 9×9 Sudoku Board** — Click any cell and enter numbers 1–9
- ✅ **Real-time Validation** — Instantly highlights invalid row, column, and box conflicts
- 🎲 **Puzzle Generation** — Random puzzles generated at runtime
- 🏆 **Difficulty Levels** — Easy, Medium, and Hard modes
- 💡 **Hint System** — Reveal a single correct cell when stuck
- ⏱️ **Timer** — Track how long each puzzle takes to solve
- 📱 **Responsive Design** — Works seamlessly on desktop, tablet, and mobile
- 🐳 **Docker Support** — Run the full stack locally in one command

---

## 🗂️ Project Structure

```
Sudoku/
├── backend/              # Server-side logic (Node.js / API)
│   └── ...
├── index.html            # Main entry point and game UI
├── script.js             # Core game logic, puzzle generation & validation
├── style.css             # Styling and responsive layout
├── config.js             # App configuration (difficulty settings, constants)
├── Dockerfile            # Docker container definition
├── .gitignore            # Git ignore rules
└── README.md             # Project documentation
```

---

## 🛠️ Tech Stack

| Layer      | Technology          |
|------------|---------------------|
| Frontend   | HTML5, CSS3, Vanilla JavaScript |
| Backend    | Node.js             |
| Containerization | Docker        |
| Deployment | GitHub Pages        |
| Version Control | Git / GitHub  |

---

## ⚙️ Getting Started

### 🔧 Option 1 — Open Directly in Browser

No installation needed. Simply clone the repo and open `index.html`:

```bash
git clone https://github.com/RohanRathodOnline/Sudoku.git
cd Sudoku
open index.html   # macOS
# or: start index.html  (Windows)
# or: xdg-open index.html  (Linux)
```

---

### 🐳 Option 2 — Run with Docker

Make sure you have [Docker](https://www.docker.com/get-started) installed.

```bash
# 1. Clone the repository
git clone https://github.com/RohanRathodOnline/Sudoku.git
cd Sudoku

# 2. Build the Docker image
docker build -t sudoku-app .

# 3. Run the container
docker run -p 8080:8080 sudoku-app

# 4. Open your browser
open http://localhost:8080
```

---

### 🖥️ Option 3 — Run with Node.js Backend

```bash
# 1. Clone the repository
git clone https://github.com/RohanRathodOnline/Sudoku.git
cd Sudoku/backend

# 2. Install dependencies
npm install

# 3. Start the server
npm start

# 4. Open your browser
open http://localhost:3000
```

---

## 🔩 Configuration

Edit `config.js` to customize game settings:

```javascript
// config.js
const CONFIG = {
  difficulty: {
    easy:   { clues: 40 },
    medium: { clues: 30 },
    hard:   { clues: 22 },
  },
  timer: {
    enabled: true,
  },
  hints: {
    maxPerGame: 3,
  }
};
```

---

## 🎮 How to Play

1. **Select a difficulty** — Easy, Medium, or Hard
2. **Click any empty cell** and type a number from **1 to 9**
3. **Red highlights** indicate conflicts — fix them before proceeding
4. **Use hints** (if enabled) to reveal a correct cell
5. Complete the board so every **row**, **column**, and **3×3 box** contains the digits **1–9** exactly once
6. 🎉 Win the game when the entire board is correctly filled!

---

## 📦 Deployment

This project is deployed automatically to **GitHub Pages** on every push to the `main` branch.

To deploy your own fork:

1. Fork this repository
2. Go to **Settings → Pages**
3. Set source to `Deploy from branch → main → / (root)`
4. Your app will be live at `https://<your-username>.github.io/Sudoku`

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!

```bash
# 1. Fork the project
# 2. Create your feature branch
git checkout -b feature/AmazingFeature

# 3. Commit your changes
git commit -m "Add AmazingFeature"

# 4. Push to the branch
git push origin feature/AmazingFeature

# 5. Open a Pull Request
```

Please make sure your code follows the existing style and all features are tested before submitting a PR.

---

## 🐛 Known Issues & Roadmap

- [ ] Add keyboard navigation (arrow keys between cells)
- [ ] Add undo / redo functionality
- [ ] Add a notes/pencil mode for candidates
- [ ] Add dark mode toggle
- [ ] Add leaderboard / high scores
- [ ] Add animated puzzle completion screen

---

## 📄 License

This project is licensed under the **MIT License** — see the [LICENSE](./LICENSE) file for details.

---

## 👤 Author

**Rohan Rathod**

[![GitHub](https://img.shields.io/badge/GitHub-RohanRathodOnline-181717?style=flat&logo=github)](https://github.com/RohanRathodOnline)

---

<div align="center">

⭐ **Star this repo if you found it useful!** ⭐

Made with ❤️ by [RohanRathodOnline](https://github.com/RohanRathodOnline)

</div>
