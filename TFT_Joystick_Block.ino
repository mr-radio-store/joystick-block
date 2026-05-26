/*
Block Game 
TFT LCD ST7735S & Analog Joystick

1. Wire Connection TFT + UNO
ST7735 Pin	Arduino Pin
VCC	        5V or 3.3V
GND	        GND
CS	        D10
RST	        D9
DC (A0)	D8
SDA (MOSI)	D11
SCK	        D13
2. Wiring for Analog Joystick + Arduino Uno
Joystick Pin	Arduino Uno Pin	Function
VCC	            5V	            Power supply
GND	            GND	            Ground
VRx	            A0	            Horizontal analog input
VRy	            A1	            Vertical analog input
SW	            D2 (optional)	Button (digital read)
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS     10
#define TFT_RST    9
#define TFT_DC     8
#define JOY_X      A0
#define JOY_BTN    2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define GRID_COLS 10
#define GRID_ROWS 20
#define CELL_SIZE 8

int grid[GRID_ROWS][GRID_COLS] = {0};
int score = 0;
bool gameOver = false;

// Define 5 block shapes (4x4 matrices)
const byte blocks[5][4][4] = {
  // I shape
  {
    {0, 0, 0, 0},
    {1, 1, 1, 1},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  // O shape
  {
    {0, 1, 1, 0},
    {0, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  // L shape
  {
    {0, 0, 1, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  // T shape
  {
    {0, 1, 0, 0},
    {1, 1, 1, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  },
  // S shape
  {
    {0, 1, 1, 0},
    {1, 1, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
  }
};

int curBlock[4][4];
int blockX = 3;
int blockY = 0;

unsigned long lastDrop = 0;
int dropSpeed = 500;

void drawCell(int x, int y, uint16_t color) {
  tft.fillRect(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, color);
}

void drawGrid() {
  for (int y = 0; y < GRID_ROWS; y++) {
    for (int x = 0; x < GRID_COLS; x++) {
      drawCell(x, y, grid[y][x] ? ST77XX_BLUE : ST77XX_BLACK);
    }
  }
}

void copyBlock(int dst[4][4], const byte src[4][4]) {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      dst[i][j] = src[i][j];
}

void spawnBlock() {
  int shape = random(5);
  copyBlock(curBlock, blocks[shape]);
  blockX = 3;
  blockY = 0;

  // Check game over
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++)
      if (curBlock[y][x] && grid[blockY + y][blockX + x]) {
        gameOver = true;
      }
}

bool canMove(int dx, int dy) {
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (curBlock[y][x]) {
        int newX = blockX + x + dx;
        int newY = blockY + y + dy;
        if (newX < 0 || newX >= GRID_COLS || newY >= GRID_ROWS) return false;
        if (newY >= 0 && grid[newY][newX]) return false;
      }
    }
  }
  return true;
}

void placeBlock() {
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++)
      if (curBlock[y][x]) {
        int gx = blockX + x;
        int gy = blockY + y;
        if (gx >= 0 && gx < GRID_COLS && gy >= 0 && gy < GRID_ROWS)
          grid[gy][gx] = 1;
      }
}

void clearLines() {
  for (int y = GRID_ROWS - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < GRID_COLS; x++)
      if (!grid[y][x]) full = false;
    if (full) {
      for (int j = y; j > 0; j--)
        for (int x = 0; x < GRID_COLS; x++)
          grid[j][x] = grid[j - 1][x];
      for (int x = 0; x < GRID_COLS; x++) grid[0][x] = 0;
      y++;
      score++;
    }
  }
}

void drawBlock(int color) {
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++)
      if (curBlock[y][x]) {
        int gx = blockX + x;
        int gy = blockY + y;
        if (gx >= 0 && gx < GRID_COLS && gy >= 0 && gy < GRID_ROWS)
          drawCell(gx, gy, color);
      }
}

void resetGame() {
  memset(grid, 0, sizeof(grid));
  gameOver = false;
  score = 0;
  spawnBlock();
  tft.fillScreen(ST77XX_BLACK);
}

void setup() {
  pinMode(JOY_BTN, INPUT_PULLUP);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  randomSeed(analogRead(A2));
  resetGame();
}

void loop() {
  if (gameOver) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(20, 50);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.print("Game Over");
    delay(2000);
    resetGame();
    return;
  }

  drawGrid();
  drawBlock(ST77XX_GREEN);

  // Move input
  int xVal = analogRead(JOY_X);
  if (xVal < 300 && canMove(-1, 0)) {
    drawBlock(ST77XX_BLACK);
    blockX--;
    delay(150);
  } else if (xVal > 700 && canMove(1, 0)) {
    drawBlock(ST77XX_BLACK);
    blockX++;
    delay(150);
  }

  if (digitalRead(JOY_BTN) == LOW) {
    while (canMove(0, 1)) {
      drawBlock(ST77XX_BLACK);
      blockY++;
    }
    delay(150);
  }

  // Drop
  if (millis() - lastDrop > dropSpeed) {
    if (canMove(0, 1)) {
      drawBlock(ST77XX_BLACK);
      blockY++;
    } else {
      placeBlock();
      clearLines();
      spawnBlock();
    }
    lastDrop = millis();
  }

  drawBlock(ST77XX_GREEN);

  // Draw score
  tft.setCursor(2, 2);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.print("Score: ");
  tft.print(score);
}
