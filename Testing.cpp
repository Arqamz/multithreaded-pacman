#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <random>
#include <time.h>
#include <iostream>

#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include <vector>
#include <map>

// PACMAN.h
enum Dir
{
	NONE,
	UP,
	DOWN,
	LEFT,
	RIGHT,
};

enum State
{
	MENU,
	GAMESTART,
	MAINLOOP,
	GAMEWIN,
	GAMELOSE,
	GAMEOVER,
};
const float TSIZE = 8;
//const float MSPEED = 0.3;
const int BSIZE = (28 + 1) * 31;
const int YOFFSET = 8 * 3;
const float win_ratio = 28.f / 36.f;

const Dir opposite_dir[5] = { NONE, DOWN, UP, RIGHT, LEFT };
const sf::Vector2f dir_addition[5] = { {0,0}, {0,-1},{0,1},{-1,0},{1,0} };

// total amount of pellets in maze
const int pellet_amt = 244;
const int pill_score = 10;
const int pow_score = 50;

const float fullmovement = 0.2;
const float player_speed = fullmovement * 0.8;
const float player_fright = fullmovement * 0.9;
const float ghost_speed = fullmovement * 0.75;
const float ghost_fright = fullmovement * 0.5;

const float reg_speed = 0.15;
const float gohome_speed = 0.4;
const float frightened_speed = 0.1;
const float inhome_speed = 0.05;

//
// Global states
//

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

	int pellets_left = pellet_amt;
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

extern GameState gState;

//
// General Functions
//
bool TileCollision(sf::Vector2f pos, bool home_tiles = false);
bool PlayerTileCollision(Dir dir, sf::Vector2f pos);
void CenterObject(Dir dir, sf::Vector2f& pos);
inline bool FComp(float a, float b)
{
	float diff = a - b;
	return (diff < 0.01f) && (-diff < 0.01f);
}
inline bool InTunnel(sf::Vector2f pos)
{
	return pos.x < 0 || pos.x > 27;
}

inline char GetTile(int x, int y)
{
	// for tunnel, / not used for anything
	if (x < 0 || x >= gState.board.at(y).size())
		return '/';

	return gState.board.at(y).at(x);
}
inline void SetTile(int x, int y, char new_c)
{
	// for tunnel, / not used for anything
	if (x < 0 || x >= gState.board.at(y).size())
		return;

	 gState.board.at(y).at(x) = new_c;
}
inline sf::Vector2f operator * (sf::Vector2f vec, float num)
{
	return { vec.x * num, vec.y * num };
}

bool TileCollision(sf::Vector2f pos, bool home_tiles)
{
	bool collided = false;
	char tile = '|';

	tile = GetTile(pos.x, pos.y);

	if (tile == '|')
		collided = true;
	if (!home_tiles && tile == '-')
		collided = true;
	return collided;
}

bool PlayerTileCollision(Dir dir, sf::Vector2f pos)
{
	bool collided = false;
	char tile = '|';
	switch (dir)
	{
	case UP:
		tile = GetTile(pos.x, pos.y - 0.51);
		break;
	case DOWN:
		tile = GetTile(pos.x, pos.y + 0.51);
		break;
	case LEFT:
		tile = GetTile(pos.x - 0.51, pos.y);
		break;
	case RIGHT:
		tile = GetTile(pos.x + 0.51, pos.y);
		break;
	}
	if (tile == '|' || tile == '-')
		collided = true;
	return collided;
}
void CenterObject(Dir dir, sf::Vector2f& pos)
{
	switch (dir)
	{
	case UP:
	case DOWN:
		pos.x = (int)pos.x + 0.5;
		break;
	case LEFT:
	case RIGHT:
		pos.y = (int)pos.y + 0.5;
		break;
	}
}

// Player.h


Dir GetCorrection(Dir pdir, sf::Vector2f ppos);
void Cornering();
void ResolveCollision();
void PlayerMovement();

