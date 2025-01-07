/*
Version: 0.0.4
C Standard: C17
Author: Tilo von Eschwege
*/

#include <stdio.h>
#include <Math.h>
#include <stdint.h>
#include <stdlib.h>

#include "E:\res\SDL3\include\SDL3\SDL.h"
#include "constants.h"
//#include "force.h"


typedef struct Vec2D {
  float x;
  float y;
} vec2D;

typedef struct Ball
{
    vec2D pos;
    vec2D velo;
    vec2D accel;
    vec2D jerk;


    uint16_t width;
    uint16_t height;
    float density;
    float mass;
} ball;



////////////////////////////////////////////////////////////////
//----------------------global variables----------------------//
////////////////////////////////////////////////////////////////
int ball_num = 0;
int len_balls_arr = 100;
ball* balls;


//TODO REALLOC in spawn balls, if ball_num gets exceeded

vec2D mouse;

uint8_t game_is_running = FALSE;

float delta_time = 0;
int last_frame_time = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;


//used for collision
int initial_y = 0;

////////////////////////////////////////////////////////////////
//--------------------------Functions-------------------------//
////////////////////////////////////////////////////////////////
void init_ball(ball* b) {
  b->velo = (vec2D){0, 0};
  b->accel = (vec2D){0, 0};
  b->jerk = (vec2D){0, 0};

  b->width = 5;
  b->height = 5;

  b->density = 1;
  b->mass = b->density * ((float)(b->width * b->height));
}

void spawn_ball(float x, float y) {
  if (len_balls_arr <= ball_num + 1) {
    len_balls_arr *= 2;

    ball* new_balls = realloc(balls, sizeof(ball) * len_balls_arr);
    if (new_balls == NULL) {
      fprintf(stderr, "realloc of balls failed!\nball_num=%d, len_balls_arr=%d"
        ,ball_num, len_balls_arr);
      exit(-1);
    }
    balls = new_balls;
    //printf("Reallocated balls! New Size:%d\n",len_balls_arr);
  }

  ball* b = &balls[ball_num];

  b->pos = (vec2D){x, y};
  //printf("Spawning Ball at (%d, %d), with index: %d\n",x, y, ball_num);

  init_ball(b);

  ball_num++;
}

/******************************************
* Applies the x and y speed
* to the x and y coordiantes of the balls
* Handles collision of the balls
* and the window border.
******************************************/
void check_borders(ball* b) {
  //left or right
  if ((b->pos.x > (WINDOW_WIDTH - b->width)) || (b->pos.x < 0))
  {
      b->velo.x *= -1;
  }
  //bottom or top
  if ((b->pos.y > (WINDOW_HEIGHT - b->height)) || (b->pos.y < 0))
  {
      b->velo.y *= -1;
  }
}

void update_ball_movement(float dt)
{
    ball* b;
    for (int i = 0; i < ball_num; i++) //disregard mouse player
    {
        b = &balls[i];

        //check_borders(b);

        b->accel.x += b->jerk.x * dt;
        b->accel.y += b->jerk.y * dt;

        b->velo.x += b->accel.x * dt;
        b->velo.y += b->accel.y * dt;

        b->pos.x += b->velo.x * dt;
        b->pos.y += b->velo.y * dt;

    }
}


vec2D gravity(ball* b1, ball* b2, float d) {
  vec2D gravitational_force = {0,0};

  gravitational_force.x = G * (b1->mass * b2->mass) / (pow(d,2) + pow(SOFTENING_FACTOR,2)); // FG = G * m1 * m2 / r^2 + softening_factor^2
  gravitational_force.y = G * (b1->mass * b2->mass) / (pow(d,2) + pow(SOFTENING_FACTOR,2));

  return gravitational_force;

}

void apply_forces()
{
    vec2D total_gravitional_force;
    vec2D gravity_v;
    vec2D distance_v;
    float d;

    ball* b1;
    ball* b2;
    for (int i = 0; i < ball_num; i++)
    {
        b1 = &balls[i];
        total_gravitional_force.x = 0.0;
        total_gravitional_force.y = 0.0;

        for (int j = 1; j < ball_num; j++)
        {
            b2 = &balls[j];

            distance_v.x = b2->pos.x - b1->pos.x;
            distance_v.y = b2->pos.y - b1->pos.y;

            d = sqrt(pow(distance_v.x,2) + pow(distance_v.y,2)); //distance btw b1 & b2



            if (d > MIN_GRAVITY_ACTION_DISTANCE) {
              gravity_v = gravity(b1,b2,d);

              //the normalized vector is multiplied with the gravitational force
              total_gravitional_force.x +=  (distance_v.x / d) * gravity_v.x;
              total_gravitional_force.y +=  (distance_v.y / d) * gravity_v.y;
            }

        }

        b1->accel.x = total_gravitional_force.x / b1->mass; // F = m * a -> a = F / m
        b1->accel.y = total_gravitional_force.y / b1->mass;

    }
}



