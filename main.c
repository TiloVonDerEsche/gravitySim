/*
Version: 0.0.2
C Standard: C17
Author: Tilo von Eschwege
*/

#include <stdio.h>
#include "E:\res\SDL3\include\SDL3\SDL.h"
#include "constants.h"
#include <Math.h>
#include <stdint.h>

//struct Player
//{
//    float x;
//    float y;
//    int width;
//    int height;
//    float speed;
//} player;

struct Ball
{
    uint16_t x;
    uint16_t y;
    float speed_x;
    float speed_y;
    float accel_x;
    float accel_y;

    uint16_t width;
    uint16_t height;
    float density;
    float mass;
    //int dir_x; //boolean; 1 -> move to the right, 0 -> move to the left
    //int dir_y; //boolean; 1 -> move to the bottom, 0 -> move to the top
} ball;

struct Ball balls[BALLS];

struct Ball* player = &balls[0];

////////////////////////////////////////////////////////////////
//----------------------global variables----------------------//
////////////////////////////////////////////////////////////////
uint16_t mouseX = 0;
uint16_t mouseY = 0;

uint8_t game_is_running = FALSE;
int last_frame_time = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;


float ball_x0 = 1.0, ball_y0 = 1.0, ball_m0 = 1.0;
float ball_x1 = 1.0, ball_y1 = 0.0, ball_m1 = 1.0;
float vx = 1.0, vy = 1.0, length_v = 1.0;

//used for collision
int initial_y = 0;

float total_gravitional_force_x = 0.0, total_gravitional_force_y = 0.0;

//gravity turns off below this distance
float min_gravity_action_distance = 20;

//should prevent the random slingshot effect of two very close balls
//F = G * m1 * m2 / (r� * softening_factor�)
float softening_factor = 50.0;

float gravity_vectors[BALLS * 2]; //malloc(sizeof(float) * BALLS * 2); //TODO why is this a static context and why doesn't malloc work here

//boolean array that holds all the positions of objects and world borders, the index represents the pixel, where the array saves the pixels row wise.
// x = i % WINDOW_WIDTH, y = i / WINDOW_WIDTH
int traversability_map[WINDOW_WIDTH * WINDOW_HEIGHT];


////////////////////////////////////////////////////////////////
//--------------------------Functions-------------------------//
////////////////////////////////////////////////////////////////
void initialize_balls()
{
    for (int i = 0; i < BALLS; i++)
    {
        balls[i].speed_x = 0;
        balls[i].speed_y = 0;
        balls[i].accel_x = 0;
        balls[i].accel_y = 0;

        balls[i].width = 20;
        balls[i].height = 20;

        balls[i].density = 1e5;
        balls[i].mass = balls[i].density * ((float)(balls[i].width * balls[i].height));


        balls[i].x = (i+1) * 505  % (WINDOW_WIDTH - balls[i].width);
        balls[i].y = (i+1) * 200  % (WINDOW_HEIGHT - balls[i].height);
    }

    player->x = 1000;
    player->y = 800;
}


/******************************************
* Applies the x and y speed
* to the x and y coordiantes of the balls
* Handles collision of the balls
* and the window border.
******************************************/
void update_ball_movement(float delta_time)
{
    for (int i = 0; i < BALLS; i++)
    {
        //is the ball inside of the window borders?, if not flip the sign of the x or y speed
        //left or right border
        if ((balls[i].x > (WINDOW_WIDTH - balls[i].width)) || (balls[i].x < 0))
        {
            balls[i].speed_x *= -1;
        }
        //bottom or top border
        if ((balls[i].y > (WINDOW_HEIGHT - balls[i].height)) || (balls[i].y < 0))
        {
            balls[i].speed_y *= -1;
        }

        balls[i].speed_x += balls[i].accel_x * delta_time;
        balls[i].speed_y += balls[i].accel_y * delta_time;

        balls[i].x += balls[i].speed_x * delta_time;
        balls[i].y += balls[i].speed_y * delta_time;

    }
}

