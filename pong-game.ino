/*
  Made with the help of 
    https://github.com/Bodmer/TFT_eSPI/blob/master/examples/320%20x%20240/TFT_Pong/TFT_Pong.ino
  and 
    https://github.com/Bodmer/TFT_eSPI/blob/master/examples/320%20x%20240/TFT_Starfield/TFT_Starfield.ino
*/

#include <TFT_eSPI.h>
#include <SPI.h>

// Colors
#define BLACK 0x0000
#define WHITE 0xFFFF

#define buzzer 4

// Config buttons
#define pause_button 10
#define reset_button 11

// Player 1 direction buttons
#define p_one_down_button 12
#define p_one_up_button 13

// Player 2 direction buttons
#define p_two_down_button 14
#define p_two_up_button 15


TFT_eSPI tft = TFT_eSPI();

// Starfield variables
#define NSTARS 1024

uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};

uint8_t za, zb, zc, zx;

uint8_t __attribute__((always_inline)) rng() {
  zx++;
  za = (za^zc^zx);
  zb = (zb+za);
  zc = ((zc+(zb>>1))^za);
  return zc;
}

// Conditions
bool paused = false;
bool level3SoundPlayed = false;

// Scores
int16_t p_one_score = 0;
int16_t p_two_score = 0;

int16_t level = 1;
int16_t difficulty = 1;

// Count touches on the ball, tracks when to increase the level
int16_t p_one_touch;
int16_t p_two_touch;

// Display size
int16_t h = 240;
int16_t w = 320;

// Paddle size
int16_t paddle_h = 50;
int16_t paddle_w = 5;

// Paddles spawn y axis
int16_t p_one_paddle_y = (h / 2) - (paddle_h / 2);  // Player 1 (Right)      // (240 / 2) - (50 / 2)
int16_t p_two_paddle_y = (h / 2) - (paddle_h / 2);  // Player 2 (Left)      // (240 / 2) - (50 / 2)

// Ball speed
int dly = 2;

// Ball spawn coordinates, Erase ball spawn coordinates
int16_t ball_x = 2;
int16_t ball_y = 2;
int16_t oldball_x = 2;
int16_t oldball_y = 2;

// Ball direction
int16_t ball_dx = 1;
int16_t ball_dy = 1;

// Ball size
int16_t ball_w = 6;
int16_t ball_h = 6;

// Dashed lines in the middle
int16_t dashline_h = 4;
int16_t dashline_w = 2;
int16_t dashline_n = h / dashline_h;
int16_t dashline_x = w / 2 - 1;

void setup() {
  // Display settings
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  pinMode(p_one_up_button, INPUT_PULLUP);
  pinMode(p_one_down_button, INPUT_PULLUP);

  pinMode(p_two_up_button, INPUT_PULLUP);
  pinMode(p_two_down_button, INPUT_PULLUP);

  pinMode(pause_button, INPUT_PULLUP);
  pinMode(reset_button, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);

  // Starfield
  za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);

  midline();
}

void loop() {
  // Check if the pause button is pressed
  if (paused) {
    starfield();

    // If pause is pressed again, exit
    if (digitalRead(pause_button) == LOW) {
      tft.fillScreen(BLACK);
      
      paused = false;

      delay(500);
    }
    
    // If reset button pressed, return to initial state
    if (digitalRead(reset_button) == LOW) {
      resetGame();

      tone(buzzer, 300, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 300, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 300, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 300, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);

      delay(1000);
    }
  } 
  
  else {
    // Check if pause is pressed
    if (digitalRead(pause_button) == LOW) {
      paused = true;
    }

    // Write Level and Scores
    tft.drawString(String(p_one_score), (w / 2) - 50, (h / 2) - 5, 2);
    tft.drawString(String(p_two_score), (w / 2) + 50, (h / 2) - 5, 2);
    tft.drawString("Level " + String(level), (w / 2) - 20, 0, 2);

    // If players reach level that are multiple of 3, play a sound
    if (level % 3 == 0 && !level3SoundPlayed) {
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 800, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 800, 100);
      delay(50);
      tone(buzzer, 600, 100);
      delay(50);
      tone(buzzer, 800, 100);
      delay(50);

      level3SoundPlayed = true;
    }

    if (level % 3 != 0) {
      level3SoundPlayed = false;
    }

    paddle(5 + difficulty);   // Change the value to increase speed of the paddle
    midline();
    ball(4 + difficulty);     // Change the value to increase speed of the ball
  }
}