Dir GetCorrection(Dir pdir, sf::Vector2f ppos)
{
	switch (pdir)
	{
	case UP:
	case DOWN:
		if (ppos.x - (int)ppos.x >= 0.5)
			return LEFT;
		return RIGHT;
		break;
	case LEFT:
	case RIGHT:
		if (ppos.y - (int)ppos.y >= 0.5)
			return UP;
		return DOWN;
		break;
	}
}
void Cornering()
{
	gState.player->pos += dir_addition[gState.player->correction] * player_speed;
	bool done = false;
	switch (gState.player->correction)
	{
	case UP:
		done = (gState.player->pos.y - (int)gState.player->pos.y <= 0.5);
		break;
	case DOWN:
		done = (gState.player->pos.y - (int)gState.player->pos.y >= 0.5);
		break;
	case LEFT:
		done = (gState.player->pos.x - (int)gState.player->pos.x <= 0.5);
		break;
	case RIGHT:
		done = (gState.player->pos.x - (int)gState.player->pos.x >= 0.5);
		break;
	}
	if (done) {
		CenterObject(gState.player->cur_dir, gState.player->pos);
		gState.player->cornering = false;
	}
}
void ResolveCollision()
{
	switch (gState.player->cur_dir) {
	case UP:
		gState.player->pos.y = (int)gState.player->pos.y + 0.5;
		break;
	case DOWN:
		gState.player->pos.y = (int)gState.player->pos.y + 0.5;
		break;
	case LEFT:
		gState.player->pos.x = (int)gState.player->pos.x + 0.5;
		break;
	case RIGHT:
		gState.player->pos.x = (int)gState.player->pos.x + 0.5;
		break;
	}
}
void PlayerMovement()
{
	Dir try_dir = NONE;

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		try_dir = UP;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
		try_dir = DOWN;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		try_dir = RIGHT;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		try_dir = LEFT;

	if (try_dir != NONE &&
		!PlayerTileCollision(try_dir, gState.player->pos) &&
		!InTunnel(gState.player->pos)) {
		gState.player->cur_dir = try_dir;
		// dont need to corner if its opposite direction
		if (gState.player->cur_dir != opposite_dir[gState.player->cur_dir]) {
			gState.player->cornering = true;
			gState.player->correction = GetCorrection(try_dir, gState.player->pos);
		}
	}

	if (!gState.player->stopped) {
		gState.player->pos += dir_addition[gState.player->cur_dir] * player_speed;
	}

	if (gState.player->cornering) {
		Cornering();
	}

	if (PlayerTileCollision(gState.player->cur_dir, gState.player->pos)) {
		ResolveCollision();
		gState.player->stopped = true;
	}
	else {
		gState.player->stopped = false;
	}

	// tunneling
	if (gState.player->pos.x < -1) {
		gState.player->pos.x += 29;
	}
	else if (gState.player->pos.x >= 29) {
		gState.player->pos.x -= 29;
	}
}

//RENDER.H

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
const sf::FloatRect pel_r = { 0,0,16,16 };
const sf::FloatRect pow_r = { 16,0,16,16 };

void InitRender();
void InitWalls();
void InitTextures();
void InitPellets();
void ResetPellets();
void RemovePellet(int x, int y);
void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h, 
	sf::Color color = { 255,255,255 }, sf::FloatRect tex_rect = { 0,0,0,0 });
void DrawGameUI();
void DrawFrame();

void FlashPPellets();
void ResetPPelletFlash();

void DrawMenuFrame();

// Text drawing
// Ive tried using SFML's text class in the past but it ends up
// looking blurry at low resolution, this is a simple way of drawing text
// using the original lettering
void ClearText();
void MakeText(std::string string, int x, int y, sf::Color f_color);

// ANIMATE.H

struct Animation
{
	int pacman_frame=0;
	int pacman_timer=0;
	bool assending = true;
	
	bool ghost_frame_2 = false;
	int ghost_timer = 0;

	int fright_flash = false;
	int energrizer_timer = 0;

	bool death_animation = false;

	// genertic pulse for win condition and energizer
	int pulse = true;
	int pulse_timer = 0;

	int pulse_limit = 200;
};

void AnimateUpdate(int ms_elapsed);
void StartPacManDeath();
void ResetAnimation();
void SetPacManMenuFrame();

sf::IntRect GetPacManFrame(Dir dir);

void PulseUpdate(int ms_elapsed);
bool IsPulse();
void SetPulseFrequency(int ms);

// ANIMATE.cpp


static Animation animate;

