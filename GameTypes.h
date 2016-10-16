/*
 * Copyright (c) 2016 @icodealot
 */
 
enum modes: int {
  arcade = 1,
  story = 2
};

typedef struct {
  int x = 0;
  int y = 0;
  char frame = 0;
  char state = 0;
} SCNOBJ_ANIMATED;

typedef struct {
  int x = 0;
  int y = 0;
  char w;
  char state = 0;
  char velocity;
} SCNOBJ;

typedef struct {
  int x;
  int y;
  char w;
  int score; 
  int life;
  char state;
  char velocity;
  char mod;
} SHIP;

typedef struct {
  int x = 0;
  int y = 0;
  int yi = 0;
  int w = 13;
  int health = 100;
  char velocity = 2;
  char behavior = 1;
  int state = 1;
} ENEMY;

typedef struct {
  int x;
  int y;
  char velocity;
  char state;
  //char power;
  //char range;
} SHOT;

