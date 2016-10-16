/*
 * Copyright (c) 2016 @icodealot
 *
 * General TODOs:
 * - More enemy types / bosses maybe
 * - Story
 * - Ship mods / different ships
 * - Cleanup code in general and drop some FOR loops...maybe.
 * - EEPROM to save audio preferences and high score
 *
 */

#include <Arduboy.h>
#include <EEPROM.h>
#include "GameTypes.h"
#include "GameTunes.h"
#include "GameArt.h"

Arduboy arduboy;

#define FRAMERATE 60
#define MAX_SHIPS 10  // max enemy sprites on screen
#define MAX_SHOTS 20  // max shot sprites on screen
#define MAX_BOOMS 3
#define MAX_OBJS 1

SHIP ship = {10, 25, 13, 0, 5, 0, 4, 0};
SHOT shots[MAX_SHOTS];
ENEMY enemies[MAX_SHIPS];
SCNOBJ objects[MAX_SHOTS];
SCNOBJ obstacles[MAX_OBJS];
SCNOBJ_ANIMATED booms[MAX_BOOMS];

int boomIndex = 0;
int shotIndex = 0;
int enemyIndex = 0;
int objIndex = 0;

int levelIndex = 1;
int enemySpawnRate = 800 ;
char fireRate = 5;
int enemyVelocity = 1;
int gameMode = arcade;

bool mute = false;

unsigned long lTime;        // current game loop time
unsigned long shotTime = 0;
unsigned long spawnTime = 0;

void setup() {
  arduboy.start();
  arduboy.setFrameRate(FRAMERATE);
  arduboy.tunes.playScore(themeSong);
  titleScreen();
}

void loop() {
  if (arduboy.nextFrame()) {
    arduboy.clearDisplay();
    lTime = millis();
    drawLayer4(); // player
    drawLayer2(); // level objects
    drawLayer3(); // enemies
    drawLayer1(); // game text
    levelCheck(); 
    arduboy.display();
  }
}

void levelCheck() {
  if (ship.life <= 0) {
    if (!mute) arduboy.tunes.playScore(badGuysWin);
    waitBox(44,18,"The End", 35, 30, "Try Again!", false, 1); //TODO: add high score and save to EEPROM
    ship = {10, 25, 13, 0, 5, 0, 4, 0};
    levelIndex = 1;
    titleScreen();
  } else if (ship.score >= (levelIndex * 30 + (levelIndex - 1) * 20)) {
    if (!mute) arduboy.tunes.playScore(goodGuysWin);
    waitBox(53,20,"Level", 45, 30, "Passed!", true, 1);
    levelIndex += 1;
    resetAll();
    loadLevel(levelIndex);
  }
}

void resetAll() {
  resetEnemies();
  resetShots();
  resetExplosions();
}

void resetExplosions() {
  for (int i = 0; i < MAX_BOOMS; i++) {
    booms[i].state = 0;
    booms[i].x = 200;
    booms[i].y = -20;
  }
}

void resetEnemies() {
  for (int i = 0; i < MAX_SHIPS; i++) {
    enemies[i].state = 0;
    enemies[i].x = 200;
    enemies[i].y = -20;
  }
}

void resetShots() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    shots[i].state = 0;
    shots[i].x = 200;
    shots[i].y = -20;
  }
}

// -----------------------------------------------
// LAYER 1: GAME TEXT LAYER
// -----------------------------------------------

void drawLayer1() {
  drawScore();
  drawLife();
}

void drawScore() {
  message(0,0,String(ship.score));
}

void drawLife() {
  char heart = 3; // char 3 -> String == heart
  for (int i = 0; i < ship.life; i++) {
    message(120 - (i * 8),0,String(heart));
  }
}

// -----------------------------------------------
// LAYER 2: GAME LEVEL
// -----------------------------------------------

void drawLayer2() {
  drawStars();
}

// -----------------------------------------------
// LAYER 3: ENEMIES
// -----------------------------------------------

void drawLayer3() {
  // draw some enemies here
  updateObjects();
  updateEnemies();
  drawExplosions();
}


// -----------------------------------------------
// LAYER 4: PLAYER
// -----------------------------------------------

void drawLayer4() {
  updateMods(); // update mods before moving -- nice lag effect
  moveShip();
  fireShot();
  updateLife();
}

// ***********************************************
// SCENE OBJECT rendering functions
// ***********************************************