void AnimateUpdate(int ms_elapsed)
{
	PulseUpdate(ms_elapsed);

	if (gState.player->stopped && !animate.death_animation) {
		animate.pacman_frame = 0;
		animate.assending = true;
	}
	else {
		animate.pacman_timer += ms_elapsed;
	}
	
	if (animate.death_animation) {
		if (animate.pacman_timer > 100) {
			animate.pacman_timer = 0;
			animate.pacman_frame++;
		}
		if (animate.pacman_frame > 10)
			gState.player->enable_draw = false;
	}
	else if (animate.pacman_timer > 25 && !gState.player->stopped) {
		animate.pacman_frame += (animate.assending) ? 1 : -1;
		animate.pacman_timer = 0;
	}
	if (!animate.death_animation && animate.pacman_frame > 2 || animate.pacman_frame < 0) {
		animate.assending = !animate.assending;
		animate.pacman_frame = (animate.pacman_frame > 2) ? 2 : 0;
	}

	animate.ghost_timer += ms_elapsed;
	if (animate.ghost_timer > 200) {
		animate.ghost_frame_2 = !animate.ghost_frame_2;
		animate.ghost_timer = 0;
	}

	// start flashing with 2 seconds to go
	if (gState.energizer_time > 0 && gState.energizer_time < 2000) {
		animate.energrizer_timer += ms_elapsed;
		if (animate.energrizer_timer > 200) {
			animate.fright_flash = !animate.fright_flash;
			animate.energrizer_timer = 0;
		}
	}
	else animate.fright_flash = false;
}
sf::IntRect GetPacManFrame(Dir dir)
{
	sf::IntRect rect = { 0,0,30,30 };
	rect.left = (2 - animate.pacman_frame) * 32;

	if (animate.death_animation) {
		rect.left = 96 + animate.pacman_frame * 32;

		return rect;
	}

	if (animate.pacman_frame == 0)
		return rect;

	switch (dir)
	{
	case UP:
		rect.top = 64;
		break;
	case DOWN:
		rect.top = 96;
		break;
	case LEFT:
		rect.top = 32;
	}

	return rect;
}
void StartPacManDeath()
{
	animate.death_animation = true;
	animate.pacman_frame = 0;
	animate.pacman_timer = -250;
}
void ResetAnimation()
{
	animate.pacman_frame = 0;
	animate.death_animation = false;
}
void SetPacManMenuFrame()
{
	animate.pacman_frame = 1;
	animate.death_animation = false;

	animate.ghost_frame_2 = false;
}
void PulseUpdate(int ms_elapsed)
{
	animate.pulse_timer += ms_elapsed;
	if (animate.pulse_timer > animate.pulse_limit) {
		animate.pulse = !animate.pulse;
		animate.pulse_timer = 0;
	}
}
void SetPulseFrequency(int ms)
{
	animate.pulse_limit = ms;
}
bool IsPulse()
{
	return animate.pulse;
}


// RENDER.cpp
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
	RItems.wall_map.setScale({ 0.5,0.5 });

	InitPellets();

	RItems.player.setTexture(RTextures.sprites);
	RItems.player.setScale({ 0.5,0.5 });
	RItems.player.setOrigin({ 15,15 });

	RItems.float_score.setTexture(RTextures.sprites);
	RItems.float_score.setScale({ 0.5,0.5 });
	RItems.float_score.setOrigin({ 16,16 });

}
void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h, sf::Color color, sf::FloatRect t_rect)
{
	sf::Vertex vert;
	vert.color = color;
	vert.position = { x,y };
	vert.texCoords = { t_rect.left,t_rect.top };
	va.append(vert);
	vert.position = { x + w,y };
	vert.texCoords = { t_rect.left+t_rect.width,t_rect.top };
	va.append(vert);
	vert.position = { x + w,y + h };
	vert.texCoords = { t_rect.left+t_rect.width,t_rect.top+t_rect.height };
	va.append(vert);
	vert.position = { x,y + h };
	vert.texCoords = { t_rect.left,t_rect.top+t_rect.height };
	va.append(vert);
}
void InitWalls()
{
	RItems.wall_va.setPrimitiveType(sf::Quads);
	for (int y = 0; y < gState.board.size(); y++) {
		for (int x = 0; x < gState.board.at(y).size(); x++) {
			if(gState.board.at(y).at(x)== '|')
				MakeQuad(RItems.wall_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, { 150,150,150 });
		}
	}
}
void InitTextures()
{
	RTextures.pellets.loadFromFile("textures/dots.png");
	RTextures.sprites.loadFromFile("textures/sprites.png");
	RTextures.wall_map_t.loadFromFile("textures/map.png");
	RTextures.font.loadFromFile("textures/font.png");
	RTextures.wall_map_t_white.loadFromFile("textures/map.png");

	RTextures.wall_map_t_white.loadFromFile("textures/map_white.png");
}

