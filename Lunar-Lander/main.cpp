/**
* Author: Matthew Gong
* Assignment: Lunar Lander
* Date due: 2023-11-08, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
//#define PLATFORM_COUNT 12

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
//#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
    int state;
};

// ––––– CONSTANTS ––––– //

int PLATFORM_COUNT = 0;
const glm::vec3 GRAVITY = {0.0,-1.0, 0.0};
const float THRUSTER_ACC = 3.0f;
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.0f,
            BG_BLUE    = 0.0f,
            BG_GREEN   = 0.0f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char SPRITESHEET_FILEPATH[] = "resources/lunar.png";
const char FONTSHEET_FILEPATH[] = "resources/font1.png";
const char PLATFORM_FILEPATH[]    = "resources/red1.png";
const char PLATFORM_GOOD_FILEPATH[]    = "resources/green1.png";

const std::string WIN_TEXT = "Mission accomplished!";
const std::string FAIL_TEXT = "Mission failed";


const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;
int text_ani = 0;

const int ROW = 7;
const int COLM = 10;
unsigned int LEVEL_DATA[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 3, 0,
    2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
const int MAX_TEXT_SIZE = 40;

Entity word[MAX_TEXT_SIZE];
Entity fuel_10;
Entity fuel_1;
const int START_FUEL = 297;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;


// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}
void print_text(std::string s){
    size_t i =0;
    for(; i < s.size(); i++){
        word[i].update_position(glm::vec3(i*(0.4) - 4.5f, 3.0f, 0.f));
        word[i].m_animation_index = 0;
        word[i].m_walking[word[i].LEFT][0] = static_cast<int>(s[i]);
        word[i].render(&g_program);
    }
    for(;i < MAX_TEXT_SIZE; i++){
        word[i].m_animation_index = 0;
        word[i].m_walking[word[i].LEFT][0] = static_cast<int>(' ');
        word[i].render(&g_program);
    }
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("Lunar Landing!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);
    GLuint platform_good_texture_id = load_texture(PLATFORM_GOOD_FILEPATH);
    PLATFORM_COUNT = 0;
    for(int i =0; i < ROW; i++){
        for(int j =0; j < COLM; j++){
            if(LEVEL_DATA[i*COLM+j]){
                PLATFORM_COUNT++;
            }
        }
    }
    g_state.platforms = new Entity[PLATFORM_COUNT];
    
    glm::vec3 player_start = glm::vec3(0.0f);
    int plat = 0;
    for(int i =0; i < ROW; i++){
        for(int j =0; j < COLM; j++){
            if(LEVEL_DATA[i*COLM+j]){
                if(LEVEL_DATA[i*COLM+j] > 1)
                    g_state.platforms[plat].m_texture_id = platform_good_texture_id;
                else
                    g_state.platforms[plat].m_texture_id = platform_texture_id;
                g_state.platforms[plat].set_position(glm::vec3(j - 4.5f, (-i)+3.0f, 0.0f));
                g_state.platforms[plat].set_width(0.4f);
                g_state.platforms[plat].set_entity_type(static_cast<EntityType>(LEVEL_DATA[i*COLM+j]));
                g_state.platforms[plat].update(0.0f, g_state.player, NULL, 0);
                plat++;
            }
            if(LEVEL_DATA[i*COLM+j] == 2){
                player_start = glm::vec3(j - 4.5f, (-i)+3.0f, 0.0f);
            }
        }
    }
    
    // Existing
    g_state.player = new Entity();
    g_state.player->set_position(player_start);
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->set_speed(1.0f);
    g_state.player->set_acceleration(glm::vec3(0.0f, -4.905f, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    g_state.player->set_thurst(THRUSTER_ACC);
    g_state.player->set_fuel(START_FUEL);
    
    // Walking
    g_state.player->set_height(0.9f);
    g_state.player->set_width(0.9f);
    g_state.player->set_entity_type(PLAYER);
    


    for(size_t i =0; i < MAX_TEXT_SIZE; i++){
        word[i].set_position(glm::vec3(i - 4.5f, 3.0f, 0.f));
        word[i].m_texture_id = load_texture(FONTSHEET_FILEPATH);
        word[i].set_entity_type(TEXT);
        word[i].m_animation_cols = 16;
        word[i].m_animation_rows = 16;
        word[i].m_animation_index = 0;

        word[i].m_walking[word[i].LEFT]  = new int[4] {0,0,0,0};
        word[i].m_animation_indices = word[i].m_walking[word[i].LEFT];
    }
    
    //for fuel
    fuel_10.set_position(glm::vec3(-4.5f, -3.0f, 0.f));
    fuel_10.m_texture_id = load_texture(FONTSHEET_FILEPATH);
    fuel_10.set_entity_type(TEXT);
    fuel_10.m_animation_cols = 16;
    fuel_10.m_animation_rows = 16;
    fuel_10.m_animation_index = 0;

    fuel_10.m_walking[fuel_10.LEFT]  = new int[4] {0,0,0,0};
    fuel_10.m_animation_indices = fuel_10.m_walking[fuel_10.LEFT];
    
    
    fuel_1.set_position(glm::vec3(0.4-4.5f, -3.0f, 0.f));
    fuel_1.m_texture_id = load_texture(FONTSHEET_FILEPATH);
    fuel_1.set_entity_type(TEXT);
    fuel_1.m_animation_cols = 16;
    fuel_1.m_animation_rows = 16;
    fuel_1.m_animation_index = 0;
    fuel_1.m_walking[fuel_1.LEFT]  = new int[4] {0,0,0,0};
    fuel_1.m_animation_indices = fuel_1.m_walking[fuel_1.LEFT];
    
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));
    //reset thrust
    g_state.player->set_applied_acceleration(glm::vec3(0.0f));
    //reset gravity 
    g_state.player->set_acceleration(GRAVITY);
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                    
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->move_left();

    }
    if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->move_right();
    }
    
    if (key_state[SDL_SCANCODE_UP])
    {
        g_state.player->move_up();
    }
    if (key_state[SDL_SCANCODE_DOWN])
    {
        g_state.player->move_down();
    }
    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->set_movement(
                                     glm::normalize(
                                                    g_state.player->get_movement()
                                                    )
                                     );
        g_state.player->normalize_app_acc();
    }
    g_state.player->set_fuel( std::max(g_state.player->get_fuel(),0));
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.state =  g_state.player->update(FIXED_TIMESTEP, g_state.player, g_state.platforms, PLATFORM_COUNT);
        delta_time -= FIXED_TIMESTEP;
    }
//    std::cout << "VEL: " << g_state.player->get_velocity().x << " " << g_state.player->get_velocity().y  << std::endl;
//    std::cout << "APP_ACC: " << g_state.player->get_applied_acceleration().x << " " << g_state.player->get_applied_acceleration().y  << std::endl;
//    std::cout << "ACC: " << g_state.player->get_acceleration().x << " " << g_state.player->get_acceleration().y  << std::endl;
    
    g_accumulator = delta_time;
//    if(g_state.state){
//        g_game_is_running = false;
//    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_state.player->render(&g_program);
    if(g_state.state == 1)
        print_text(WIN_TEXT);
    if(g_state.state == 2 )
        print_text(FAIL_TEXT);
    
    int scale_fuel = g_state.player->get_fuel()/(START_FUEL/99);
//    std::cout << scale_fuel << std::endl;
    fuel_10.update_position(glm::vec3(-4.5f, -3.0f, 0.f));
    fuel_10.m_animation_index = 0;
    fuel_10.m_walking[fuel_1.LEFT][0] = scale_fuel/10+'0';
    fuel_10.render(&g_program);
    fuel_1.update_position(glm::vec3(-4.1f, -3.0f, 0.f));
    fuel_1.m_animation_index = 0;
    fuel_1.m_walking[fuel_1.LEFT][0] = scale_fuel%10+'0';
    fuel_1.render(&g_program);
    
    
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        if(!g_state.state){
            update();
        }
        render();
    }
    
    shutdown();
    return 0;
}