void drawExplosions() {
  for (int i = 0; i < MAX_BOOMS; i++) {
    if (booms[i].frame < 12 && booms[i].state == 1) { 
      booms[i].frame += 1;
      arduboy.drawPixel(booms[i].x - booms[i].frame, booms[i].y - booms[i].frame, WHITE);
      arduboy.drawPixel(booms[i].x - booms[i].frame, booms[i].y + booms[i].frame, WHITE);
      arduboy.drawPixel(booms[i].x + booms[i].frame, booms[i].y - booms[i].frame, WHITE);
      arduboy.drawPixel(booms[i].x + booms[i].frame, booms[i].y + booms[i].frame, WHITE);
      arduboy.drawCircle(booms[i].x, booms[i].y, 1 * booms[i].frame / 2, WHITE);
      arduboy.drawCircle(booms[i].x, booms[i].y, 1 * booms[i].frame, WHITE);
    } else {
      booms[i].state = 0;
      booms[i].frame = 0;
    }
  }
}

void drawStars() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (objects[i].x > 0) { 
      objects[i].x -= 1;
      arduboy.drawPixel(objects[i].x, objects[i].y, WHITE);
    } else {
      if (random(0,50) == 25) {
        objects[i].x = WIDTH;
        objects[i].y = random(0, 55);
      }
    }
  }
}

void initStars() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (objects[i].x <= 0) { 
        objects[i].x = random(0,WIDTH);
        objects[i].y = random(0, 55);
    }
  }
}

// ***********************************************
// OBJECT rendering functions
// ***********************************************

void updateObjects() {

  if (spawnTime <= lTime) { //currently tied to enemy spawn time and then random
    if (random(1,max(20 - levelIndex,levelIndex)) == levelIndex) {
      objIndex = (objIndex + 1) % MAX_OBJS;
    
      if (obstacles[objIndex].state == 0) {
        obstacles[objIndex].x = (ship.x) + random(0, 10);
        obstacles[objIndex].y = -13;
        obstacles[objIndex].w = 8;
        obstacles[objIndex].velocity = 1;
        obstacles[objIndex].state = 1;
      }
    }
  }
  for (int i = 0; i < MAX_OBJS; i++) {
    if (obstacles[i].state == 1) { 
      if (obstacles[i].y >= HEIGHT) {
        obstacles[i].state = 0;
        obstacles[i].y = -20;
        continue;
      }
      obstacles[i].y += obstacles[i].velocity;

      arduboy.fillCircle(obstacles[i].x, obstacles[i].y,obstacles[i].w, WHITE);
    }
  }
}

// ***********************************************
// ENEMY rendering functions
// ***********************************************

void updateEnemies() {
  
  if (spawnTime <= lTime) {
    enemyIndex = (enemyIndex + 1) % MAX_SHIPS;
    spawnTime = millis() + enemySpawnRate;
    if (enemies[enemyIndex].state == 0) {
      enemies[enemyIndex].x = WIDTH;
      enemies[enemyIndex].y = random(0, 50);
      enemies[enemyIndex].yi = enemies[enemyIndex].y;
      enemies[enemyIndex].w = 13;
      enemies[enemyIndex].health = 100;
      enemies[enemyIndex].velocity = enemyVelocity;
      enemies[enemyIndex].behavior = 1;
      enemies[enemyIndex].state = 1;
    }
  }
  
  for (int i = 0; i < MAX_SHIPS; i++) {
    if (enemies[i].state == 1) { 
      if (enemies[i].x <= -10) {
        enemies[i].state = 0;
        enemies[i].x = 200;
        continue;
      }
      enemies[i].x -= enemies[i].velocity;
      enemies[i].y = enemies[i].yi + 10*sin(enemies[i].x * PI / 60);
      
      int id = gotHit(enemies[i].x, enemies[i].y);
      
      if (id >= 0) {
        if (!mute) arduboy.tunes.playScore(point);
        enemies[i].health = 0;
        enemies[i].state = 0;
        explodeAt(enemies[i].x, enemies[i].y);
        enemies[i].x = 200;
        enemies[i].y = -10;
        ship.score += 1 ;
        shots[id].state = 0;
        shots[id].x = 200;
        shots[id].y = -20;
      } else {
        arduboy.drawBitmap(enemies[i].x, enemies[i].y, Enemy1, enemies[i].w, enemies[i].w, WHITE);
      }
    }
  }
}

int gotHit(int x, int y) {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (abs(shots[i].x - (x + 7)) <= 5 && abs(shots[i].y - (y + 7)) <= 5) {
      return i;
    }
  }
  return -1;
}

// ***********************************************
// SHIP rendering functions
// ***********************************************

void clearShip() {
  arduboy.fillRect(ship.x,ship.y,ship.w,ship.w, BLACK);
}

void drawShip() {
  clearShip();
  arduboy.drawBitmap(ship.x, ship.y, Ship3, ship.w, ship.w, WHITE);
}

void bankLeft() {
  clearShip();
  arduboy.drawBitmap(ship.x, ship.y, Ship3BankLeft, ship.w, ship.w, WHITE);
}