void InitPellets()
{
	RItems.pellet_va.clear();
	int VA_Index = 0;
	int Pow_index = 0;
	for (int y = 0; y < gState.board.size(); y++) {
		for (int x = 0; x < gState.board.at(y).size(); x++) {
			char temp = GetTile(x, y);
			if (temp == '.') {
				MakeQuad(RItems.pellet_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, { 255,255,255 }, pel_r);
				RItems.pellet_va_indicies.insert({ y * 28 + x, VA_Index });
				VA_Index += 4;
			}
			else if (temp == 'o') {
				MakeQuad(RItems.pellet_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, { 255,255,255 }, pow_r);
				RItems.pellet_va_indicies.insert({ y * 28 + x, VA_Index });
				RItems.pow_indicies[Pow_index] = VA_Index;
				Pow_index++;
				VA_Index += 4;
			}
		}
	}
}
void ResetPellets()
{
	for (int i = 0; i < RItems.pellet_va.getVertexCount(); i++) {
		RItems.pellet_va[i].color = { 255,255,255,255 };
	}

	RItems.wall_map.setTexture(RTextures.wall_map_t);
}
void RemovePellet(int x, int y)
{
	int va_idx = RItems.pellet_va_indicies.at(y * 28 + x);
	sf::Vertex* vert = &RItems.pellet_va[va_idx];
	vert[0].color = { 0,0,0,0 };
	vert[1].color = { 0,0,0,0 };
	vert[2].color = { 0,0,0,0 };
	vert[3].color = { 0,0,0,0 };
}
void DrawGameUI()
{
	ClearText();
	
	MakeText("High Score", 9, 0, { 255,255,255 });
	
	if (gState.game_state == GAMESTART)
		MakeText("Ready!", 11, 20, { 255,255,00 });
	else if (gState.game_state == GAMEOVER)
		MakeText("GAME  OVER", 9, 20, { 255,0,00 });

	std::string score  = std::to_string(gState.game_score);
	if (score.size() == 1)
		score.insert(score.begin(), '0');
	MakeText(score, 7-score.size(), 1, { 255,255,255 });

	score = std::to_string(gState.high_score);
	if (score.size() == 1)
		score.insert(score.begin(), '0');
	MakeText(score, 17 - score.size(), 1, { 255,255,255 });

	RItems.player.setTextureRect({ 256,32,30,30 });
	for (int i = 0; i < gState.player_lives; i++) {
		RItems.player.setPosition({ 24.f + 16 * i,35 * TSIZE });
		gState.window->draw(RItems.player);
	}
}
void FlashPPellets()
{
	sf::Uint8 new_alpha = (RItems.pow_is_off) ? 255 : 1;

	for (int i = 0; i < 4; i++) {
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
}
void ResetPPelletFlash()
{
	RItems.pow_is_off = true;
	FlashPPellets();
}
void DrawFrame()
{
	gState.window->clear(sf::Color::Black);
	DrawGameUI();

	if (RItems.pow_is_off != IsPulse()) {
		FlashPPellets();
		RItems.pow_is_off = !RItems.pow_is_off;
	}

	static bool white_texture = false;
	if (gState.game_state == GAMEWIN) {
		if (IsPulse() && !white_texture) {
			RItems.wall_map.setTexture(RTextures.wall_map_t_white);
			white_texture = true;
		}
		else if (!IsPulse() && white_texture) {
			RItems.wall_map.setTexture(RTextures.wall_map_t);
			white_texture = false;
		}
	}
		
	gState.window->draw(RItems.wall_map);
	gState.window->draw(RItems.text_va, &RTextures.font);
	gState.window->draw(RItems.pellet_va, &RTextures.pellets);
	

	if (gState.player->enable_draw) {
		RItems.player.setPosition(gState.player->pos.x * TSIZE, gState.player->pos.y * TSIZE + YOFFSET);
		RItems.player.setTextureRect(GetPacManFrame(gState.player->cur_dir));
		
		gState.window->draw(RItems.player);
	}

	if (gState.player_eat_ghost) {
		RItems.float_score.setPosition(gState.player->pos.x * TSIZE, gState.player->pos.y * TSIZE + YOFFSET);
		RItems.float_score.setTextureRect({ (gState.ghosts_eaten_in_powerup - 1) * 32,256,32,32 });

		gState.window->draw(RItems.float_score);
	}

	gState.window->display();
}
void DrawMenuFrame()
{
	gState.window->clear(sf::Color::Black);
	DrawGameUI();
	if (IsPulse())
		MakeText("ENTER", 12, 25, { 255,255,255 });
	
	MakeText("-BLINKY", 9, 8, { 255,0,0 });
	MakeText("-PINKY", 9, 11, { 252,181,255 });
	MakeText("-INKY", 9, 14, { 0,255,255 });
	MakeText("-CLYDE", 9, 17, { 248,187,85 });
	MakeText("-PACMAN", 9, 20, { 255,255,0 });

	RItems.player.setPosition(gState.player->pos.x * TSIZE, gState.player->pos.y * TSIZE + YOFFSET);
	RItems.player.setTextureRect(GetPacManFrame(gState.player->cur_dir));

	gState.window->draw(RItems.player);

	gState.window->draw(RItems.text_va, &RTextures.font);

	gState.window->display();
}
void ClearText()
{
	RItems.text_va.clear();
}
void MakeText(std::string string, int x, int y, sf::Color color)
{
	for (int i = 0; i < string.size(); i++) {
		sf::FloatRect font_let = { 0,0,16,16 };
		char letter = toupper(string.at(i));
		font_let.left = (letter - ' ') * 16 + 1;
		MakeQuad(RItems.text_va, x*TSIZE + 8 * i, y*TSIZE, 8, 8, color, font_let);
	}
}

//GAMELOOP.h


void OnStart();
void OnQuit();
void GameLoop(int ms_elapsed);

void Init();
void InitBoard();
void ResetGhostsAndPlayer();
void ResetBoard();
void SetupMenu();

void IncrementGhostHouse();
void CheckPelletCollision();
void CheckWin();

void CheckHighScore();
void LoadHighScore();
void SaveHighScore();

// game states
void MainLoop(int ms_elasped);
void GameStart(int ms_elasped);
void GameLose(int ms_elasped);
void GameWin(int ms_elasped);
void Menu(int ms_elapsed);

//GAMELOOP.cpp

GameState gState;


void OnStart()
{
	Init();
	LoadHighScore();
}
void OnQuit()
{
	SaveHighScore();
}

void LoadHighScore()
{
	std::ifstream infile;
	std::stringstream ss;
	std::string line;
	int hs = 0;
	infile.open("highscore.txt");
	if (!infile) {
		gState.high_score = 0;
		return;
	}
	getline(infile, line);
	ss << line;
	ss >> hs;

	gState.high_score = hs;
}
void SaveHighScore()
{
	std::ofstream outfile("highscore.txt");
	if (!outfile.is_open())
		printf("Cant open file!");
	
	outfile << gState.high_score;
	outfile.close();
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

}
void Init()
{
	Player* pl = new Player();
	pl->cur_dir = UP;
	pl->pos = { 14,23.5 };
	pl->stopped = true;
	gState.player = pl;

	InitBoard();
	InitRender();
	ResetGhostsAndPlayer();
	
	SetupMenu();
	gState.game_state = MENU;
	gState.pause_time = 2000;
}
void ResetGhostsAndPlayer()
{

	gState.player->cur_dir = UP;
	gState.player->pos = { 14,23.5 };
	gState.player->stopped = true;
	gState.player->enable_draw = true;

	ResetAnimation();
	gState.energizer_time = 0;
	gState.wave_counter = 0;
	gState.wave_time = 0;

	ResetPPelletFlash();

	if (!gState.first_life)
		gState.using_global_counter = true;
	
	gState.global_dot_counter = 0;
}
void ResetBoard()
{
	gState.board.clear();
	InitBoard();

	gState.pellets_left = 244;

	gState.first_life = true;
	gState.using_global_counter = false;
}

void CheckPelletCollision()
{
	char tile = GetTile(gState.player->pos.x, gState.player->pos.y);
	bool collided = false;
	if (tile == '.') {
		collided = true;
		gState.game_score += 10;
	}
	else if (tile == 'o') {
		collided = true;
		gState.game_score += 50;
		// gState.energizer_time = fright_time*1000;

		gState.ghosts_eaten_in_powerup = 0;

	}

	if (collided) {
		RemovePellet(gState.player->pos.x, gState.player->pos.y);
		SetTile(gState.player->pos.x, gState.player->pos.y, ' ');
		gState.pellet_eaten = true;
		gState.pellets_left--;
	}
}


void CheckHighScore()
{
	if (gState.game_score > gState.high_score)
		gState.high_score = gState.game_score;
}
void CheckWin()
{
	if (gState.pellets_left <= 0) {
		gState.game_state = GAMEWIN;
		gState.player->stopped = true;
		gState.pause_time = 2000;
		SetPulseFrequency(200);
	}
}

void MainLoop(int ms_elapsed)
{
	if (gState.player_eat_ghost) {
		gState.pause_time -= ms_elapsed;
		if (gState.pause_time < 0) {
			gState.player->enable_draw = true;
			gState.player_eat_ghost = false;
		}
		DrawFrame();

		return;
	}

	if (!gState.pellet_eaten)
		PlayerMovement();
	else gState.pellet_eaten = false;

	CheckPelletCollision();
	CheckHighScore();
	CheckWin();

	AnimateUpdate(ms_elapsed);
	DrawFrame();
}

void GameStart(int ms_elasped)
{
	gState.pause_time -= ms_elasped;
	if (gState.pause_time <= 0) {
		gState.game_state = MAINLOOP;
		SetPulseFrequency(150);
	}

	DrawFrame();
}
void GameLose(int ms_elapsed)
{
	gState.pause_time -= ms_elapsed;
	if (gState.pause_time <= 0) {
		if (gState.player_lives < 0) {
			gState.game_state = GAMEOVER;
			gState.pause_time = 5000;
			gState.player->enable_draw = false;
		}
		else {
			gState.game_state = GAMESTART;
			gState.pause_time = 2000;
			
			ResetGhostsAndPlayer();
		}
	}
	AnimateUpdate(ms_elapsed);
	DrawFrame();
}
void GameWin(int ms_elapsed)
{
	gState.pause_time -= ms_elapsed;
	if (gState.pause_time <= 0) {
		ResetPellets();
		ResetBoard();
		ResetGhostsAndPlayer();
		gState.pause_time = 2000;
		gState.game_state = GAMESTART;
	}
	AnimateUpdate(ms_elapsed);
	DrawFrame();
}
void SetupMenu()
{
	gState.player->enable_draw = true;
	gState.player->pos = { 6,17.5f };
	gState.player->cur_dir = RIGHT;
	SetPacManMenuFrame();
	SetPulseFrequency(200);
}
void Menu(int ms_elapsed)
{
	PulseUpdate(ms_elapsed);
	DrawMenuFrame();

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
	{
		ResetPellets();
		ResetBoard();
		ResetGhostsAndPlayer();
		gState.game_score = 0;
		gState.player_lives = 3;
		gState.pause_time = 4000;
		gState.game_state = GAMESTART;
	}
}
void GameLoop(int ms_elapsed)
{
	switch (gState.game_state)
	{
	case MAINLOOP:
		MainLoop(ms_elapsed);
		break;
	case GAMESTART:
		GameStart(ms_elapsed);
		break;
	case GAMELOSE:
		GameLose(ms_elapsed);
		break;
	case GAMEOVER:
		gState.pause_time -= ms_elapsed;
		if (gState.pause_time < 0) {
			SetupMenu();
			gState.game_state = MENU;
		}
		DrawFrame();
		break;
	case GAMEWIN:
		GameWin(ms_elapsed);
		break;
	case MENU:
		Menu(ms_elapsed);
		break;
	}
}

int main()
{
	srand(time(NULL));
	sf::RenderWindow window(sf::VideoMode(28.f * TSIZE * 2, 36.f * TSIZE * 2), "PACMAN");
	window.setFramerateLimit(60);
	window.setView(sf::View({ 0, 0, 28.f * TSIZE, 36.f * TSIZE }));
	gState.window = &window;
	
	OnStart();

	sf::Clock clock;
	sf::Time elapsed;

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
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
		GameLoop(elapsed.asMilliseconds());
	}

	OnQuit();

	return 0;
}