#include<math.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctime>

extern "C" {
	#include"./SDL2-2.0.10/include/SDL.h"
	#include"./SDL2-2.0.10/include/SDL_main.h"
}

// define needed constants
#define _USE_MATH_DEFINES
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define LEGEND_X_CORD 430
#define LEGEND_Y_CORD 420 //only the first line
#define LEGEND_HEIGHT 40
#define TEXT_HEIGHT 10 
#define INFO_TEXT_WIDTH SCREEN_WIDTH - 8
#define INFO_TEXT_HEIGHT 36
#define BACKGROUND_HEIGHT 3 * SCREEN_HEIGHT
#define ROAD_WIDTH 213
#define INITIAL_CAMERA_SPEED 1
#define CAR_WIDTH 30
#define CAR_HEIGHT 40
#define INITIAL_PLAYER_X_CORD SCREEN_WIDTH / 2
#define INITIAL_PLAYER_Y_CORD SCREEN_HEIGHT / 1.5
#define MAX_PLAYER_SPEED 5
#define MIN_PLAYER_SPEED 1
#define PLAYER_HP 4
#define SCORE_MULTIPLIER 15
#define NUM_OF_SEC_BEFORE_SCORE_INCREMENTS 0.5
#define NUM_OF_SEC_TO_WAIT_BEFORE_MOVING_OBJECT 0.01
#define INITIAL_ENEMY_SPEED 4
#define ENEMY_HP 2
#define INITIAL_NON_ENEMY_SPEED 4
#define NON_ENEMY_HP 2
#define POWERUP_HEIGHT 16
#define POWERUP_WIDTH 32
#define POWERUP_TIME 5
#define POWERUP_SPEED -2
#define BULLET_WIDTH 9
#define BULLET_HEIGHT 18
#define BOMB_WIDTH 16
#define BOMB_HEIGHT 16
#define BULLET_SPEED 4
#define BOMB_SPEED 2
#define ENEMY_SHOOT_RANGE 200
#define SHOOT_RANGE 200
#define NUM_OF_SEC_TO_WAIT_AFTER_DEATH 1
#define UNLIMITED_CARS_TIME 1
#define NUM_OF_SEC_BEFORE_ENEMY_ATTACKS 0.25
#define NUM_OF_POINTS_NEEDED_TO_GET_CARS 5000
#define ENEMY_KILLED_BONUS 100
#define HALT_SCORE_TIME 2

// enum for different types of game objects
enum class ObjectType {
  Player,
  NonEnemy,
  Enemy,
  Bullet,
  Powerup
};

// structure for storing game objects
struct Object {
  ObjectType type;
  int x;
  int y;
  int hp;
  double speed;
  SDL_Texture* texture;
};

// structure for storing the camera cordinates
struct Camera {
  int x;
  int y;
};

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};

// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
	};


// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
		};
	};

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

// starts a new game
void newGame(double* elapsedTime, int* score, Object& player, int* multiplier, bool* isPlayerDead, SDL_Renderer* renderer,
			 double* isPlayerDeadTime, bool* isUnlimitedCars, bool* isPowerup, int* scoreThreshold, int* damage) {
	// set variables to initial values
	*damage = (ENEMY_HP + NON_ENEMY_HP) / 4;
	*isPlayerDeadTime = 0;
	*elapsedTime = 0;
	*score = 0;
	*scoreThreshold = 0;
	player.hp = PLAYER_HP;
	player.speed = MIN_PLAYER_SPEED;
	player.x = INITIAL_PLAYER_X_CORD;
	player.y = INITIAL_PLAYER_Y_CORD;
	*multiplier = 0;
	*isPlayerDead = 0;
	*isUnlimitedCars = true;
	*isPowerup = false;
	SDL_Surface* player_image = SDL_LoadBMP("player.bmp");
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(player_image, SDL_TRUE, SDL_MapRGB(player_image->format, 255, 0, 255)); 
	player.texture = SDL_CreateTextureFromSurface(renderer, player_image);
}

// change image of passed object 
void changeImage( Object* object, SDL_Renderer* renderer, char* fileName, int R, int G, int B) {
	SDL_Surface* object_image = SDL_LoadBMP(fileName);
	SDL_SetColorKey(object_image, SDL_TRUE, SDL_MapRGB(object_image->format, R, G, B)); 
	object->texture = SDL_CreateTextureFromSurface(renderer, object_image);
}

// pause the game
void pause(bool* paused, double* elapsedTime) {
	if(*paused == false) {
		*paused = true;
	}
	else{
		*paused = false;
	}
}