void midline() {
  if ((ball_x < dashline_x - ball_w) && (ball_x > dashline_x + dashline_w)) return;

  tft.startWrite();
  tft.setAddrWindow(dashline_x, 0, dashline_w, h);
  
  for(int16_t i = 0; i < dashline_n; i+=2) {
    tft.pushColor(WHITE, dashline_w * dashline_h);
    tft.pushColor(BLACK, dashline_w * dashline_h);
  }

  tft.endWrite();
}

void paddle(int16_t paddle_speed) {
  // Player 1
  tft.fillRect(0, p_one_paddle_y, paddle_w, paddle_h, WHITE);
  // Player 2
  tft.fillRect((w - paddle_w), p_two_paddle_y, paddle_w, paddle_h, WHITE);


  // Left Player movements
  
  // Player 1 up movement
  if (digitalRead(p_one_up_button) == LOW && p_one_paddle_y + paddle_h < h) {
    // erase past position
    tft.fillRect(0, p_one_paddle_y, paddle_w, paddle_h, BLACK);
    p_one_paddle_y += paddle_speed; // Move paddle up
    // add new position
    tft.fillRect(0, p_one_paddle_y, paddle_w, paddle_h, WHITE);
  }

  // Player 1 down movement
  if (digitalRead(p_one_down_button) == LOW && p_one_paddle_y > 0) {
    // erase past position
    tft.fillRect(0, p_one_paddle_y, paddle_w, paddle_h, BLACK);
    p_one_paddle_y -= paddle_speed; // Move paddle down
    // add new position
    tft.fillRect(0, p_one_paddle_y, paddle_w, paddle_h, WHITE);
  }


  // Right Player movements
  
  // Player 2 up movement
  if (digitalRead(p_two_up_button) == LOW && p_two_paddle_y + paddle_h < h) {
    // erase past position
    tft.fillRect((w - paddle_w), p_two_paddle_y, paddle_w, paddle_h, BLACK);
    p_two_paddle_y += paddle_speed; // Move paddle up
    // add new position
    tft.fillRect((w - paddle_w), p_two_paddle_y, paddle_w, paddle_h, WHITE);
  }

  // Player 2 down movement
  if (digitalRead(p_two_down_button) == LOW && p_two_paddle_y > 0) {
    // erase past position
    tft.fillRect((w - paddle_w), p_two_paddle_y, paddle_w, paddle_h, BLACK);
    p_two_paddle_y -= paddle_speed; // Move paddle down
    // add new position
    tft.fillRect((w - paddle_w), p_two_paddle_y, paddle_w, paddle_h, WHITE);
  }

  delay(100);
}

