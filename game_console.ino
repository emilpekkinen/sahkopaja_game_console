#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(); 

// Input pins for joysticks
const int leftInputUD = 5; 
const int leftInputLR = 6; 
const int rightInputUD = 2; 
const int rightInputLR = 3; 

const int refPin = 4;
const int selectButtonPin = 7; // Button to select a game from the menu

int16_t screenHeight;
int16_t screenWidth;

enum GameState {
    GAME_MENU,
    GAME_PONG,
    GAME_SNAKE,
    GAME_OVER
};

GameState currentGameState = GAME_MENU;

// Menu variables
int selectedGameIndex = 0;
const char* gameList[] = {
    "Pong",
    "Snake"
};
int totalGames = 2;

// Pong variables
double paddleP1Y = 0;
double paddleP2Y = 0;
int oldPaddleP1Y;
int oldPaddleP2Y;
int ballDirectionX = 1;
int ballDirectionY = 1;
float ballSpeed = 160.0;
double ballX, ballY;
int oldBallX, oldBallY;

// Snake variables
const int cellSize = 10;
int gridCols; 
int gridRows; 

const int MAX_SNAKE_LENGTH = 100;
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength;
int snakeDirX = 0; 
int snakeDirY = 0;

int fruitX, fruitY;

// Snake timing
unsigned long lastSnakeMove = 0;
unsigned long snakeMoveInterval = 200;

// For game over timing
unsigned long gameOverTime = 0; // When game ended

// Inputs
int LUD;
int LLR;
int RUD;
int RLR;
int refVoltage= 3300;
long lastMillis;
double deltaTime;

void setup()
{
    Serial.begin(9600);
    delay(500);
    tft.init();
    tft.setRotation(1); // Set the display rotation

    screenHeight = tft.height();
    screenWidth = tft.width();

    pinMode(selectButtonPin, INPUT_PULLUP);

    tft.fillScreen(TFT_BLACK);

    gridCols = screenWidth / cellSize;
    gridRows = screenHeight / cellSize;

    lastMillis = millis();
}

void loop()
{
    long currentMillis = millis();
    deltaTime = (currentMillis - lastMillis) / 1000.0;
    readInputs();

    switch(currentGameState) {
        case GAME_MENU:
            menu();
            break;
        case GAME_PONG:
            pong();
            break;
        case GAME_SNAKE:
            snake();
            break;
        case GAME_OVER:
            gameOverScreen();
            break;
    }

    lastMillis = currentMillis;
    delay(25);
}

void readInputs()
{
    LUD = analogRead(leftInputUD);
    LLR = analogRead(leftInputLR);
    RUD = analogRead(rightInputUD);
    RLR = analogRead(rightInputLR);
    //refVoltage = analogRead(refPin);

    Serial.println(LUD);
    Serial.println(LLR);
    Serial.println(RUD);
    Serial.println(RLR);
    Serial.println(refVoltage);
    Serial.println();
}

//-------------------------------------
// MENU FUNCTIONS
//-------------------------------------
void menu()
{
    static int oldSelectedGameIndex = -1;
    handleMenuInput();
    if (oldSelectedGameIndex != selectedGameIndex) {
        drawMenu();
        oldSelectedGameIndex = selectedGameIndex;
    }
}

void handleMenuInput()
{
    int menuDelta = map(LUD, 0, refVoltage, -100, 100);
    static long lastMoveTime = 0;
    long now = millis();
    int moveDelay = 200; 

    if (now - lastMoveTime > moveDelay) {
        if (menuDelta > 20) {
            selectedGameIndex++;
            if (selectedGameIndex >= totalGames) selectedGameIndex = 0;
            lastMoveTime = now;
        } else if (menuDelta < -20) {
            selectedGameIndex--;
            if (selectedGameIndex < 0) selectedGameIndex = totalGames - 1;
            lastMoveTime = now;
        }
    }

    if (digitalRead(selectButtonPin) == LOW) {
        selectGame(selectedGameIndex);
    }

    int selectDelta = map(LLR, 0, refVoltage, -100, 100);
    if (selectDelta > 50 || selectDelta < -50) {
        selectGame(selectedGameIndex);
    }
}

void drawMenu()
{
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("Select a Game:");

    tft.setTextSize(2);
    int yStart = 50;
    for (int i = 0; i < totalGames; i++) {
        if (i == selectedGameIndex) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(20, yStart + i * 30);
        tft.print(gameList[i]);
    }
}

void selectGame(int index)
{
    if (strcmp(gameList[index], "Pong") == 0) {
        startPong();
    } else if (strcmp(gameList[index], "Snake") == 0) {
        startSnake();
    }
}

//-------------------------------------
// PONG FUNCTIONS
//-------------------------------------