void bankRight() {
  clearShip();
  arduboy.drawBitmap(ship.x, ship.y, Ship3BankRight, ship.w, ship.w, WHITE);
}

void updateMods() {
  // MOD 1 - Double Trouble
  if (ship.mod == 1) {
    arduboy.fillRect(ship.x + 15,ship.y - 5, 3, 3, WHITE);
    arduboy.fillRect(ship.x + 15,ship.y + 15, 3, 3, WHITE);
  }
}

void moveShip() {
  if (arduboy.pressed(UP_BUTTON)) {
    ship.y -= (ship.y <= ship.velocity / 2 ? 0 : ship.velocity / 2);
    bankLeft();
  } else if (arduboy.pressed(DOWN_BUTTON)) {
    ship.y += (ship.y >= HEIGHT - ship.w - ship.velocity / 2 ? 0 : ship.velocity / 2);
    bankRight();
  } else if (arduboy.pressed(RIGHT_BUTTON)) {
    ship.x += (ship.x >= WIDTH - ship.w - ship.velocity / 2 ? 0 : ship.velocity);
    drawShip();
  } else if (arduboy.pressed(LEFT_BUTTON)) {
    ship.x -= (ship.x <= ship.velocity ? 0 : ship.velocity);
    drawShip();
  } else {
    drawShip();
  }
}

void fireShot() {
  
  if (shotTime <= lTime && (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON))) {
    
    shotIndex = (shotIndex + 1) % MAX_SHOTS;
    shotTime = millis() + (1000/fireRate);
    shots[shotIndex].x = ship.x + ship.w / 2;
    shots[shotIndex].y = ship.y + ship.w / 2;
    
    if (arduboy.pressed(A_BUTTON)) {
      shots[shotIndex].state = 1;
      shots[shotIndex].velocity = 4;
      message(10,55,"Pew!");
    } else if (arduboy.pressed(B_BUTTON)) {
      shots[shotIndex].state = 2;
      shots[shotIndex].velocity = 1;
      message(10,55,"Pew Pew!");
    }
  }
  
  for (int i = 0; i < MAX_SHOTS; i++) {
    
    if (shots[i].x >= WIDTH) {
      shots[i].state = 0;
      shots[i].x = 200;
      continue;
    }
        
    if (shots[i].state == 1) { 
      shots[i].x += shots[i].velocity;
      arduboy.drawCircle(shots[i].x, shots[i].y,4, WHITE);
    } else if (shots[i].state == 2) {
      shots[i].x += shots[i].velocity;
      arduboy.drawBitmap(shots[i].x, shots[i].y, ShotType2, 5, 3, WHITE);
    }
  }
}

void updateLife() {
  // check for damage and update here
  for (int i = 0; i < MAX_SHIPS; i++) {
    if (abs((enemies[i].x+7) - (ship.x + 6)) <= 7 && abs((enemies[i].y+7) - (ship.y + 7)) <= 6) {
      if(random(1,20) == 7) {
        if (!mute) arduboy.tunes.playScore(point); // TODO: play negative hit sound instead of point sound
        enemies[i].health = 0;
        enemies[i].state = 0;
        explodeAt(enemies[i].x, enemies[i].y);
        enemies[i].x = 200;
        enemies[i].y = -10;
        ship.score += 1 ;
        ship.life = max(ship.life - 1, 0);
        arduboy.allPixelsOn(true);
        delay(50);
        arduboy.allPixelsOn(false);
      }
    }
  }
  for (int i = 0; i < MAX_OBJS; i++) {
    if (abs((obstacles[i].x) - (ship.x + 6)) <= 8 && abs((obstacles[i].y) - (ship.y + 7)) <= 8) {
      if (!mute) arduboy.tunes.playScore(point); // TODO: play "low boop" instead of point sound
      obstacles[i].state = 0;
      explodeAt(obstacles[i].x, obstacles[i].y);
      obstacles[i].y = -20;
      ship.life = max(ship.life - 1, 0);
      arduboy.allPixelsOn(true);
      delay(50);
      arduboy.allPixelsOn(false);
    }
  }
}

// ***********************************************
// GAME intro title screen loop
// ***********************************************

void titleScreen() {
  initStars();
  resetAll();
  waitBmp(39, 7, Logo, false);
  waitMsg(20, 25, "By @icodealot", false, 1);
  while (true) {
    gameMode = menuSelect("Start Game", "Settings", (char*)NULL); // add "Story Mode"
    if (gameMode == 2) {
      mute = (menuSelect("Audio On", "Audio Off", (char*)NULL) == 2);
    } else {
      break;
    }
  }
  loadLevel(levelIndex);
}


// ***********************************************
// OTHER UTILITIES
// ***********************************************