void ball(int16_t multiplier) {
  int16_t speed_multiplier = multiplier; // 5 is the default speed

  // Erase the previous ball position
  tft.fillRect(oldball_x, oldball_y, ball_w, ball_h, BLACK);

  // Update ball position
  ball_x = ball_x + ball_dx * speed_multiplier;
  ball_y = ball_y + ball_dy * speed_multiplier;

  // Ball collision with Player 1 (Right Paddle)
  if (ball_dx == -1 && ball_x <= paddle_w && ball_y + ball_h >= p_one_paddle_y && ball_y <= p_one_paddle_y + paddle_h) {
    ball_dx = -ball_dx;  // Reverse horizontal direction
    dly = random(5);     // Randomize ball speed on collision
    tone(buzzer, 1000, 100);
    p_one_touch++;
    
    // Condition to check if both players touched the ball
    if (p_one_touch == 3 && p_two_touch == 3) {
      difficulty += 2;
      level += 1;

      p_one_touch = 0;
      p_two_touch = 0;
    }
  } 
  
  // Ball collision with Player 2 (Left Paddle)
  else if (ball_dx == 1 && ball_x + ball_w >= w - paddle_w && ball_y + ball_h >= p_two_paddle_y && ball_y <= p_two_paddle_y + paddle_h) {
    ball_dx = -ball_dx;  // Reverse horizontal direction
    dly = random(5);     // Randomize ball speed on collision
    tone(buzzer, 1000, 100);
    p_two_touch++;
    
    // Condition to check if both players touched the ball
    if (p_one_touch == 3 && p_two_touch == 3) {
      difficulty += 2;
      level += 1;

      p_one_touch = 0;
      p_two_touch = 0;
    }
  } 

  // Ball goes out of bounds (either side)
  else if ((ball_dx == 1 && ball_x >= w) || (ball_dx == -1 && ball_x + ball_w <= 0)) {
    // Check the side on which the ball touched, mark a point
    if (ball_x >= w) {
      p_one_score++;
    } 
    if ((ball_x + ball_w) <= 0) {
      p_two_score++;
    }

    dly = 5;  // Reset delay (ball speed) when out of bounds
    ball_x = w / 2 - ball_w / 2;  // Reset ball to center horizontally
    ball_y = h / 2 - ball_h / 2;  // Reset ball to center vertically
    ball_dx = (random(2) == 0) ? -1 : 1;  // Randomize initial direction after reset

    tone(buzzer, 100, 500);
    
    level = 1;
    difficulty = 1;
  }

  // Ball collision with top or bottom of the screen
  if (ball_y > h - ball_h || ball_y < 0) {
    ball_dy = -ball_dy;  // Reverse vertical direction

    tone(buzzer, 300, 50);
  }

  // Draw the ball at its new position
  tft.fillRect(ball_x, ball_y, ball_w, ball_h, WHITE);

  // Save the ball's current position for the next frame
  oldball_x = ball_x;
  oldball_y = ball_y;

  // Add delay to control ball speed
  delay(dly);
}

void starfield() {
  unsigned long t0 = micros();
  uint8_t spawnDepthVariation = 255;
      
  for(int i = 0; i < NSTARS; ++i) {
    if (sz[i] <= 1) {
      sx[i] = 160 - 120 + rng();
      sy[i] = rng();
      sz[i] = spawnDepthVariation--;
    }

    else {
      int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
      int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

      tft.drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

      sz[i] -= 2;
          
      if (sz[i] > 1) {
        int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
        int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;
      
        if (screen_x >= 0 && screen_y >= 0 && screen_x < 320 && screen_y < 240) {
          uint8_t r, g, b;
          r = g = b = 255 - sz[i];
          tft.drawPixel(screen_x, screen_y, tft.color565(r,g,b));
        }

        else
          sz[i] = 0;
      }
    }
  }
}

void resetGame() {
  // Reset game state variables
  p_one_score = 0;
  p_two_score = 0;
  
  level = 1;
  difficulty = 1;
  
  p_one_touch = 0;
  p_two_touch = 0;

  paused = false;

  // Reset paddle positions
  p_one_paddle_y = (h / 2) - (paddle_h / 2);
  p_two_paddle_y = (h / 2) - (paddle_h / 2);

  // Reset ball position and direction
  ball_x = w / 2 - ball_w / 2;
  ball_y = h / 2 - ball_h / 2;
  ball_dx = (random(2) == 0) ? -1 : 1;  // Random initial direction
  ball_dy = 1;

  // Clear the screen
  tft.fillScreen(BLACK);

  // Redraw midline, paddles, and score
  midline();
  paddle(4 + difficulty);

  tft.drawString(String(p_one_score), (w / 2) - 50, (h / 2) - 5, 2);
  tft.drawString(String(p_two_score), (w / 2) + 50, (h / 2) - 5, 2);
  tft.drawString("Level " + String(level), (w / 2) - 20, 0, 2);
}