int scoreP1 = 0;
int scoreP2 = 0;

void startPong()
{
    currentGameState = GAME_PONG;
    tft.fillScreen(TFT_BLACK);

    scoreP1 = 0;
    scoreP2 = 0;

    // Initialize paddles
    paddleP1Y = (screenHeight / 2) - 20;
    paddleP2Y = (screenHeight / 2) - 20;
    oldPaddleP1Y = (int)paddleP1Y;
    oldPaddleP2Y = (int)paddleP2Y;

    // Initialize ball in the center
    ballX = screenWidth / 2;
    ballY = screenHeight / 2;
    oldBallX = (int)ballX;
    oldBallY = (int)ballY;

    // Draw initial paddles and ball
    tft.fillRect(10, (int)paddleP1Y, 5, 40, TFT_WHITE);
    tft.fillRect(screenWidth - 15, (int)paddleP2Y, 5, 40, TFT_WHITE);
    tft.fillCircle((int)ballX, (int)ballY, 5, TFT_WHITE);

    // Draw initial scores
    drawPongScores();
}

void drawPongScores()
{
    // Clear old score area
    tft.fillRect(0, 0, screenWidth, 20, TFT_BLACK);

    // Print scores at top
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 0);
    tft.print("P1: ");
    tft.print(scoreP1);

    tft.setCursor(screenWidth - 80, 0);
    tft.print("P2: ");
    tft.print(scoreP2);
}

boolean inPaddle(int x, int y, int rectX, int rectY, int rectWidth, int rectHeight)
{
    if ((x >= rectX && x <= (rectX + rectWidth)) &&
        (y >= rectY && y <= (rectY + rectHeight))) {
        return true;
    }

    return false;
}

void pong()
{
    int paddleP1Delta = map(LUD, 0, refVoltage, -100, 100);
    int paddleP2Delta = map(RUD, 0, refVoltage, -100, 100);

    paddleP1Delta = -paddleP1Delta; //invert up down input
    paddleP2Delta = -paddleP2Delta;

    if (abs(paddleP1Delta) > 10) paddleP1Y += paddleP1Delta * deltaTime;
    if (abs(paddleP2Delta) > 10) paddleP2Y += paddleP2Delta * deltaTime;

    // Keep paddles within screen bounds
    if (paddleP1Y < 0) paddleP1Y = 0;
    if (paddleP1Y > screenHeight - 40) paddleP1Y = screenHeight - 40;

    if (paddleP2Y < 0) paddleP2Y = 0;
    if (paddleP2Y > screenHeight - 40) paddleP2Y = screenHeight - 40;

    // Erase old paddles
    tft.fillRect(10, oldPaddleP1Y, 5, 40, TFT_BLACK);
    tft.fillRect(screenWidth - 15, oldPaddleP2Y, 5, 40, TFT_BLACK);

    // Draw new paddles
    tft.fillRect(10, (int)paddleP1Y, 5, 40, TFT_WHITE);
    tft.fillRect(screenWidth - 15, (int)paddleP2Y, 5, 40, TFT_WHITE);

    oldPaddleP1Y = (int)paddleP1Y;
    oldPaddleP2Y = (int)paddleP2Y;

    moveBall();
}

void resetBall()
{
    // Reset ball to center
    ballX = screenWidth / 2;
    ballY = screenHeight / 2;
    ballDirectionX = (random(2) == 0) ? 1 : -1; // Random direction
    ballDirectionY = (random(2) == 0) ? 1 : -1; // Random direction
    oldBallX = (int)ballX;
    oldBallY = (int)ballY;

    // Erase old ball just in case
    tft.fillCircle(oldBallX, oldBallY, 5, TFT_BLACK);
    // Draw new ball
    tft.fillCircle((int)ballX, (int)ballY, 5, TFT_WHITE);
}

void moveBall()
{
    // Erase old ball
    tft.fillCircle(oldBallX, oldBallY, 5, TFT_BLACK);

    ballX += ballDirectionX * ballSpeed * deltaTime;
    ballY += ballDirectionY * ballSpeed * deltaTime;

    // Check collision with top/bottom walls
    if (ballY < 0 || ballY > screenHeight) {
        ballDirectionY = -ballDirectionY;
    }

    // Check if ball goes off left/right screen
    if (ballX < 0) {
        // Player 2 scores
        scoreP2++;
        drawPongScores();
        if (scoreP2 == 10) {
            gameOver();
            return;
        }
        resetBall();
        return;
    }

    if (ballX > screenWidth) {
        // Player 1 scores
        scoreP1++;
        drawPongScores();
        if (scoreP1 == 10) {
            gameOver();
            return;
        }
        resetBall();
        return;
    }

    // Check collision with paddles:
    // Left paddle: x=10, y=paddleP1Y, width=5, height=40
    if (inPaddle((int)ballX, (int)ballY, 10, (int)paddleP1Y, 5, 40)) {
        ballDirectionX = -ballDirectionX;
        ballX = 10 + 5 + 1; // Move ball out of paddle to avoid sticking
    }

    // Right paddle: x=screenWidth-15, y=paddleP2Y, width=5, height=40
    if (inPaddle((int)ballX, (int)ballY, screenWidth - 15, (int)paddleP2Y, 5, 40)) {
        ballDirectionX = -ballDirectionX;
        ballX = screenWidth - 15 - 1; // Move ball out of paddle
    }

    // Draw new ball
    tft.fillCircle((int)ballX, (int)ballY, 5, TFT_WHITE);

    oldBallX = (int)ballX;
    oldBallY = (int)ballY;
}