void apply_gravity()
{
    for (int i = 0; i < BALLS; i++)
    {
        //P0 is in the center of ball i
        ball_x0 = balls[i].x; //+ (balls[i].width / 2);
        ball_y0 = balls[i].y; //+ (balls[i].height / 2);

        total_gravitional_force_x = 0.0;
        total_gravitional_force_y = 0.0;

        //the mass of the balls is equal to their surface area, so every ball has the same density of 1
        ball_m0 = balls[i].mass;
        for (int j = 0; j < BALLS; j++)
        {
            ball_x1 = balls[j].x;
            ball_y1 = balls[j].y;

            ball_m1 = balls[j].mass;

            vx = ball_x1 - ball_x0;
            vy = ball_y1 - ball_y0;

            length_v = sqrt(vx * vx + vy * vy);

            //if two balls occupy the same space, length_v becomes 0 -> division by zero
            //if two balls get very close their length_v gets very small, which results in a large gravitational force
            //it is required to find a threshold for length_v which stops the increase of the gravitional force
            if (length_v > min_gravity_action_distance) {
                //the normalized vector is multiplied with the gravitational force
                total_gravitional_force_x += (vx / length_v) * G * ((ball_m0 * ball_m1) / (length_v * length_v + softening_factor * softening_factor)); // FG = G * m1 * m2 / r� + softening_factor�
                total_gravitional_force_y += (vy / length_v) * G * ((ball_m0 * ball_m1) / (length_v * length_v + softening_factor * softening_factor));
            }

        }

        //don't apply gravity to the player (mouse)
        if (i != 0) {
          balls[i].accel_x = total_gravitional_force_x / ball_m0; // F = m * a -> a = F / m
          balls[i].accel_y = total_gravitional_force_y / ball_m0;
        }


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
            //measure Scancode, what Key triggered the event?

            if (SDL_GetKeyFromScancode(SDLK_ESCAPE ,SDL_KMOD_NONE, 1) ) //Escape is pressed
            {
                game_is_running = FALSE;
                break;
            }



            //SDL_GetKeyFromScancode(SDL_SCANCODE_W, SDL_KMOD_NONE, true) == SDLK_W doesn't work and crashes the game
            if (SDL_GetKeyFromScancode(SDL_SCANCODE_W, SDL_KMOD_NONE, true) == SDLK_W) //Escape is pressed
            {
               player->y -= 10;
            }
            if (SDL_GetKeyFromScancode(SDL_SCANCODE_S, SDL_KMOD_NONE, true) == SDLK_S) //Escape is pressed
            {
               player->y += 10;
            }
            if (SDL_GetKeyFromScancode(SDL_SCANCODE_A, SDL_KMOD_NONE, true) == SDLK_A) //Escape is pressed
            {
               player->x -= 10;
            }
            if (SDL_GetKeyFromScancode(SDL_SCANCODE_D, SDL_KMOD_NONE, true) == SDLK_D) //Escape is pressed
            {
               player->x += 10;
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            mouseX = (uint16_t)event.motion.x;
            mouseY = (uint16_t)event.motion.y;

            player->x = mouseX;
            player->y = mouseY;
            break;



    }
}


/************************
* Gets called only once.
*************************/
void setup()
{
    initialize_balls();

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


    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;

    last_frame_time = SDL_GetTicks();



    apply_gravity();
    update_ball_movement(delta_time);

    for (unsigned int i = 0; i < BALLS; i++)
    {
        initial_y = (int)balls[i].x / WINDOW_WIDTH;
        for (unsigned int w = 0; w < balls[i].width; w++)
        {
            //prevents unexpected behavior when a ball is near the right window border
            if (((int)balls[i].x + w) / WINDOW_WIDTH == initial_y)
            {
                traversability_map[(int)balls[i].x + w] = 0;
            }

        }

    }


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

    int ball_x;
    int ball_y;

    //start at 1 to not render the player (ball following the mouse)
    for (int i = 1; i < BALLS; i++) {

        //Draw rectangle
        SDL_FRect ball_rect =
        {
            //Cast the values to int, because we are using pixels,
            //And pixels can't be divided.
            ball_x = (int)balls[i].x,
            ball_y = (int)balls[i].y,
            balls[i].width,
            balls[i].height
        };

        SDL_SetRenderDrawColor(renderer, i % 2 * 255, 50, (i+1) % 2 * 255, 255); //4 colors(ugly): i % 3 * 255, (i + 1) % 3 * 255, (i + 2) % 3 * 255, 255
        SDL_RenderFillRect(renderer, &ball_rect);
    }


    //SDL_Rect player =
    //{
    //    (int)player.x,
    //    (int)player.y
    //};
    //SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    //SDL_RenderFillRect(renderer, &player);



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
int main(int argc, char* args[])
{
    game_is_running = initialize_window();

    setup();

    printf("Game is running...\n");
    while (game_is_running) //Game Loop
    {
        process_input();
        update();
        render();
    }

    destroy_window();

    return 0;
}
