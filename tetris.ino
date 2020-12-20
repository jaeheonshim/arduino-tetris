#include <LiquidCrystal.h>
#include <LedControl.h>

#define LEFT_BUTTON 22
#define RIGHT_BUTTON 24
#define SPEED_BUTTON 26
#define ROTATE_BUTTON 28
#define BUZZER 9

#define JOY_X 1
#define JOY_Y 2
#define JOY_SW 20

#define UNUSED_ANALOG 0 // for random seed

#define BLOCK_COUNT (sizeof(blocks) / sizeof(blocks[0]))

LedControl lc = LedControl(11, 10, 12, 1);
LiquidCrystal lcd = LiquidCrystal(2, 3, 4, 5, 6, 7);

struct point {
  int x;
  int y;
};

static int blocks[][4][8] = {
  {
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 2, 1, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0}
  },
  {
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 2, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0}
  },
  {
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 1, 2, 1, 0, 0, 0},
  },
  {
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0},
  },
  {
    {0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 1, 2, 0, 0, 0},
  },
  {
    {0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 1, 2, 1, 0, 0, 0},
  },
  {
    {0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 2, 1, 0, 0},
  },
};

void(* resetFunc) (void) = 0;

static int moving[12][8];
static int stationary[8][8];

static unsigned long lastUpdate;
static const unsigned long updateInterval = 1000;

static unsigned long lastInput;
static const unsigned long inputDelay = 120;

static int score;
static int level;

void setup() {
  // put your setup code here, to run once:
  lc.shutdown(0, false);
  lc.setIntensity(0, 1);

  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  pinMode(SPEED_BUTTON, INPUT);
  pinMode(ROTATE_BUTTON, INPUT);
  pinMode(UNUSED_ANALOG, INPUT);

  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);
  pinMode(JOY_SW, INPUT);

  randomSeed(analogRead(UNUSED_ANALOG));
  queueNewBlock();

  lcd.begin(16, 2);

  Serial.begin(9600);
}

void loop() {
  if (isGameOver()) {
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Game Over!");
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.print(score);
    for (int t = 0; t < 3; t++) {
      for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          lc.setLed(0, i, j, true);
        }
      }
      delay(500);
      render();
      delay(500);
    }

    delay(3000);
    resetFunc();
  }
  unsigned long currentMillis = millis();
  if (currentMillis - lastInput > inputDelay) {
    handleInput();
    lastInput = currentMillis;
  }
  if (currentMillis - lastUpdate > updateInterval) {
    updateState();
    renderLcd();
    lastUpdate = currentMillis;
  }
  render();
}

void handleInput() {
  Serial.print("Switch: ");
  Serial.println(digitalRead(JOY_SW));
  Serial.print("X: ");
  Serial.println(analogRead(JOY_X));
  Serial.print("Y: ");
  Serial.println(analogRead(JOY_Y));
  
  if (digitalRead(LEFT_BUTTON) || analogRead(JOY_X) < 150) {
    transformMoving(-1, 0);
  } else if (digitalRead(RIGHT_BUTTON) || analogRead(JOY_X) > 850) {
    transformMoving(1, 0);
  } else if (digitalRead(SPEED_BUTTON) || analogRead(JOY_Y) > 850) {
    lastUpdate -= updateInterval;
    score++;
  } else if (digitalRead(ROTATE_BUTTON) || analogRead(JOY_Y) < 150) {
    rotate90();
  }
}

void updateState() {
  if (isMovingAtBottom()) {
    handleAtBottom();
    queueNewBlock();
  }
  transformMoving(0, -1);

  updateRows();
}

void render() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, moving[i + 4][j] | stationary[i][j]);
    }
  }
}

void renderLcd() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Level: ");
  lcd.print(level);

  lcd.setCursor(1, 1);
  lcd.print("Score: ");
  lcd.print(score);
}