//-------------------------------------
// SNAKE FUNCTIONS
//-------------------------------------
void startSnake()
{
    currentGameState = GAME_SNAKE;
    tft.fillScreen(TFT_BLACK);

    snakeLength = 5;
    int startX = gridCols / 2;
    int startY = gridRows / 2;
    for (int i = 0; i < snakeLength; i++) {
        snakeX[i] = startX - i; 
        snakeY[i] = startY;
    }

    snakeDirX = 1;
    snakeDirY = 0;

    placeFruit();
    drawSnake();
    drawFruit();
}

void snake()
{
    handleSnakeInput();

    unsigned long now = millis();
    if (now - lastSnakeMove > snakeMoveInterval) {
        lastSnakeMove = now;
        updateSnake();
    }
}

void handleSnakeInput()
{
    int upDown = map(LUD, 0, refVoltage, -100, 100);
    upDown = -upDown;
    int leftRight = map(LLR, 0, refVoltage, -100, 100);
    int threshold = 30;


    // Up
    if (upDown < -threshold && snakeDirY == 0) {
        snakeDirX = 0;
        snakeDirY = -1;
    }
    // Down
    else if (upDown > threshold && snakeDirY == 0) {
        snakeDirX = 0;
        snakeDirY = 1;
    }
    // Left
    else if (leftRight < -threshold && snakeDirX == 0) {
        snakeDirX = -1;
        snakeDirY = 0;
    }
    // Right
    else if (leftRight > threshold && snakeDirX == 0) {
        snakeDirX = 1;
        snakeDirY = 0;
    }
}

void updateSnake()
{
    int newHeadX = snakeX[0] + snakeDirX;
    int newHeadY = snakeY[0] + snakeDirY;

    // Check wall collision
    if (newHeadX < 0 || newHeadX >= gridCols || newHeadY < 0 || newHeadY >= gridRows) {
        gameOver();
        return;
    }

    // Check self-collision
    for (int i = 0; i < snakeLength; i++) {
        if (snakeX[i] == newHeadX && snakeY[i] == newHeadY) {
            gameOver();
            return;
        }
    }

    // Move body
    for (int i = snakeLength - 1; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
    }

    snakeX[0] = newHeadX;
    snakeY[0] = newHeadY;

    // Check fruit collision
    if (newHeadX == fruitX && newHeadY == fruitY) {
        snakeLength++;
        snakeX[snakeLength - 1] = snakeX[snakeLength - 2];
        snakeY[snakeLength - 1] = snakeY[snakeLength - 2];
        placeFruit();
    }

    tft.fillScreen(TFT_BLACK);
    drawSnake();
    drawFruit();
}

void drawSnake()
{
    for (int i = 0; i < snakeLength; i++) {
        tft.fillRect(snakeX[i]*cellSize, snakeY[i]*cellSize, cellSize, cellSize, TFT_GREEN);
    }
}

void drawFruit()
{
    tft.fillRect(fruitX*cellSize, fruitY*cellSize, cellSize, cellSize, TFT_RED);
}

void placeFruit()
{
    fruitX = random(gridCols);
    fruitY = random(gridRows);
    for (int i = 0; i < snakeLength; i++) {
        if (snakeX[i] == fruitX && snakeY[i] == fruitY) {
            placeFruit();
            return; 
        }
    }
}

//-------------------------------------
// GAME OVER FUNCTIONS
//-------------------------------------
void gameOver()
{
    currentGameState = GAME_OVER;
    gameOverTime = millis();

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(screenWidth/2 - 50, screenHeight/2 - 10);
    tft.print("Game Over");
}

void gameOverScreen()
{
    // Check if 5 seconds have passed
    unsigned long now = millis();
    if (now - gameOverTime > 2500) {
        // After 2.5 seconds, go back to menu
        currentGameState = GAME_MENU;
        tft.fillScreen(TFT_BLACK);
        drawMenu();
    }
}