/**********************************
* Creates the window and renderer,
* While handling potential errors.
**********************************/
int initialize_window()
{
    int initErrC = SDL_Init(SDL_INIT_VIDEO);
    if (initErrC != 1)
    {
        fprintf(stderr, "An error occured, while initializing SDL.\nError Code: %d\nSDL Error:%s\n",initErrC,SDL_GetError());

        return FALSE;
    }

    window = SDL_CreateWindow(
        "GravitySim", //window title
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_FULLSCREEN
    );

    if (!window)
    {
        fprintf(stderr, "An error occured, while creating the window.\nSDL Error:%s\n",SDL_GetError());
        return FALSE;
    }


    // int numRenders = SDL_GetNumRenderDrivers();
    // printf("NumRenderDrivers:%d\n",
    // numRenders);
    //
    // puts("\nAvailable Renders:\n");
    // for(int i = 0; i < numRenders; i++) {
    //   printf("%s\n",SDL_GetRenderDriver(i));
    // }

    renderer = SDL_CreateRenderer(window,"gpu");//driver code, display number; -1 -> default

    if (!renderer)
    {
        fprintf(stderr, "An error occured, while creating the SDL-Renderer.\nSDL Error:%s\n",SDL_GetError());
        return FALSE;
    }

    return TRUE;
}


/*********************
* Handles user-input.
*********************/
void process_input()
{
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type)
    {
        case SDL_EVENT_QUIT: //window's x button is clicked
            game_is_running = FALSE;
            break;

        case SDL_EVENT_KEY_DOWN: //keypress

            switch(event.key.key)
            {
              case SDLK_ESCAPE:
                game_is_running = FALSE;
                break;

              case SDLK_O:
                ball_num = 0;
                break;
            }
            //printf("%d\n",event.key.key);

        case SDL_EVENT_MOUSE_MOTION:
            mouse.x = (uint16_t)event.motion.x;
            mouse.y = (uint16_t)event.motion.y;

            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            //printf("mouse=(%d, %d)\n",(int)mouse.x, (int)mouse.y);
            spawn_ball(mouse.x, mouse.y);
            break;

    }
}


/************************
* Gets called only once.
*************************/
void setup()
{
  balls = calloc(len_balls_arr, sizeof(ball));
}


/************************
* Applies physics
* To the game objects.
*************************/
void update()
{
    //delay, so that the capped framerate is reached (and not overshoot).
    int time_to_wait = FRAME_TARGET_TIME - (SDL_GetTicks() - last_frame_time);

    if (time_to_wait > 0 && time_to_wait <= FRAME_TARGET_TIME)
    {
        SDL_Delay(time_to_wait);
    }


    delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;

    last_frame_time = SDL_GetTicks();



    apply_forces();
    update_ball_movement(delta_time);



}


/*******************************
* Re-/Draws the window,
* With all of the game-objects.
*******************************/
void render()
{
    //background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); //R, G, B, Alpha
    SDL_RenderClear(renderer);


    for (int i = 0; i < ball_num; i++) {

        //printf("rendering ball at (%f,%f)\n",balls[i].pos.x,balls[i].pos.y);

        SDL_FRect ball_rect =
        {
            (int)balls[i].pos.x,
            (int)balls[i].pos.y,
            balls[i].width,
            balls[i].height
        };

        SDL_SetRenderDrawColor(renderer, i % 2 * 255, 50, (i+1) % 2 * 255, 255); //4 colors(ugly): i % 3 * 255, (i + 1) % 3 * 255, (i + 2) % 3 * 255, 255
        SDL_RenderFillRect(renderer, &ball_rect);
    }


    SDL_RenderPresent(renderer);

}






/***************************
* Quits the renderer,
* SDL and closes the window.
***************************/
void destroy_window()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}


/*********************************
* Initializes the window,
* Calls the setup function
* And starts the game-loop,
* With the process_input(),
* update() and render() function.
* When the game-loop terminates,
* The window gets destroyed.
*
* @param argc, @param args
*********************************/
void game() {
  printf("Game is running...\n");


  game_is_running = initialize_window();
  setup();
  while (game_is_running) //Game Loop
  {
      process_input();
      update();
      render();

  }

  destroy_window();
}

int main(int argc, char* args[])
{
    game();
    return 0;
}

/*
* The click of the mouse should spawn a ball at that position.
* That way the user doesn't need to input the ball_num
*/