void updateRows() {
  int rowsUpdated = 0;

  for (int i = 0; i < 8; i++) {
    int validRow = 1;
    for (int j = 0; j < 8; j++) {
      if (!stationary[i][j]) {
        validRow = 0;
      }
    }

    if (validRow) {
      tone(BUZZER, 1000);
      delay(50);
      noTone(BUZZER);
      rowsUpdated++;
      for (int j = 0; j < 8; j++) {
        stationary[i][j] = 0;
      }

      int updated[8][8];
      for (int k = i; k >= 0; k--) {
        for (int l = 0; l < 8; l++) {
          if (k - 1 >= 0) {
            updated[k][l] = stationary[k - 1][l];
          }
        }
      }

      for (int k = i + 1; k < 12; k++) {
        for (int l = 0; l < 8; l++) {
          updated[k][l] = stationary[k][l];
        }
      }

      for (int k = 0; k < 8; k++) {
        for (int l = 0; l < 8; l++) {
          stationary[k][l] = updated[k][l];
        }
      }
    }
  }

  switch (rowsUpdated) {
    case 1:
      score += 40 * (level + 1);
      break;
    case 2:
      score += 100 * (level + 1);
      break;
    case 3:
      score += 300 * (level + 1);
      break;
    case 4:
      score += 1200 * (level + 1);
      break;
  }
}

void queueNewBlock() {
  int randomBlock = random(BLOCK_COUNT - 1);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {
      moving[i][j] = blocks[randomBlock][i][j];
    }
  }
}

int isMovingAtBottom() {
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      if (moving[i][j] && i == 11 || (moving[i][j] && i >= 4 && i + 1 < 12 && stationary[i - 4 + 1][j])) {
        return 1;
      }
    }
  }

  return 0;
}

int isGameOver() {
  for (int j = 0; j < 8; j++) {
    if (stationary[0][j]) {
      return 1;
    }
  }

  return 0;
}

void handleAtBottom() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      stationary[i][j] = stationary[i][j] || moving[i + 4][j];
      moving[i + 4][j] = 0;
    }
  }
}

void rotate90() {
  struct point pivot = { -1, -1};

  int updatedArr[12][8];

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      updatedArr[i][j] = 0;
      if (moving[i][j] == 2) {
        pivot.y = i;
        pivot.x = j;
      }
    }
  }

  if (pivot.y == -1 || pivot.x == -1) {
    return;
  }

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      if (moving[i][j] == 1) {
        struct point toCoordinate = {j - pivot.x, i - pivot.y};
        struct point transformedCoordinate = { -toCoordinate.y, toCoordinate.x};
        struct point transformedGrid = {transformedCoordinate.x + pivot.x, transformedCoordinate.y + pivot.y};

        if (transformedGrid.x >= 8 || transformedGrid.x < 0 || transformedGrid.y >= 12 || transformedGrid.y < 0 || (transformedGrid.y >= 4 && stationary[transformedGrid.y - 4][transformedGrid.x])) {
          return;
        }

        updatedArr[transformedGrid.y][transformedGrid.x] = 1;
      } else if (moving[i][j] == 2) {
        updatedArr[i][j] = 2;
      }
    }
  }

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      moving[i][j] = updatedArr[i][j];
    }
  }
}

void transformMoving(int x, int y) {
  y = -y;
  int updatedArr[12][8];
  int xValid = 1;

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      updatedArr[i][j] = 0;
    }
  }

  for (int i = 0; i < 12 && xValid; i++) {
    for (int j = 0; j < 8 && xValid; j++) {
      if (moving[i][j]) {
        if (j + x >= 8 || j + x < 0) {
          xValid = 0;
        }
      }
    }
  }

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      if (moving[i][j]) {
        if (i + y < 12 && i + y >= 0) {
          if (xValid) {
            if (i >= 4 && stationary[i - 4 + y][j + x]) {
              return;
            }
            updatedArr[i + y][j + x] = moving[i][j];
          } else {
            updatedArr[i + y][j] = moving[i][j];
          }
        }
      }
    }
  }

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      moving[i][j] = updatedArr[i][j];
    }
  }
}
