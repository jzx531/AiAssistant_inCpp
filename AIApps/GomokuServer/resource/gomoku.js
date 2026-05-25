const boardSize = 32;
const emptyCell = 0;
const blackStone = 1;
const whiteStone = 2;

const canvas = document.getElementById("gomokuBoard");
const ctx = canvas.getContext("2d");
const statusEl = document.getElementById("status");
const turnTextEl = document.getElementById("turnText");
const lastMoveEl = document.getElementById("lastMove");
const startBtn = document.getElementById("startBtn");
const resetBtn = document.getElementById("resetBtn");
const aiBtn = document.getElementById("aiBtn");

const state = {
    board: Array.from({ length: boardSize }, () => Array(boardSize).fill(emptyCell)),
    currentTurn: blackStone,
    gameOver: false,
    winner: emptyCell,
    lastMove: null,
};

const padding = 30;
const cellSize = (canvas.width - padding * 2) / (boardSize - 1);

function drawBoard() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let i = 0; i < boardSize; i += 1) {
        const offset = padding + i * cellSize;

        ctx.strokeStyle = "#6b431f";
        ctx.lineWidth = 1;

        ctx.beginPath();
        ctx.moveTo(padding, offset);
        ctx.lineTo(canvas.width - padding, offset);
        ctx.stroke();

        ctx.beginPath();
        ctx.moveTo(offset, padding);
        ctx.lineTo(offset, canvas.height - padding);
        ctx.stroke();
    }

    drawStars();
    drawStones();
}

function drawStars() {
    const starPoints = [7, 15, 23];
    ctx.fillStyle = "#5a3312";

    for (const x of starPoints) {
        for (const y of starPoints) {
            ctx.beginPath();
            ctx.arc(padding + x * cellSize, padding + y * cellSize, 4, 0, Math.PI * 2);
            ctx.fill();
        }
    }
}

function drawStones() {
    for (let y = 0; y < boardSize; y += 1) {
        for (let x = 0; x < boardSize; x += 1) {
            const value = state.board[y][x];
            if (value === emptyCell) {
                continue;
            }

            const centerX = padding + x * cellSize;
            const centerY = padding + y * cellSize;
            const gradient = ctx.createRadialGradient(centerX - 6, centerY - 6, 3, centerX, centerY, 16);

            if (value === blackStone) {
                gradient.addColorStop(0, "#666");
                gradient.addColorStop(1, "#111");
            } else {
                gradient.addColorStop(0, "#fff");
                gradient.addColorStop(1, "#cfcfcf");
            }

            ctx.beginPath();
            ctx.fillStyle = gradient;
            ctx.arc(centerX, centerY, 16, 0, Math.PI * 2);
            ctx.fill();
        }
    }
}

function updateInfo() {
    if (state.gameOver) {
        if (state.winner === blackStone) {
            statusEl.textContent = "对局结束：黑子获胜";
        } else if (state.winner === whiteStone) {
            statusEl.textContent = "对局结束：白子获胜";
        } else {
            statusEl.textContent = "对局结束：平局";
        }
    } else {
        statusEl.textContent = state.currentTurn === blackStone ? "当前轮到黑子" : "当前轮到白子";
    }

    turnTextEl.textContent = state.currentTurn === blackStone ? "黑子" : "白子";

    if (state.lastMove) {
        const role = state.lastMove.color === blackStone ? "黑子" : "白子";
        lastMoveEl.textContent = `${role} 落子：(${state.lastMove.x}, ${state.lastMove.y})`;
    } else {
        lastMoveEl.textContent = "暂无";
    }
}

function resetBoard() {
    state.board = Array.from({ length: boardSize }, () => Array(boardSize).fill(emptyCell));
    state.currentTurn = blackStone;
    state.gameOver = false;
    state.winner = emptyCell;
    state.lastMove = null;
    drawBoard();
    updateInfo();
}

function getGridPosition(event) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const mouseX = (event.clientX - rect.left) * scaleX;
    const mouseY = (event.clientY - rect.top) * scaleY;

    const x = Math.round((mouseX - padding) / cellSize);
    const y = Math.round((mouseY - padding) / cellSize);

    if (x < 0 || x >= boardSize || y < 0 || y >= boardSize) {
        return null;
    }

    return { x, y };
}

function placeStone(x, y, color) {
    if (state.gameOver || state.board[y][x] !== emptyCell) {
        return false;
    }

    state.board[y][x] = color;
    state.lastMove = { x, y, color };
    state.currentTurn = color === blackStone ? whiteStone : blackStone;
    drawBoard();
    updateInfo();
    return true;
}

async function requestState() {
    try {
        const response = await fetch("/gomoku/state");
        if (!response.ok) {
            return;
        }
        const data = await response.json();
        if (Array.isArray(data.board)) {
            state.board = data.board;
        }
        if (typeof data.nextTurn === "number") {
            state.currentTurn = data.nextTurn;
        }
        if (typeof data.gameOver === "boolean") {
            state.gameOver = data.gameOver;
        }
        if (typeof data.winner === "number") {
            state.winner = data.winner;
        }
        drawBoard();
        updateInfo();
    } catch (error) {
        console.warn("Failed to load gomoku state:", error);
    }
}

async function sendMove(x, y) {
    try {
        const response = await fetch("/gomoku/move", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ x, y }),
        });

        if (!response.ok) {
            return;
        }

        const data = await response.json();
        if (Array.isArray(data.board)) {
            state.board = data.board;
        }
        if (typeof data.nextTurn === "number") {
            state.currentTurn = data.nextTurn;
        }
        if (typeof data.gameOver === "boolean") {
            state.gameOver = data.gameOver;
        }
        if (typeof data.winner === "number") {
            state.winner = data.winner;
        }
        state.lastMove = { x, y, color: blackStone };
        drawBoard();
        updateInfo();
    } catch (error) {
        console.warn("Failed to send move:", error);
    }
}

async function requestAiMove() {
    try {
        const response = await fetch("/gomoku/ai-move", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
        });

        if (!response.ok) {
            return;
        }

        const data = await response.json();
        if (Array.isArray(data.board)) {
            state.board = data.board;
        }
        if (data.aiMove) {
            state.lastMove = {
                x: data.aiMove.x,
                y: data.aiMove.y,
                color: whiteStone,
            };
        }
        if (typeof data.nextTurn === "number") {
            state.currentTurn = data.nextTurn;
        }
        if (typeof data.gameOver === "boolean") {
            state.gameOver = data.gameOver;
        }
        if (typeof data.winner === "number") {
            state.winner = data.winner;
        }
        drawBoard();
        updateInfo();
    } catch (error) {
        console.warn("Failed to request AI move:", error);
    }
}

async function resetRemoteBoard() {
    try {
        await fetch("/gomoku/reset", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
        });
    } catch (error) {
        console.warn("Failed to reset remote board:", error);
    }
    resetBoard();
}

canvas.addEventListener("click", (event) => {
    const position = getGridPosition(event);
    if (!position) {
        return;
    }

    if (!placeStone(position.x, position.y, blackStone)) {
        return;
    }

    sendMove(position.x, position.y);
});

startBtn.addEventListener("click", resetRemoteBoard);
resetBtn.addEventListener("click", resetRemoteBoard);
aiBtn.addEventListener("click", requestAiMove);

drawBoard();
updateInfo();
requestState();
