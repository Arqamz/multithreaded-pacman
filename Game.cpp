#include <SFML/Graphics.hpp>
#include <random>
#include <fstream>
#include <time.h>
#include <iostream> // Added for cout

#include <map>

const float TSIZE = 8;
//const float MSPEED = 0.3;
const int BSIZE = (28 + 1) * 31;
const int YOFFSET = 8 * 3;

enum State
{
    MENU,
    GAMESTART,
    MAINLOOP,
    GAMEWIN,
    GAMELOSE,
    GAMEOVER,
};

struct Player
{
	sf::Vector2f pos;
	Dir cur_dir;
	Dir correction;
	Dir try_dir;

	bool stopped;
	bool cornering;
	bool enable_draw = true;
};

struct GameState
{
    State game_state;

    int current_level = 1;
    int player_lives = 0;
    int game_score = 0;
    int high_score;
    int wave_counter = 0;

    bool pellet_eaten = false;

    bool first_life = true;
    bool using_global_counter = false;
    int global_dot_counter = 0;

    bool player_eat_ghost = false;
    int ghosts_eaten_in_powerup = 0;

    Player* player;

    std::vector<std::string> board;

    // Timers in milliseconds
    int energizer_time = 0;
    int pause_time = 0;
    int wave_time = 0;

    sf::RenderWindow* window;
};

GameState gState;

struct Textures
{
    sf::Texture pellets;
    sf::Texture sprites;
    sf::Texture wall_map_t;
    sf::Texture wall_map_t_white;

    sf::Texture font;
};
struct RenderItems
{
    sf::VertexArray pellet_va;
    sf::VertexArray sprite_va;
    sf::VertexArray wall_va;
    sf::Sprite wall_map;

    sf::Sprite float_score;

    sf::Sprite player;
    sf::Sprite ghosts[4];

    // instead of rebuilding pellet vertex array every frame, store va index of
    // each pellet location then only delete the one eaten
    std::map<int, int> pellet_va_indicies;

    // also keep index of power up pellets so they can be flashed
    int pow_indicies[4];

    sf::VertexArray text_va;

    bool pow_is_off = false;
};

const int font_width = 14;

// pellet rects
const sf::FloatRect pel_r = {0, 0, 16, 16};
const sf::FloatRect pow_r = {16, 0, 16, 16};

void InitRender();
void InitWalls();
void InitTextures();
void InitPellets();
void RemovePellet(int x, int y);
void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h,
               sf::Color color = {255, 255, 255}, sf::FloatRect tex_rect = {0, 0, 0, 0});
void DrawFrame();

void FlashPPellets();
void ResetPPelletFlash();

static RenderItems RItems;
static Textures RTextures;

void InitRender()
{
    InitTextures();
    InitWalls();
    RItems.pellet_va.setPrimitiveType(sf::Quads);
    RItems.sprite_va.setPrimitiveType(sf::Quads);
    RItems.text_va.setPrimitiveType(sf::Quads);

    RItems.wall_map.setTexture(RTextures.wall_map_t);
    RItems.wall_map.setScale({0.5, 0.5});

    InitPellets();

    for (int i = 0; i < 4; i++)
    {
        RItems.ghosts[i].setTexture(RTextures.sprites);
        RItems.ghosts[i].setScale({0.5, 0.5});
        RItems.ghosts[i].setOrigin({16, 16});
    }

    RItems.player.setTexture(RTextures.sprites);
    RItems.player.setScale({0.5, 0.5});
    RItems.player.setOrigin({15, 15});

    RItems.float_score.setTexture(RTextures.sprites);
    RItems.float_score.setScale({0.5, 0.5});
    RItems.float_score.setOrigin({16, 16});

    std::cout << "Initialization of RenderItems completed." << std::endl;
}
void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h, sf::Color color, sf::FloatRect t_rect)
{
    sf::Vertex vert;
    vert.color = color;
    vert.position = {x, y};
    vert.texCoords = {t_rect.left, t_rect.top};
    va.append(vert);
    vert.position = {x + w, y};
    vert.texCoords = {t_rect.left + t_rect.width, t_rect.top};
    va.append(vert);
    vert.position = {x + w, y + h};
    vert.texCoords = {t_rect.left + t_rect.width, t_rect.top + t_rect.height};
    va.append(vert);
    vert.position = {x, y + h};
    vert.texCoords = {t_rect.left, t_rect.top + t_rect.height};
    va.append(vert);
}

void InitWalls()
{
    RItems.wall_va.setPrimitiveType(sf::Quads);
    for (int y = 0; y < gState.board.size(); y++)
    {
        for (int x = 0; x < gState.board.at(y).size(); x++)
        {
            if (gState.board.at(y).at(x) == '|')
            {
                MakeQuad(RItems.wall_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, {150, 150, 150});
            }
        }
    }

    std::cout << "Initialization of walls completed." << std::endl;
}

void InitTextures()
{
    RTextures.pellets.loadFromFile("textures/dots.png");
    RTextures.sprites.loadFromFile("textures/sprites.png");
    RTextures.wall_map_t.loadFromFile("textures/map.png");
    RTextures.font.loadFromFile("textures/font.png");
    RTextures.wall_map_t_white.loadFromFile("textures/map.png");

    RTextures.wall_map_t_white.loadFromFile("textures/map_white.png");

    std::cout << "Initialization of textures completed." << std::endl;
}