void message(int x, int y, String string) {
  arduboy.setCursor(x,y);
  arduboy.print(string);
}

void explodeAt(int x, int y) {
  boomIndex = (boomIndex + 1) % MAX_BOOMS;
  if (booms[boomIndex].state == 0) {
    booms[boomIndex].state = 1;
    booms[boomIndex].frame = 0;
    booms[boomIndex].x = x;
    booms[boomIndex].y = y;
  }
}

int menuSelect(String opt1, String opt2, String opt3) {
  lTime = millis();
  unsigned long sTime = lTime;
  int selected = 0;
  bool holding = true;
  while ( true ) {     
    if (arduboy.nextFrame()) {
      lTime = millis();
      arduboy.clearDisplay();
      drawStars();
      
      message(27,10,opt1);
      message(27,27,opt2);
      
      if (opt3) 
        message(27,44,opt3);

      arduboy.drawBitmap(11, 7 + (selected * 17), Ship3, 13, 13, WHITE);
      
      if (arduboy.getInput()) {
        if (lTime <= (sTime + 50))
          holding = true;
        else {
          holding = false;
          if (arduboy.pressed(UP_BUTTON)) {
            selected = max((selected - 1),0);
          } else if (arduboy.pressed(DOWN_BUTTON)) {
            selected = min((selected + 1),(opt3 ? 2 : 1));
          }
        }
        sTime = lTime;
      }

      if (!holding && (arduboy.pressed(A_BUTTON) || arduboy.pressed(B_BUTTON)))
        return selected + 1;
      
      arduboy.display();
    }
  }
}

void waitBmp(int x, int y, const uint8_t *bitmap, bool celebrate) {
  unsigned long sTime = lTime;
  bool entered = false;
  while ( true ) { 
        
    if (arduboy.nextFrame()) {
      lTime = millis();
      arduboy.clearDisplay();
      if (celebrate)
        explodeAt(random(10,WIDTH-10),random(10,HEIGHT-10));
      drawStars();
      drawExplosions();
      
      arduboy.drawBitmap(x, y, bitmap, 50, 50, WHITE);
      
      arduboy.display(); 
    }
    
    if (arduboy.getInput()) {
      if (lTime <= (sTime + 50))
        entered = false;
      else {
        break;
      }
      sTime = lTime;
    }
  }
}

void waitMsg(int x, int y, String msg, bool celebrate, int wait) {
  unsigned long sTime = lTime;
  while ( true ) { 
    if (lTime >= (sTime + (1000*wait)))
      break;
    
    if (arduboy.nextFrame()) {
      lTime = millis();
      arduboy.clearDisplay();
      if (celebrate)
        explodeAt(random(10,WIDTH-10),random(10,HEIGHT-10));
      drawStars();
      drawExplosions();
      message(x,y,msg);
      arduboy.display(); 
    }
  }
}

void waitBox(int x1, int y1, String msg1, int x2, int y2, String msg2, bool celebrate, int wait) {
  unsigned long sTime = lTime;
  while ( true ) { 
    if (arduboy.getInput() && lTime >= (sTime + (1000*wait)))
      break;
    
    if (arduboy.nextFrame()) {
      lTime = millis();
      arduboy.clearDisplay();
      if (celebrate)
        explodeAt(random(10,WIDTH-10),random(10,HEIGHT-10));
      drawStars();
      drawExplosions();
      arduboy.drawRect(32,15,WIDTH / 2, HEIGHT / 2, WHITE);
      message(x1,y1,msg1);
      message(x2,y2,msg2);
      arduboy.display(); 
    }
  }
}

void loadLevel(int level) {
  enemySpawnRate = max(700 - (100 * levelIndex), 100);
  enemyVelocity = min(3, (levelIndex + 1)/2);
  ship.life = min(ship.life + 1, 5);
  for (int i = 0; i < WIDTH; i++) {
    arduboy.clearDisplay();
    ship.x = -10+ i*2;//(i)%140;
    ship.y = (i < 20 ? 25 : 25 + (15 * cos(i)));
    ship.y = (i > 115 ? 25 + (10 * cos(i)) : ship.y);
    ship.y = (i > 125 ? 25 + (5 * cos(i)) : ship.y);
    ship.y = (i > 130 ? 25 : ship.y);
    delay(10);
    moveShip();
    drawStars();
    arduboy.drawRect(35,15,WIDTH / 2, HEIGHT / 2, WHITE);
    if (level <= 9)
      message(46,20,"Level " + String(level));
    else if (level <= 99)
      message(42,20,"Level " + String(level));
    else if (level <= 999)
      message(38,20,"Level " + String(level));
    
    message(48,30,"Loaded");
    arduboy.display();
    lTime = millis();
  }
  ship.x = 10;
}