// sets screen to black and prints game over and score at the centre
void gameOver(SDL_Renderer* renderer, int score, SDL_Surface* charset, int* quit, int* t1) {
	SDL_Surface* gameOverScreen;
	SDL_Texture* gameOverScreenTex;
	char text[256];
	bool game_over = 1;

	// initialize game over screen surface and texture
	gameOverScreen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
								0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	gameOverScreenTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
							SDL_TEXTUREACCESS_STREAMING,
							SCREEN_WIDTH, SCREEN_HEIGHT);

	// calculate the dimensions of the "Game Over" text
	const char* gameOverText = "Game Over";
	int textWidth = 8 * strlen(gameOverText);
	int textHeight = 8;

	// calculate the position of the "Game Over" text
	int x = (SCREEN_WIDTH - textWidth) / 2;
	int y = (SCREEN_HEIGHT - textHeight) / 2;

  	// draw the "Game Over" text
	sprintf(text, "Game Over");	
	DrawString(gameOverScreen, x, y, text, charset);

  	// draw the score text
  	sprintf(text, "Score: %d", score);
	DrawString(gameOverScreen, x , y + textHeight + 2, text, charset);

	// draw legend
	sprintf(text, "esc: quit the program");
	DrawString(gameOverScreen, LEGEND_X_CORD, LEGEND_Y_CORD + TEXT_HEIGHT, text, charset);
	sprintf(text, "n: start a new game");		
	DrawString(gameOverScreen, LEGEND_X_CORD, LEGEND_Y_CORD + 2 * TEXT_HEIGHT, text, charset);

	// update texture
	SDL_UpdateTexture(gameOverScreenTex, NULL, gameOverScreen->pixels, gameOverScreen->pitch);

  	// set the draw color to white
  	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

	// clear the screen
	SDL_RenderClear(renderer);

	// render the screen
	SDL_RenderCopy(renderer, gameOverScreenTex, NULL, NULL);
	SDL_RenderPresent(renderer);
  	
	while(game_over) {
		*t1 = SDL_GetTicks();
		// get player input
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				*quit = 1;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					*quit = 1;
				case 'n':
					game_over = 0;
				}
			}
		}
	}
	SDL_FreeSurface(gameOverScreen);
	SDL_DestroyTexture(gameOverScreenTex);
}

