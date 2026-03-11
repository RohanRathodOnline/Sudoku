(() => {
    const API_BASE_URL = (typeof window.BACKEND_URL === "string" && window.BACKEND_URL.trim())
        ? window.BACKEND_URL.trim().replace(/\/$/, "")
        : "http://localhost:8080";

    const input = document.getElementById("sudoku-image-input");
    const dropzone = document.querySelector(".upload-dropzone");
    const previewImage = document.querySelector(".upload-preview-image");
    const changeImageBtn = document.querySelector(".change-image-btn");
    const processImageBtn = document.querySelector(".process-image-btn");
    const solveBtn = document.querySelector(".solve-btn");
    const ocrStatus = document.querySelector(".ocr-status");
    const gridCells = Array.from(document.querySelectorAll(".sudoku-grid .cell"));
    const numberButtons = Array.from(document.querySelectorAll(".num-btn"));
    const actionItems = Array.from(document.querySelectorAll(".action-item"));

    if (!input || !dropzone || !previewImage || !changeImageBtn || !processImageBtn || !solveBtn || !ocrStatus || gridCells.length !== 81) {
        return;
    }

    let selectedIndex = null;
    const boardHistory = [];

    // ─────────────────────────────────────────────────────────────────────────
    // JS Fallback Sudoku Solver (backtracking — runs entirely in the browser)
    // ─────────────────────────────────────────────────────────────────────────
    const jsSolver = {
        // Deep-copy a 9x9 board
        clone(board) {
            return board.map(row => [...row]);
        },

        // Check if placing `val` at (row, col) is valid
        isSafe(board, row, col, val) {
            // Check row
            for (let c = 0; c < 9; c++) {
                if (board[row][c] === val) return false;
            }
            // Check column
            for (let r = 0; r < 9; r++) {
                if (board[r][col] === val) return false;
            }
            // Check 3x3 box
            const boxRow = Math.floor(row / 3) * 3;
            const boxCol = Math.floor(col / 3) * 3;
            for (let r = boxRow; r < boxRow + 3; r++) {
                for (let c = boxCol; c < boxCol + 3; c++) {
                    if (board[r][c] === val) return false;
                }
            }
            return true;
        },

        // Check if the given clues have any conflicts
        hasConflicts(board) {
            for (let r = 0; r < 9; r++) {
                for (let c = 0; c < 9; c++) {
                    const val = board[r][c];
                    if (val === 0) continue;
                    // Temporarily clear to check against others
                    board[r][c] = 0;
                    if (!this.isSafe(board, r, c, val)) {
                        board[r][c] = val;
                        return true;
                    }
                    board[r][c] = val;
                }
            }
            return false;
        },

        // Backtracking solve — modifies board in place, returns true if solved
        solve(board) {
            for (let r = 0; r < 9; r++) {
                for (let c = 0; c < 9; c++) {
                    if (board[r][c] !== 0) continue;
                    for (let val = 1; val <= 9; val++) {
                        if (this.isSafe(board, r, c, val)) {
                            board[r][c] = val;
                            if (this.solve(board)) return true;
                            board[r][c] = 0;
                        }
                    }
                    return false; // No valid value found
                }
            }
            return true; // All cells filled
        },

        // Main entry: returns { solved: [...] } or { error: "..." }
        run(board) {
            const copy = this.clone(board);
            if (this.hasConflicts(copy)) {
                return { error: "Board contains conflicting numbers." };
            }
            if (this.solve(copy)) {
                return { solved: copy };
            }
            return { error: "This puzzle has no valid solution." };
        }
    };

    // ─────────────────────────────────────────────────────────────────────────

    const setStatus = (message, isError = false) => {
        ocrStatus.textContent = message;
        ocrStatus.style.display = "block";
        ocrStatus.style.color = isError ? "#b91c1c" : "#1e3a8a";
    };

    const showSolveCelebration = () => {
        const existing = document.querySelector(".solve-celebration");
        if (existing) existing.remove();

        const bubble = document.createElement("div");
        bubble.className = "solve-celebration";
        bubble.textContent = "🎉 Congratulations! Sudoku solved!";
        document.body.appendChild(bubble);

        window.setTimeout(() => {
            bubble.classList.add("hide");
            window.setTimeout(() => bubble.remove(), 500);
        }, 3000);
    };

    const snapshotBoard = () => gridCells.map((cell) => cell.textContent || "");

    const restoreBoard = (snapshot) => {
        if (!Array.isArray(snapshot) || snapshot.length !== 81) return;
        for (let i = 0; i < 81; i++) {
            gridCells[i].textContent = snapshot[i] || "";
        }
    };

    const wait = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

    const animateSolveByColumn = async (fromBoard, solvedBoard) => {
        if (!Array.isArray(fromBoard) || !Array.isArray(solvedBoard)) return;

        const animationOrder = [];
        const visited = new Set();
        const addRegion = (rowStart, rowEnd, colStart, colEnd) => {
            for (let row = rowStart; row <= rowEnd; row++) {
                for (let col = colStart; col <= colEnd; col++) {
                    const key = `${row}-${col}`;
                    if (!visited.has(key)) {
                        visited.add(key);
                        animationOrder.push([row, col]);
                    }
                }
            }
        };

        for (let i = 0; i < 9; i++) {
            addRegion(i, i, 0, 8);
            addRegion(0, 8, i, i);
        }

        for (const [row, col] of animationOrder) {
            const fromValue = Number(fromBoard[row]?.[col] || 0);
            const toValue = Number(solvedBoard[row]?.[col] || 0);

            if (fromValue === 0 && toValue > 0) {
                const cellIndex = row * 9 + col;
                gridCells[cellIndex].textContent = String(toValue);
                gridCells[cellIndex].classList.add("is-selected");
                await wait(45);
                gridCells[cellIndex].classList.remove("is-selected");
            }
        }
    };

    const pushHistory = () => {
        boardHistory.push(snapshotBoard());
        if (boardHistory.length > 100) boardHistory.shift();
    };

    const setSelectedCell = (index) => {
        selectedIndex = index;
        gridCells.forEach((cell, i) => {
            cell.classList.toggle("is-selected", i === index);
        });
    };

    const readBoardFromGrid = () => {
        const board = Array.from({ length: 9 }, () => Array(9).fill(0));
        for (let i = 0; i < 81; i++) {
            const value = Number(gridCells[i].textContent || 0);
            board[Math.floor(i / 9)][i % 9] = Number.isFinite(value) ? value : 0;
        }
        return board;
    };

    const writeCellValue = (index, value) => {
        if (index == null || index < 0 || index >= 81) return;
        gridCells[index].textContent = value === 0 ? "" : String(value);
    };

    const clearStatus = () => {
        ocrStatus.style.display = "none";
        ocrStatus.textContent = "";
    };

    const renderMatrix = (matrix) => {
        if (!Array.isArray(matrix) || matrix.length !== 9) return;
        let index = 0;
        for (let row = 0; row < 9; row++) {
            if (!Array.isArray(matrix[row]) || matrix[row].length !== 9) return;
            for (let col = 0; col < 9; col++) {
                const value = Number(matrix[row][col]) || 0;
                gridCells[index++].textContent = value === 0 ? "" : String(value);
            }
        }
    };

    const clearPreview = () => {
        if (previewImage.src) URL.revokeObjectURL(previewImage.src);
        previewImage.removeAttribute("src");
        dropzone.classList.remove("has-image", "is-processing", "is-dragging");
        clearStatus();
    };

    const setPreviewFromFile = (file) => {
        if (!file) { clearPreview(); return; }
        if (!file.type || !file.type.startsWith("image/")) {
            alert("Please select a valid image file.");
            input.value = "";
            clearPreview();
            return;
        }
        if (previewImage.src) URL.revokeObjectURL(previewImage.src);
        previewImage.src = URL.createObjectURL(file);
        dropzone.classList.add("has-image");
        dropzone.classList.remove("is-processing");
        setStatus("Image selected. Click Process Image.");
    };

    const processSelectedImage = async () => {
        const file = input.files && input.files[0];
        if (!file) { setStatus("Please select an image first.", true); return; }

        try {
            processImageBtn.disabled = true;
            dropzone.classList.add("is-processing");
            setStatus("Processing image, please wait...");

            const formData = new FormData();
            formData.append("image", file);

            const response = await fetch(`${API_BASE_URL}/solve-image`, {
                method: "POST",
                body: formData
            });

            const payload = await response.json().catch(() => ({}));

            if (!response.ok) {
                const fallbackMatrix = payload.rawDetected || payload.cleanedDetected || payload.detected;
                if (fallbackMatrix) renderMatrix(fallbackMatrix);
                const message =
                    (payload?.error && typeof payload.error === "object" ? payload.error.message : payload?.error) ||
                    payload?.message ||
                    `Image processing failed (${response.status}).`;
                setStatus(message, true);
                return;
            }

            const matrixToShow = payload.rawDetected || payload.cleanedDetected || payload.detected || payload.solved;
            if (matrixToShow) {
                pushHistory();
                renderMatrix(matrixToShow);
                setStatus(payload.rawDetected ? "Image processed. Showing raw OCR extraction." : "Image processed successfully.");
            } else {
                setStatus("Processing finished, but no Sudoku matrix was returned.", true);
            }
        } catch (error) {
            console.error("Process image error:", error);
            setStatus(`Cannot connect to backend at ${API_BASE_URL}. Is the server running?`, true);
        } finally {
            processImageBtn.disabled = false;
            dropzone.classList.remove("is-processing");
        }
    };

    // ── Event listeners ───────────────────────────────────────────────────────

    input.addEventListener("change", (event) => {
        setPreviewFromFile(event.target.files && event.target.files[0]);
    });

    changeImageBtn.addEventListener("click", (event) => {
        event.preventDefault();
        event.stopPropagation();
        input.click();
    });

    processImageBtn.addEventListener("click", (event) => {
        event.preventDefault();
        event.stopPropagation();
        processSelectedImage();
    });

    dropzone.addEventListener("dragover", (event) => {
        event.preventDefault();
        dropzone.classList.add("is-dragging");
    });

    dropzone.addEventListener("dragleave", () => {
        dropzone.classList.remove("is-dragging");
    });

    dropzone.addEventListener("drop", (event) => {
        event.preventDefault();
        dropzone.classList.remove("is-dragging");
        const file = event.dataTransfer?.files?.[0];
        if (!file) return;
        const dt = new DataTransfer();
        dt.items.add(file);
        input.files = dt.files;
        setPreviewFromFile(file);
    });

    gridCells.forEach((cell, index) => {
        cell.tabIndex = 0;
        cell.addEventListener("click", () => setSelectedCell(index));
        cell.addEventListener("keydown", (event) => {
            if (event.key >= "1" && event.key <= "9") {
                pushHistory();
                writeCellValue(index, Number(event.key));
                return;
            }
            if (event.key === "Backspace" || event.key === "Delete" || event.key === "0") {
                pushHistory();
                writeCellValue(index, 0);
            }
        });
    });

    numberButtons.forEach((button) => {
        button.addEventListener("click", () => {
            if (selectedIndex == null) return;
            const value = Number(button.textContent);
            if (value < 1 || value > 9) return;
            pushHistory();
            writeCellValue(selectedIndex, value);
        });
    });

    const getActionButton = (label) => {
        const item = actionItems.find((entry) => {
            const text = entry.querySelector(".action-label")?.textContent?.trim().toLowerCase();
            return text === label;
        });
        return item?.querySelector(".circle-btn") || null;
    };

    getActionButton("undo")?.addEventListener("click", () => {
        const previous = boardHistory.pop();
        if (previous) restoreBoard(previous);
    });

    getActionButton("erase")?.addEventListener("click", () => {
        if (selectedIndex == null) return;
        pushHistory();
        writeCellValue(selectedIndex, 0);
    });

    getActionButton("clear all")?.addEventListener("click", () => {
        pushHistory();
        gridCells.forEach((cell) => { cell.textContent = ""; });
    });

    // ── Solve button — tries backend first, falls back to JS solver ───────────
    solveBtn.addEventListener("click", async () => {
        const board = readBoardFromGrid();

        solveBtn.disabled = true;
        setStatus("Solving puzzle...");

        // ── Try backend first ─────────────────────────────────────────────────
        try {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 8000); // 8s timeout

            const response = await fetch(`${API_BASE_URL}/solve-manual`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ board }),
                signal: controller.signal
            });

            clearTimeout(timeout);

            const payload = await response.json().catch(() => ({}));

            if (response.ok && payload?.solved) {
                pushHistory();
                await animateSolveByColumn(board, payload.solved);
                renderMatrix(payload.solved);
                setStatus("Sudoku solved successfully.");
                showSolveCelebration();
                solveBtn.disabled = false;
                return;
            }

            // Backend responded but returned an error (e.g. invalid board)
            const message =
                (payload?.error && typeof payload.error === "object" ? payload.error.message : payload?.error) ||
                payload?.message ||
                `Solve failed (${response.status}).`;
            setStatus(message, true);
            solveBtn.disabled = false;
            return;

        } catch (err) {
            // Network error or timeout — fall through to JS solver
            console.warn("Backend unavailable, using JS solver:", err.message);
        }

        // ── JS fallback solver ────────────────────────────────────────────────
        setStatus("Backend offline — solving in browser...");

        // yield to browser so status text renders before heavy computation
        await wait(30);

        const result = jsSolver.run(board);

        if (result.error) {
            setStatus(result.error, true);
        } else {
            pushHistory();
            await animateSolveByColumn(board, result.solved);
            renderMatrix(result.solved);
            setStatus("Sudoku solved in browser (backend was offline).");
            showSolveCelebration();
        }

        solveBtn.disabled = false;
    });
})();