inline char GetTile(int x, int y)
{
    if (x < 0 || x >= gState.board.at(y).size())
        return '/';

    return gState.board.at(y).at(x);
}

void InitPellets()
{
    RItems.pellet_va.clear();
    int VA_Index = 0;
    int Pow_index = 0;
    for (int y = 0; y < gState.board.size(); y++)
    {
        for (int x = 0; x < gState.board.at(y).size(); x++)
        {
            char temp = GetTile(x, y);
            if (temp == '.')
            {
                MakeQuad(RItems.pellet_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, {255, 255, 255}, pel_r);
                RItems.pellet_va_indicies.insert({y * 28 + x, VA_Index});
                VA_Index += 4;
            }
            else if (temp == 'o')
            {
                MakeQuad(RItems.pellet_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, {255, 255, 255}, pow_r);
                RItems.pellet_va_indicies.insert({y * 28 + x, VA_Index});
                RItems.pow_indicies[Pow_index] = VA_Index;
                Pow_index++;
                VA_Index += 4;
            }
        }
    }

    std::cout << "Initialization of pellets completed." << std::endl;
}

void RemovePellet(int x, int y)
{
    int va_idx = RItems.pellet_va_indicies.at(y * 28 + x);
    sf::Vertex* vert = &RItems.pellet_va[va_idx];
    vert[0].color = {0, 0, 0, 0};
    vert[1].color = {0, 0, 0, 0};
    vert[2].color = {0, 0, 0, 0};
    vert[3].color = {0, 0, 0, 0};

    std::cout << "Pellet at (" << x << ", " << y << ") removed." << std::endl;
}

void FlashPPellets()
{
    sf::Uint8 new_alpha = (RItems.pow_is_off) ? 255 : 1;

    for (int i = 0; i < 4; i++)
    {
        int index = RItems.pow_indicies[i];
        sf::Vertex* vert = &RItems.pellet_va[index];
        // I am using alpha 0 to hide pellets, so for flashing, Ill just use alpha 1
        if (vert->color.a == 0)
            continue;
        vert[0].color.a = new_alpha;
        vert[1].color.a = new_alpha;
        vert[2].color.a = new_alpha;
        vert[3].color.a = new_alpha;
    }

    std::cout << "Flashing power pellets completed." << std::endl;
}

void ResetPPelletFlash()
{
    RItems.pow_is_off = true;
    FlashPPellets();

    std::cout << "Resetting power pellet flash completed." << std::endl;
}

void InitBoard()
{
    std::string line;
    std::ifstream infile("Map.txt");
    if (!infile)
        return;
    while (getline(infile, line))
    {
        gState.board.push_back(line);
    }

    infile.close();

    std::cout << "Board initialization completed." << std::endl;
}

bool IsPulse()
{
    return true;
}

void DrawFrame()
{
      std::cout << "Entering DrawFrame()" << std::endl;

      gState.window->clear(sf::Color::Black);

      std::cout << "Cleared window" << std::endl;

      if (RItems.pow_is_off != IsPulse())
      {
            FlashPPellets();
            RItems.pow_is_off = !RItems.pow_is_off;
      }

      std::cout << "Checked power pellets" << std::endl;

      static bool white_texture = false;
      if (gState.game_state == GAMEWIN)
      {
            if (IsPulse() && !white_texture)
            {
                  RItems.wall_map.setTexture(RTextures.wall_map_t_white);
                  white_texture = true;
            }
            else if (!IsPulse() && white_texture)
            {
                  RItems.wall_map.setTexture(RTextures.wall_map_t);
                  white_texture = false;
            }
      }

      std::cout << "Checked game state" << std::endl;

      gState.window->draw(RItems.wall_map);
      gState.window->draw(RItems.text_va, &RTextures.font);
      gState.window->draw(RItems.pellet_va, &RTextures.pellets);

      std::cout << "Drew wall map, text, and pellets" << std::endl;

      gState.window->display();

      std::cout << "Displayed window" << std::endl;

      std::cout << "Exiting DrawFrame()" << std::endl;
}

int main()
{
    srand(time(NULL));
    sf::RenderWindow window(sf::VideoMode(28.f * TSIZE * 2, 36.f * TSIZE * 2), "PACMAN");
    window.setFramerateLimit(60);
    window.setView(sf::View({0, 0, 28.f * TSIZE, 36.f * TSIZE}));
    gState.window = &window;

    std::cout << "Window created." << std::endl;

    InitBoard();

      InitRender();
      InitWalls();
      InitTextures();
      InitPellets();

    sf::Clock clock;
    sf::Time elapsed;

    std::cout << "Entering game loop." << std::endl;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            case sf::Event::KeyPressed:
                switch (event.key.code)
                {
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                }
            }
        }
        elapsed = clock.restart();
        std::cout << "Going to draw\n";
        DrawFrame();
    }

    std::cout << "Game loop ended." << std::endl;

    return 0;
}