// makes shoot
void make_shoot(int* bullet_x, int* bullet_y, Object player, bool* shoot, double* shootTime) {
	*bullet_x = player.x + (CAR_WIDTH / 3);
	*bullet_y = player.y - (CAR_HEIGHT / 2);
	*shootTime = 0;
	*shoot = true;
}

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char **argv) {
	// initialize needed variables
	SDL_Event event;
	SDL_Surface *screen;
	SDL_Surface *charset;
	SDL_Surface *bg;
	SDL_Texture *scrtex;
	SDL_Texture *bgtex;
	SDL_Window *window;
	SDL_Renderer *renderer;
	Camera camera;
	int t1, t2, rc;
	const int rightEdgeOfTheRoad = 2 * ROAD_WIDTH - CAR_WIDTH / 2;
	const int leftEdgeOfTheRoad = ROAD_WIDTH - CAR_WIDTH / 2;
	int max_y = -SCREEN_HEIGHT;
	int min_y = - BACKGROUND_HEIGHT;
	int num_of_lives_left = 99999999;
	int damage = (ENEMY_HP + NON_ENEMY_HP) / 4;
	int frames = 0;
	int quit = 0;
	int multiplier = 0;
	int score = 0;
	int scoreThreshold = 0;
	double delta; 
	double unlimitedCarsTime = 0;
	double haltScoreTime = 0;
	double fpsTimer = 0;
	double fps = 0;
	double elapsedTime = 0;
	double time = 0;
	double time2 = 0;
	double isPlayerDeadTime = 0;
	double shootTime = 999999999999;
	double pausedTime = 0;
	double gameOverTime = 0;
	double powerupTime = 0;
	double nearEnemyTime = 0;
	bool shoot = 0;
	bool game_over = 0;
	bool isUnlimitedCars = 1;
	bool isPowerup = 0;
	bool isOnTheEdge = 0;
	bool firstTime = 1;
	bool isPlayerDead = 0;
	bool paused = 0;
	bool haltScore = 0;
	camera.x = 0;
	camera.y = BACKGROUND_HEIGHT;
	char text[128];

	if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// fullscreen mode
	// rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
	//                                  &window, &renderer);

	// screen mode
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if(rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	}
	
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(window, "Spy hunter");

	// load the icon image
	SDL_Surface *icon = SDL_LoadBMP("icon.bmp");
	if(icon == NULL) {
		printf("SDL_LoadBMP(icon.bmp) error: %s\n", SDL_GetError());
	}

  	SDL_SetWindowIcon(window, icon);

	// initialize screen surface and texture
	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
	                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                           SDL_TEXTUREACCESS_STREAMING,
	                           SCREEN_WIDTH, SCREEN_HEIGHT);

	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	// set up player object	(red car)
	Object player = { ObjectType::Player, INITIAL_PLAYER_X_CORD, INITIAL_PLAYER_Y_CORD };
	SDL_Surface* player_image = SDL_LoadBMP("player.bmp");
	player.speed = MIN_PLAYER_SPEED;
	player.hp = PLAYER_HP;
	if(player_image == NULL) {
		printf("SDL_LoadBMP(player.bmp) error: %s\n", SDL_GetError());
	}
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(player_image, SDL_TRUE, SDL_MapRGB(player_image->format, 255, 0, 255)); 
	player.texture = SDL_CreateTextureFromSurface(renderer, player_image);

	// set up enemies objects (blue cars)
	const int NUM_ENEMIES = 2;
	Object enemies[NUM_ENEMIES];
	SDL_Surface* enemy_image = SDL_LoadBMP("enemy.bmp");
	if(enemy_image == NULL) {
		printf("SDL_LoadBMP(enemy.bmp) error: %s\n", SDL_GetError());
	}
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(enemy_image, SDL_TRUE, SDL_MapRGB(enemy_image->format, 255, 0, 255)); 
	for (int i = 0; i < NUM_ENEMIES; i++) {
		enemies[i] = { ObjectType::Enemy, rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad, rand() % BACKGROUND_HEIGHT};
		enemies[i].texture = SDL_CreateTextureFromSurface(renderer, enemy_image);
		enemies[i].hp = ENEMY_HP;
	}

	// set up non enemies objects (green cars)
	const int NUM_NON_ENEMIES = 1;
	Object non_enemies[NUM_NON_ENEMIES];
	SDL_Surface* non_enemy_image = SDL_LoadBMP("non_enemy.bmp");
	if(non_enemy_image == NULL) {
		printf("SDL_LoadBMP(non_enemy.bmp) error: %s\n", SDL_GetError());
	}
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(non_enemy_image, SDL_TRUE, SDL_MapRGB(non_enemy_image->format, 255, 0, 255)); 
	for (int i = 0; i < NUM_NON_ENEMIES; i++) {
		non_enemies[i] = { ObjectType::Enemy, rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad, rand() % BACKGROUND_HEIGHT};
		non_enemies[i].texture = SDL_CreateTextureFromSurface(renderer, non_enemy_image);
		non_enemies[i].hp = NON_ENEMY_HP;
	}

	// set up bullet object
	Object bullet = { ObjectType::Bullet, player.x + (CAR_WIDTH / 3), player.y };
	SDL_Surface* bullet_image = SDL_LoadBMP("bullet.bmp");
	if(bullet_image == NULL) {
		printf("SDL_LoadBMP(bullet.bmp) error: %s\n", SDL_GetError());
	}
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(bullet_image, SDL_TRUE, SDL_MapRGB(bullet_image->format, 255, 0, 255)); 
	bullet.texture = SDL_CreateTextureFromSurface(renderer, bullet_image);

	// set up powerups objects
	const int NUM_POWERUPS = 1;
	Object powerups[NUM_POWERUPS];
	SDL_Surface* powerup_image = SDL_LoadBMP("powerup.bmp");
	if(powerup_image == NULL) {
		printf("SDL_LoadBMP(powerup.bmp) error: %s\n", SDL_GetError());
	}
	// magenta (255, 0, 255) will be treated as transparent
	SDL_SetColorKey(powerup_image, SDL_TRUE, SDL_MapRGB(powerup_image->format, 255, 0, 255)); 
	for (int i = 0; i < NUM_POWERUPS; i++) {
		powerups[i] = { ObjectType::Powerup, rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad, rand() % BACKGROUND_HEIGHT};
		powerups[i].texture = SDL_CreateTextureFromSurface(renderer, powerup_image);
	}

	// turnoff visability of cursor
	SDL_ShowCursor(SDL_DISABLE);

	// load background picture
	bg = SDL_LoadBMP("bg.bmp");
	if(bg == NULL) {
		printf("SDL_LoadBMP(bg.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}

	// create background texture
	bgtex = SDL_CreateTextureFromSurface(renderer, bg);

	// load picture cs8x8.bmp
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	}

	SDL_SetColorKey(charset, true, 0x000000);

	// initialize powerup speed
	for(int i = 0; i < NUM_POWERUPS; i++) {
		powerups[i].speed = POWERUP_SPEED;
	}

	// initilaize bullet speed
	bullet.speed = BULLET_SPEED;

	t1 = SDL_GetTicks();

	while(!quit) {

		while(paused) {

			t1 = SDL_GetTicks();
			// get player input
			SDL_Event event;
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_QUIT:
					quit = 1;
					pause(&paused, &elapsedTime);
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						quit = 1;
					case 'n':
						newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime,
								&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);
					case 'p':
						pause(&paused, &elapsedTime);
					}
				}
			}
		}

		t2 = SDL_GetTicks();

		// here t2-t1 is the time in milliseconds since
		// the last screen was drawn
		// delta is the same time in seconds
		delta = (t2 - t1) * 0.001;

		if(!haltScore) {
			time += delta;
		}

		time2 += delta;

		shootTime += delta; 

		elapsedTime += delta;

		haltScoreTime += delta;

		powerupTime += delta;

		unlimitedCarsTime += delta;
		
		// increment score 
		if(time >= NUM_OF_SEC_BEFORE_SCORE_INCREMENTS) {
			time -= NUM_OF_SEC_BEFORE_SCORE_INCREMENTS;
			score += multiplier;
			scoreThreshold += multiplier;
		}

		// every x num of points get some cars
		if((scoreThreshold % NUM_OF_POINTS_NEEDED_TO_GET_CARS) == 0) {
			unlimitedCarsTime = 0;
			isUnlimitedCars = 1;
		}

		if(scoreThreshold > NUM_OF_POINTS_NEEDED_TO_GET_CARS) {
			scoreThreshold = 0;
		}

		// if time passed turn off unlimited cars
		if(unlimitedCarsTime >= UNLIMITED_CARS_TIME) {
			isUnlimitedCars = false;
		}

		// if time passed turn of halt of score
		if(haltScoreTime >= HALT_SCORE_TIME) {
			haltScore = false;
		}

		// if powerup is on damage will be 2 times bigger
		if(isPowerup) {
			damage = (ENEMY_HP + NON_ENEMY_HP) / 2;
		}
		else{
			damage = (ENEMY_HP + NON_ENEMY_HP) / 4;
		}

		// if time passed turn off powerup and change bullet image
		if(powerupTime >= POWERUP_TIME) {
			isPowerup = false;
			changeImage(&bullet, renderer, "bullet.bmp", 255, 0, 255);
		}

		t1 = t2;

		// change fps
		fpsTimer += delta;
		if(fpsTimer > 0.5) {
			fps = frames * 2;
			frames = 0;
			fpsTimer -= 0.5;
		}
		
		if(player.speed <= MIN_PLAYER_SPEED) {
			player.speed = MIN_PLAYER_SPEED;
			// enemies are faster than player and move up 
			for(int i = 0; i < NUM_ENEMIES; i++) {
				enemies[i].speed = INITIAL_ENEMY_SPEED;
				non_enemies[i].speed = INITIAL_NON_ENEMY_SPEED;
			}
		}

		if((player.speed > MIN_PLAYER_SPEED) && (player.speed <= MIN_PLAYER_SPEED + 1)) {
			// enemies are faster than player and move up but slower than initialy
			for(int i = 0; i < NUM_ENEMIES; i++) {
				enemies[i].speed = INITIAL_ENEMY_SPEED / 2;
				non_enemies[i].speed = INITIAL_NON_ENEMY_SPEED / 2;
			}
		}

		if((player.speed > MIN_PLAYER_SPEED + 1) && (player.speed <= MIN_PLAYER_SPEED + 2)) {
			// enemies are slower than player and move down with bg
			for(int i = 0; i < NUM_ENEMIES; i++) {
				enemies[i].speed = -(INITIAL_ENEMY_SPEED / 4);
				non_enemies[i].speed = -(INITIAL_NON_ENEMY_SPEED / 4);
			}
		}

		if((player.speed > MIN_PLAYER_SPEED + 2) && (player.speed <= MIN_PLAYER_SPEED + 3)) {
			// enemies are slower than player move down with bg
			for(int i = 0; i < NUM_ENEMIES; i++) {
				enemies[i].speed = -(INITIAL_ENEMY_SPEED / 2) ;
				non_enemies[i].speed = -(INITIAL_NON_ENEMY_SPEED / 2) ;
			}
		}

		if((player.speed > MIN_PLAYER_SPEED + 3) && (player.speed <= MAX_PLAYER_SPEED)) {
			// enemies are slower than player move down with bg
			for(int i = 0; i < NUM_ENEMIES; i++) {
				enemies[i].speed = -INITIAL_ENEMY_SPEED;
				non_enemies[i].speed = -INITIAL_NON_ENEMY_SPEED;
			}
		}

		if(player.speed > MAX_PLAYER_SPEED) {
			player.speed = MAX_PLAYER_SPEED;
		}

		// if is on the edge move camera slowly
		if(isOnTheEdge) {
			if(player.speed > MIN_PLAYER_SPEED) {
				camera.y -= INITIAL_CAMERA_SPEED;
			}
		}

		if(!isPlayerDead) {
			// move camera
			if(!isOnTheEdge) {
				if((player.speed > MIN_PLAYER_SPEED) && (player.speed <= MIN_PLAYER_SPEED + 1)) {
					camera.y -= INITIAL_CAMERA_SPEED;
				}

				if((player.speed > MIN_PLAYER_SPEED + 1) && (player.speed <= MIN_PLAYER_SPEED + 2)) {
					camera.y -= 2 * INITIAL_CAMERA_SPEED;
				}

				if((player.speed > MIN_PLAYER_SPEED + 2) && (player.speed <= MIN_PLAYER_SPEED + 3)) {
					camera.y -= 3 * INITIAL_CAMERA_SPEED;
				}

				if((player.speed > MIN_PLAYER_SPEED + 3) && (player.speed <= MAX_PLAYER_SPEED)) {
					camera.y -= 4 * INITIAL_CAMERA_SPEED;
				}
			}
			
			if(time2 >= NUM_OF_SEC_TO_WAIT_BEFORE_MOVING_OBJECT) {
				time2 -= NUM_OF_SEC_TO_WAIT_BEFORE_MOVING_OBJECT;
				// move each object 
				for(int i = 0; i < NUM_ENEMIES; i++) {
					enemies[i].y -= enemies[i].speed;
				}

				for(int i = 0; i < NUM_NON_ENEMIES; i++) {
					non_enemies[i].y -= non_enemies[i].speed;
				}

				for(int i = 0; i < NUM_POWERUPS; i++) {
					powerups[i].y -= powerups[i].speed;
				}

				if(shoot) {
					bullet.y -= bullet.speed;
				}
			}
		}
		else{
			// (if player is dead)
			isPlayerDeadTime += delta;
			player.speed = MIN_PLAYER_SPEED;
			if(isUnlimitedCars || (isPlayerDeadTime >= NUM_OF_SEC_TO_WAIT_AFTER_DEATH)) {
				isPlayerDeadTime = 0;
				isPlayerDead = 0;
				firstTime = 1;
				player.x = INITIAL_PLAYER_X_CORD;
				player.y = INITIAL_PLAYER_Y_CORD;
				changeImage(&player, renderer, "player.bmp", 255, 0, 255);
			}
		}

		// if the camera y coordinate goes past the bottom of the background image, reset it to the top
		if (camera.y < 0) {
		camera.y = BACKGROUND_HEIGHT;
		}

		// set screen to black
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

		// create source and destination rectangles and blitsurface
		SDL_Rect src, dest;
		src.x = 0;
		src.y = camera.y;
		src.w = bg->w;
		src.h = bg->h - camera.y;
		dest.x = 0;
		dest.y = 0;
		dest.w = bg->w;
		dest.h = bg->h - camera.y;
		SDL_BlitSurface(bg, &src, screen, &dest);

		// if the camera has moved past the bottom of the background image,
		// draw another copy of the background image above it
		if (camera.y > 0) {
			src.x = 0;
			src.y = 0;
			src.w = bg->w;
			src.h = camera.y;
			dest.x = 0;
			dest.y = bg->h - camera.y;
			dest.w = bg->w;
			dest.h = camera.y;
			SDL_BlitSurface(bg, &src, screen, &dest);
		}

		for(int i = 0; i < NUM_ENEMIES; i++) {
			// collisions with enemies

			// if player hits enemy car from below
			if((abs(player.y - (CAR_HEIGHT / 2) - (enemies[i].y + (CAR_HEIGHT / 2))) < CAR_HEIGHT / 20) 
			&& (abs(player.x - enemies[i].x) <= CAR_WIDTH )) {
				changeImage(&player, renderer, "explosion.bmp", 255, 255, 255);
				multiplier = 0;
				isPlayerDead = 1;
				enemies[i].y -= CAR_HEIGHT / 4;
				if(firstTime && !isUnlimitedCars) {
					player.hp--;	
					firstTime = 0;
				}
				if(player.hp == 0) {
					game_over = 1;
					// whole screen is set to black and at the center is printed game over and score
					gameOver(renderer, score, charset, &quit, &t1);
					newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime, 
					&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);				

				}
			}	
			// enemy car hits player from below
			if((abs(player.y + (CAR_HEIGHT / 2) - (enemies[i].y - (CAR_HEIGHT / 2))) <= CAR_HEIGHT / 20) 
			&& (abs(player.x - enemies[i].x) <= CAR_WIDTH )) {
				// recycle enemy object
				enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				enemies[i].y = -SCREEN_HEIGHT;
			}

			// forcing player out of the road

			// if player is near enemy car from left side for set time
			if((abs(player.y - enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - enemies[i].x >= CAR_WIDTH) && 
			  (player.x - enemies[i].x) < CAR_WIDTH * 2)) {
				nearEnemyTime += delta;
				// move player and enemy right
				if(nearEnemyTime >= NUM_OF_SEC_BEFORE_ENEMY_ATTACKS) {
					nearEnemyTime -= NUM_OF_SEC_BEFORE_ENEMY_ATTACKS;
					player.x += CAR_WIDTH / 2;
					enemies[i].x += CAR_WIDTH / 2;
				}
			}

			// if player is near enemy car from left side for set time
			if((abs(player.y - enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - enemies[i].x <= -CAR_WIDTH) && 
			  (player.x - enemies[i].x) > 2 * (-CAR_WIDTH))) {
				nearEnemyTime += delta;
				// move player and enemy left
				if(nearEnemyTime >= NUM_OF_SEC_BEFORE_ENEMY_ATTACKS) {
					nearEnemyTime -= NUM_OF_SEC_BEFORE_ENEMY_ATTACKS;
					player.x -= CAR_WIDTH / 2;
					enemies[i].x -= CAR_WIDTH / 2;
			  	}
			}

			// forcing enemies out of the road

			// if player moves left and hits enemy car from side
			if((abs(player.y - enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - enemies[i].x < CAR_WIDTH) && 
			  (player.x - enemies[i].x) > 0)) {
				// move enemy car left
				enemies[i].x -= CAR_WIDTH / 2;
			}

			// if player moves right and hits enemy car from side
			if((abs(player.y - enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - enemies[i].x > -CAR_WIDTH) && 
			  (player.x - enemies[i].x) < 0)) {
				// move enemy car right
				enemies[i].x += CAR_WIDTH / 2;
			}
	
		}
		for(int i = 0; i < NUM_NON_ENEMIES; i++) {
			// collisions with non enemy cars 

			// if player hits non enemy car from below
			if((abs(player.y - (CAR_HEIGHT / 2) - (non_enemies[i].y + (CAR_HEIGHT / 2))) < CAR_HEIGHT / 20) 
			&& (abs(player.x - non_enemies[i].x) <= CAR_WIDTH )) {
				changeImage(&player, renderer, "explosion.bmp", 255, 255, 255);
				multiplier = 0;
				isPlayerDead = 1;
				non_enemies[i].y -= CAR_HEIGHT / 4;
				if(firstTime && !isUnlimitedCars) {
					player.hp--;	
					firstTime = 0;
				}
				if(player.hp == 0) {
					game_over = 1;
					// whole screen is set to black and at the center is printed game over and score
					gameOver(renderer, score, charset, &quit, &t1);
					newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime, 
					&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);				

				}
			}

			// non enemy car hits player from below
			if((abs(player.y + (CAR_HEIGHT / 2) - (non_enemies[i].y - (CAR_HEIGHT / 2))) <= CAR_HEIGHT / 20) 
			&& (abs(player.x - non_enemies[i].x) <= CAR_WIDTH )) {
				// recycle non enemy object
				non_enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				non_enemies[i].y = -SCREEN_HEIGHT;
			}

			// forcing non enemy cars out of the road
			
			// if player moves left and hits enemy car from side
			if((abs(player.y - non_enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - non_enemies[i].x < CAR_WIDTH) && 
			  (player.x - non_enemies[i].x) > 0)) {
				// move enemy car left
				non_enemies[i].x -= CAR_WIDTH / 2;
			}

			// if player moves right and hits enemy car from side
			if((abs(player.y - non_enemies[i].y) < CAR_HEIGHT / 2) && ((player.x - non_enemies[i].x > -CAR_WIDTH) && 
			  (player.x - non_enemies[i].x) < 0)) {
				// move enemy car right
				non_enemies[i].x += CAR_WIDTH / 2;
			}
		}

		for(int i = 0; i < NUM_POWERUPS; i++) {
			// if player hits powerup
			if((abs(player.y - powerups[i].y) <= POWERUP_HEIGHT) && (abs(player.x - powerups[i].x) <= POWERUP_WIDTH)) {
				changeImage(&bullet, renderer, "bomb.bmp", 255, 0, 255);
				isPowerup = 1;
				powerupTime = 0;
				// recycle powerup object
				powerups[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				powerups[i].y = -2 * BACKGROUND_HEIGHT;
			}
		}

		// if player is out of road
		if(player.x < leftEdgeOfTheRoad || player.x > rightEdgeOfTheRoad) {
			changeImage(&player, renderer, "explosion.bmp", 255, 255, 255);
			multiplier = 0;
			isPlayerDead = 1;

			if(firstTime && !isUnlimitedCars) {
				player.hp--;	
				firstTime = 0;
			}

			if(player.hp == 0) {
				game_over = 1;
				// whole screen is set to black and at the center is printed game over and score
				gameOver(renderer, score, charset, &quit, &t1);
				newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime,
						&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);					
			}

		}

		// if player is on the edge slow down and halt score 
		if((abs(player.x - leftEdgeOfTheRoad) < CAR_WIDTH / 2) || abs(player.x - rightEdgeOfTheRoad) < CAR_WIDTH / 2) {
			isOnTheEdge = 1;
			haltScore = 1;
		}
		else{
			isOnTheEdge = 0;
			haltScore = 0;
		}

		// if enemies are out of road
		for(int i = 0; i < NUM_ENEMIES; i++) {
			if(enemies[i].x < leftEdgeOfTheRoad || enemies[i].x >rightEdgeOfTheRoad) {
				// increase the score
				if(!haltScore) {
					score += ENEMY_KILLED_BONUS;
					scoreThreshold += ENEMY_KILLED_BONUS;
				}
				// recycle enemy object
				enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				enemies[i].y = -SCREEN_HEIGHT;
			}
		}

		// if non enemy cars are out of road
		for(int i = 0; i < NUM_NON_ENEMIES; i++) {
			if(non_enemies[i].x < leftEdgeOfTheRoad || non_enemies[i].x >rightEdgeOfTheRoad) {
				// halt the score
				haltScoreTime = 0;
				haltScore = 1;
				// recycle non enemy object
				non_enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				non_enemies[i].y = -SCREEN_HEIGHT;
			}
		}
		
		// if objects are far below move them above
		for(int i = 0; i < NUM_ENEMIES; i++) {
			if(enemies[i].y > (3 / 2) * BACKGROUND_HEIGHT) {
				enemies[i].y = rand() % (max_y - min_y + 1) + min_y;
				enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
			}
		}
		for(int i = 0; i < NUM_NON_ENEMIES; i++) {
			if(non_enemies[i].y > (3 / 2) * BACKGROUND_HEIGHT) {
				non_enemies[i].y = rand() % (max_y - min_y + 1) + min_y;
				non_enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
			}
		}
		for(int i = 0; i < NUM_POWERUPS; i++) {
			if(powerups[i].y > (3 / 2) * BACKGROUND_HEIGHT) {
				powerups[i].y = rand() % (max_y - min_y + 1) + min_y;
				powerups[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
			}
		}

		for(int i = 0; i < NUM_ENEMIES; i++) {
			// if bullet hits enemy
			if((abs(enemies[i].y - bullet.y) <= CAR_HEIGHT) && 
			(abs((enemies[i].x + (CAR_WIDTH / 2)) - (bullet.x + (BULLET_WIDTH / 2))) <= (CAR_WIDTH / 2))) {
				// move bullet out of screen 
				bullet.x = 2 * SCREEN_WIDTH;
				// decrement enemy hp
				enemies[i].hp -= damage;
				// increase the score
				if(!haltScore) {
					score += ENEMY_KILLED_BONUS;
					scoreThreshold += ENEMY_KILLED_BONUS;
				}
			}
		}

		for(int i = 0; i < NUM_NON_ENEMIES; i++) {
			// if bullet hits non enemy
			if((abs(non_enemies[i].y - bullet.y) <= CAR_HEIGHT) && 
			(abs((non_enemies[i].x + (CAR_WIDTH / 2)) - (bullet.x + (BULLET_WIDTH / 2))) <= (CAR_WIDTH / 2))) {
				// move bullet out of screen 
				bullet.x = 2 * SCREEN_WIDTH;
				// decrement non enemy hp
				non_enemies[i].hp -= damage;
				// halt the score
				haltScoreTime = 0;
				haltScore = 1;
			}
		}

		// if enemies have no hp move them above
		for(int i = 0; i < NUM_ENEMIES; i++) {
			if(enemies[i].hp <= 0) {
				enemies[i].hp = ENEMY_HP;
				// recycle enemy object
				enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				enemies[i].y = -SCREEN_HEIGHT;
				enemies[i].hp = ENEMY_HP;
			}
		}

		// if non enemies have no hp move them above
		for(int i = 0; i < NUM_NON_ENEMIES; i++) {
			if(non_enemies[i].hp <= 0) {
				non_enemies[i].hp = NON_ENEMY_HP;
				// recycle non enemy object
				non_enemies[i].x = rand() % (rightEdgeOfTheRoad - leftEdgeOfTheRoad + 1) + leftEdgeOfTheRoad;
				non_enemies[i].y = -SCREEN_HEIGHT;
				non_enemies[i].hp = NON_ENEMY_HP;
			}
		}

		// if distance between player and bullet is grater or equal shoot range then move bullet out of screen
		if((player.y - bullet.y) >= SHOOT_RANGE) {
			bullet.x = 2 * SCREEN_WIDTH;
		}

		// get player input
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				quit = 1;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = 1;
					break;
				case SDLK_LEFT:
					if(!isPlayerDead) {
						player.x -= CAR_WIDTH / 2;
					}
					break;
				case SDLK_RIGHT:
					if(!isPlayerDead) {
						player.x += CAR_WIDTH / 2;
					}
					break;
				case SDLK_UP:
					if(!isPlayerDead) {
						player.speed += 1;
						if(player.speed <= MAX_PLAYER_SPEED) {
							multiplier += SCORE_MULTIPLIER;
						}
					}
					break;
				case SDLK_DOWN:
					if(!isPlayerDead) {
						player.speed -= 1;
						if(player.speed >= MIN_PLAYER_SPEED) {
							multiplier -= SCORE_MULTIPLIER;
						}
					}
					break;
				case 'n':
					newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime,
							&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);
					break;
				case 'p':
					pause(&paused, &elapsedTime);
					break;
				case 'f':
					game_over = 1;
					// whole screen is set to black and at the center is printed game over and score
					gameOver(renderer, score, charset, &quit, &t1);
					newGame(&elapsedTime, &score, player, &multiplier, &isPlayerDead, renderer, &isPlayerDeadTime, 
					&isUnlimitedCars, &isPowerup, &scoreThreshold, &damage);				
					break;
				case SDLK_SPACE:
					make_shoot(&(bullet.x), &(bullet.y), player, &shoot, &shootTime);
					break;
				}
			break;
			}
		}

		// draw info text
		DrawRectangle(screen, 4, 4, INFO_TEXT_WIDTH, INFO_TEXT_HEIGHT, czerwony, niebieski);
		sprintf(text, "Wiktor Nazaruk s190454   score = %d   elapsed time = %.1lf s   %.0lf fps", score, elapsedTime, fps);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, TEXT_HEIGHT, text, charset);
		if(isUnlimitedCars) {
			sprintf(text, "player hp: infinity   player speed: %.0lf", player.speed);
		}
		else{
			sprintf(text, "player hp: %d   player speed: %.0lf", player.hp, player.speed);
		}
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 2 * TEXT_HEIGHT + 4, text, charset);


		// draw legend
		sprintf(text, "arrows: moving the player ");
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD, text, charset);
		sprintf(text, "esc: quit the program");
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD + TEXT_HEIGHT, text, charset);
		sprintf(text, "n: start a new game");		
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD + 2 * TEXT_HEIGHT, text, charset);
		sprintf(text, "p: pause/continue");		
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD + 3 * TEXT_HEIGHT, text, charset);
		sprintf(text, "space: shoot");		
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD + 4 * TEXT_HEIGHT, text, charset);
		sprintf(text, "f: finish");		
		DrawString(screen, LEGEND_X_CORD, LEGEND_Y_CORD + 5 * TEXT_HEIGHT, text, charset);

		// update texture
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);

		// clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		// render the background 
		SDL_RenderCopy(renderer, bgtex, &src, &dest);

		// render the screen
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);

		// draw each game object
		SDL_Rect car;
		car.w = CAR_WIDTH;
		car.h = CAR_HEIGHT;
		car.x = player.x;
		car.y = player.y;

		SDL_Rect bullet_rec;
		bullet_rec.x = bullet.x;
		bullet_rec.y = bullet.y;

		SDL_Rect powerup;
		powerup.w = POWERUP_WIDTH;
		powerup.h = POWERUP_HEIGHT;

		if(!isPowerup) {
			bullet_rec.w = BULLET_WIDTH;
			bullet_rec.h = BULLET_HEIGHT;
		}
		else{
			bullet_rec.w = BOMB_WIDTH;
			bullet_rec.h = BOMB_HEIGHT;			
		}

		// bullet should be rendered only if player shooted
		if(shoot) {
			SDL_RenderCopy(renderer, bullet.texture, nullptr, &bullet_rec);
		}

		// render player
		SDL_RenderCopy(renderer, player.texture, nullptr, &car);

		for (int i = 0; i < NUM_ENEMIES; i++) {
			car.x = enemies[i].x;
			car.y = enemies[i].y;
			// render enemies
			SDL_RenderCopy(renderer, enemies[i].texture, nullptr, &car);
		}

		for (int i = 0; i < NUM_NON_ENEMIES; i++) {
			car.x = non_enemies[i].x;
			car.y = non_enemies[i].y;
			// render non enemies
			SDL_RenderCopy(renderer, non_enemies[i].texture, nullptr, &car);
		}

		for (int i = 0; i < NUM_POWERUPS; i++) {
			powerup.x = powerups[i].x;
			powerup.y = powerups[i].y;
			// render powerups
			SDL_RenderCopy(renderer, powerups[i].texture, nullptr, &powerup);
		}

		// update the screen
		SDL_RenderPresent(renderer);

		// increment frames
		frames++;
	}

	// freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_FreeSurface(bg);
	SDL_FreeSurface(bullet_image);
	SDL_FreeSurface(player_image);
	SDL_FreeSurface(enemy_image);
	SDL_FreeSurface(non_enemy_image);
	SDL_FreeSurface(powerup_image);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyTexture(bgtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(icon);

	SDL_Quit();
	return 0;
}
