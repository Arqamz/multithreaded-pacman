#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <random>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <string>
#include <cmath>
#include <vector>
#include <iostream>
#include <map>

#include <pthread.h>
#include <semaphore.h>

// PACMAN.h
enum Dir
{
	NONE,
	UP,
	DOWN,
	LEFT,
	RIGHT,
};
enum GhostType
{
	RED,
	PINK,
	BLUE,
	ORANGE,
};
enum TargetState
{
	CHASE,
	CORNER,
	FRIGHTENED,
	GOHOME,		// inital state to pathfind to the square above the door
	HOMEBASE,	// indefinite state going up and down in home
	LEAVEHOME,
	ENTERHOME,
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

// seconds
const int fright_time = 6;

// scatter, chase repeating
const int wave_times[8] = { 7,20,7,20,5,20,5,-1 };

//
// Global states
//
struct Ghost
{
	GhostType type;
	sf::Vector2f pos;
	Dir cur_dir;
	TargetState target_state;
	float move_speed = ghost_speed;
	bool update_dir = false;
	bool in_house = false;
	bool enable_draw = true;

	int dot_counter = 0;
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

	int pellets_left = pellet_amt;
	int current_level = 1;
	int player_lives = 0;
	int game_score = 0;
	int high_score;
	int wave_counter = 0;

	bool pellet_eaten = false;
	bool powerPelletMode = false;

	bool first_life = true;
	bool using_global_counter = false;
	int global_dot_counter = 0;
	
	bool player_eat_ghost = false;
	int ghosts_eaten_in_powerup = 0;
	Ghost* recent_eaten = nullptr;

	std::vector<Ghost*> ghosts;
	Player* player;

	std::vector<std::string> board;

    Dir inputDir;
    bool sem_b_input;

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

inline TargetState GetGlobalTarget()
{
	if (gState.wave_counter >= 7)
		return CHASE;

	return (gState.wave_counter % 2) ? CHASE : CORNER;
}
inline bool GhostRetreating()
{
	for (int i = 0; i < 4; i++) {
		if (gState.ghosts[i]->target_state == GOHOME)
			return true;
	}

	return false;
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

//Ghosts.h


bool InMiddleTile(sf::Vector2f pos, sf::Vector2f prev, Dir dir);
std::vector<Dir> GetAvailableSquares(sf::Vector2f pos, Dir dir, bool home_tiles);
float Distance(int x, int y, int x1, int y1);
Dir GetShortestDir(const std::vector<Dir>& squares, const Ghost& ghost, sf::Vector2f target);
Dir GetOppositeTile(Ghost& ghost);

sf::Vector2f BlinkyUpdate(Ghost& ghost);
sf::Vector2f PinkyUpdate(Ghost& ghost);
sf::Vector2f InkyUpdate(Ghost& ghost);
sf::Vector2f ClydeUpdate(Ghost& ghost);

bool PassedEntrence(Ghost& ghost);
void HouseUpdate(Ghost& ghost);

void UpdateDirection(std::vector<Dir> squares, Ghost& ghost);
void UpdateGhosts(int ghostNum);

void SetAllGhostState(TargetState new_state);
void SetGhostState(Ghost& ghost, TargetState new_state);

const float starting_x[4] = { 14,14,12,16 };
const Dir enter_dir[4] = { UP, UP, LEFT, RIGHT };
const sf::Vector2f corners[4] = { {31,0},{0,0},{31,31}, {0,31} };
const int dot_counters[4] = { 0,0,30,60 };

const int global_dot_limit[4] = { 0,7,17,32 };


// Ghosts.cpp


bool InMiddleTile(sf::Vector2f pos, sf::Vector2f prev, Dir dir)
{
	if ((int)pos.x != (int)prev.x || (int)pos.y != (int)prev.y)
		return false;
	float x = pos.x - (int)pos.x;
	float y = pos.y - (int)pos.y;

	float px = prev.x - (int)prev.x;
	float py = prev.y - (int)prev.y;

	switch (dir)
	{
	case UP:
	case DOWN:
		return std::min(y, py) <= 0.5 && std::max(y, py) >= 0.5;
		break;

	case LEFT:
	case RIGHT:
		return std::min(x, px) <= 0.5 && std::max(x, px) >= 0.5;
		break;
	}
	return false;
}
// this should be cached/hardcoded, but its performant enough
std::vector<Dir> GetAvailableSquares(sf::Vector2f pos, Dir dir, bool home_tile)
{
	std::vector<Dir> squares;
	// this is the reverse order of precedence that
	// the original pacman game used
	// up is picked before left if the distances are tied etc.
	if (!TileCollision(pos + sf::Vector2f{ 0.9, 0 },home_tile ))
		squares.push_back(RIGHT);
	if (!TileCollision(pos + sf::Vector2f{ 0, 0.9 }, home_tile))
		squares.push_back(DOWN);
	if (!TileCollision(pos + sf::Vector2f{ -0.9, 0 }, home_tile))
		squares.push_back(LEFT);
	if (!TileCollision(pos + sf::Vector2f{ 0, -0.9 }, home_tile))
		squares.push_back(UP);
	for (int i = 0; i < squares.size(); i++) {
		if (squares[i] == opposite_dir[dir])
			squares.erase(squares.begin() + i);
	}
	return squares;
}
// returns squared distance
float Distance(int x, int y, int x1, int y1)
{
	return ((x - x1) * (x - x1)) + ((y - y1) * (y - y1));
}

// the original game didnt use a pathfinding algorithm
// simple distance comparisons are fine enough
Dir GetShortestDir(const std::vector<Dir>& squares, const Ghost& ghost, sf::Vector2f target)
{
	int min_dist = 20000000;
	Dir min_dir = NONE;
	if (squares.size() == 0)
		printf("EMPTY\n");

	for (auto dir : squares) {
		sf::Vector2f square = ghost.pos + dir_addition[dir];
		float dir_dist = Distance(target.x, target.y, square.x, square.y);
		if (dir_dist <= min_dist) {
			min_dir = dir;
			min_dist = dir_dist;
		}
	}

	return min_dir;
}
sf::Vector2f BlinkyUpdate(Ghost& ghost)
{
	return gState.player->pos;
}
sf::Vector2f PinkyUpdate(Ghost& ghost)
{
	return gState.player->pos + dir_addition[gState.player->cur_dir] * 4;
}
sf::Vector2f InkyUpdate(Ghost& ghost)
{
	sf::Vector2f target = gState.player->pos + dir_addition[gState.player->cur_dir] * 2;
	sf::Vector2f offset = gState.ghosts[RED]->pos - target;
	target = target + offset * -1;
	return target;
}
sf::Vector2f ClydeUpdate(Ghost& ghost)
{
	sf::Vector2f target = gState.player->pos;

	// if clyde is 8 tiles away, target player, else target corner
	if (Distance(ghost.pos.x, ghost.pos.y, gState.player->pos.x, gState.player->pos.y) < 64)
		target = { 0,31 };
	return target;
}
bool PassedEntrence(Ghost& ghost)
{
	float prev_x = ghost.pos.x - dir_addition[ghost.cur_dir].x * ghost.move_speed;

	return FComp(ghost.pos.y, 11.5)
		&& std::min(ghost.pos.x, prev_x) <= 14.1
		&& std::max(ghost.pos.x, prev_x) >= 13.9;
}
void UpdateDirection(std::vector<Dir> squares, Ghost& ghost)
{
	sf::Vector2f target;
	bool update_dir = false;
	if (squares.size() == 1) {
		ghost.cur_dir = squares.at(0);
	}

	switch (ghost.target_state)
	{
	case FRIGHTENED:
		ghost.cur_dir = squares.at(rand() % (squares.size()));
		break;
	case GOHOME:
		target = { 14,11.5 };
		update_dir = true;
		break;
	case CHASE:
		switch (ghost.type)
		{
		case RED:
			target = BlinkyUpdate(ghost);
			break;
		case PINK:
			target = PinkyUpdate(ghost);
			break;
		case BLUE:
			target = InkyUpdate(ghost);
			break;
		case ORANGE:
			target = ClydeUpdate(ghost);
			break;
		}
		update_dir = true;
		break;
	case CORNER:
		target = corners[ghost.type];
		update_dir = true;
		break;
	}

	if (update_dir)
		ghost.cur_dir = GetShortestDir(squares, ghost, target);

	CenterObject(ghost.cur_dir, ghost.pos);
}
// held together with duct tape
// my waypoint system works on the assumption that movement is only
// made in the center of tiles, in the house, movement is made off center,
// which breaks it
void HouseUpdate(Ghost& ghost)
{
	switch (ghost.target_state)
	{
	case ENTERHOME:
		if (ghost.pos.y >= 15) {
			ghost.cur_dir = enter_dir[ghost.type];
			ghost.pos.y = 15;
			if (FComp(ghost.pos.x, starting_x[ghost.type])
				|| (ghost.cur_dir == LEFT && ghost.pos.x <= starting_x[ghost.type])
				|| (ghost.cur_dir == RIGHT && ghost.pos.x >= starting_x[ghost.type])) {
				ghost.target_state = HOMEBASE;
				ghost.move_speed = inhome_speed;
			}
		}
		break;
	case LEAVEHOME:
		if (FComp(ghost.pos.x, 14) || (ghost.cur_dir == LEFT && ghost.pos.x <= 14) || (ghost.cur_dir == RIGHT && ghost.pos.x >= 14)) {
			ghost.cur_dir = UP;
			ghost.pos.x = 14;
			if (ghost.pos.y <= 11.5) {
				ghost.move_speed = ghost_speed;
				ghost.target_state = GetGlobalTarget();
				ghost.pos.y = 11.5;
				ghost.in_house = false;
				ghost.cur_dir = LEFT;
			}
		}
		else {
			ghost.cur_dir = opposite_dir[enter_dir[ghost.type]];
		}
		break;
	case HOMEBASE:
		ghost.move_speed = inhome_speed;

		// check timer if its time to leave
		if (gState.using_global_counter) {
			if (gState.global_dot_counter >= global_dot_limit[ghost.type])
				ghost.target_state = LEAVEHOME;
		}
		else if (ghost.dot_counter >= dot_counters[ghost.type])
			ghost.target_state = LEAVEHOME;

		if (ghost.pos.y <= 14) {
			ghost.pos.y = 14;
			ghost.cur_dir = DOWN;
		}
		else if (ghost.pos.y >= 15) {
			ghost.pos.y = 15;
			ghost.cur_dir = UP;
		}
		break;
	}
}
void UpdateGhosts(int ghostNum)
{
	//printf("Ghost %d is being updated\n", ghostNum);
	Ghost* ghost = gState.ghosts[ghostNum];
	sf::Vector2f prev_pos = ghost->pos;

	ghost->pos += dir_addition[ghost->cur_dir] * ghost->move_speed;
	if (ghost->in_house) {
		HouseUpdate(*ghost);
	}
	// if ghost pos is in the middle of the tile, didnt update last turn, and isnt in the tunnel, 
	// do waypoint calculations, else do nothing
	else if (!ghost->update_dir &&
		InMiddleTile(ghost->pos, prev_pos, ghost->cur_dir) &&
		!InTunnel(ghost->pos)) {
		UpdateDirection(GetAvailableSquares(ghost->pos, ghost->cur_dir, false), *ghost);
		ghost->update_dir = true;
	}
	else {
		ghost->update_dir = false;
	}

	// blech, at least it terminates quickly
	if (ghost->target_state == GOHOME &&
		PassedEntrence(*ghost)) {
		ghost->target_state = ENTERHOME;
		ghost->in_house = true;
		ghost->cur_dir = DOWN;
		//ghost->move_speed = 0.02;
		ghost->pos.x = 14;
	}

	// tunneling
	if (ghost->pos.x < -1) {
		ghost->pos.x += 29;
	}
	else if (ghost->pos.x >= 29) {
		ghost->pos.x -= 29;
	}
}
Dir GetOppositeTile(Ghost& ghost)
{
	// this will be the case 99% of times
	if (!TileCollision(ghost.pos + dir_addition[opposite_dir[ghost.cur_dir]] * 0.9))
		return opposite_dir[ghost.cur_dir];


	auto squares = GetAvailableSquares(ghost.pos, ghost.cur_dir, false);
	if (!squares.empty())
		return squares.at(0);

	// last case scenario, dont reverse
	return ghost.cur_dir;

}
void SetAllGhostState(TargetState new_state)
{
	for (int i = 0; i < 4; i++) {
		SetGhostState(*gState.ghosts[i], new_state);
	}
}

void SetGhostState(Ghost& ghost, TargetState new_state)
{
	if (ghost.target_state == GOHOME)
		return;
	switch (new_state)
	{
	case LEAVEHOME:
		if (ghost.in_house)
			ghost.target_state = new_state;
	case ENTERHOME:
	case HOMEBASE:
		break;
	case FRIGHTENED:
		if (!ghost.in_house) {
			ghost.cur_dir = GetOppositeTile(ghost);
			ghost.move_speed = ghost_fright;
			ghost.target_state = new_state;
		}
		break;
	case GOHOME:
		if (!ghost.in_house) {
			ghost.move_speed = gohome_speed;
			ghost.target_state = new_state;
		}
		break;
	default:
		if (!ghost.in_house) {
			// ghost reverses direction whenever changing states,
			// except when coming from frightened
			if (ghost.target_state != FRIGHTENED)
				ghost.cur_dir = GetOppositeTile(ghost);
			ghost.move_speed = ghost_speed;
			ghost.target_state = new_state;
		}
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
    return pdir;
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

    try_dir = gState.inputDir;

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

	int pulse = true;
	int pulse_timer = 0;

	int pulse_limit = 200;
};

void AnimateUpdate(int ms_elapsed);
void StartPacManDeath();
void ResetAnimation();
void SetPacManMenuFrame();

sf::IntRect GetGhostFrame(GhostType type, TargetState state, Dir dir);
sf::IntRect GetPacManFrame(Dir dir);

void PulseUpdate(int ms_elapsed);
bool IsPulse();
void SetPulseFrequency(int ms);

// ANIMATE.cpp

static Animation animate;

sf::IntRect GetGhostFrame(GhostType type, TargetState state, Dir dir)
{
	sf::IntRect ghost = { 0,128,32,32 };

	int offset = 0;
	if (state != FRIGHTENED) {
		switch (dir)
		{
		case UP:
			offset = 128;
			break;
		case DOWN:
			offset = 128 + 64;
			break;
		case LEFT:
			offset = 64;
			break;
		}
	}
	
	if (state == FRIGHTENED) {
		ghost.left = 256;
		if (animate.fright_flash)
			ghost.left += 64;
	}
	else if (state == GOHOME || state == ENTERHOME) {
		ghost.left = 256 + (offset/2);
		ghost.top = 128 + 32;
	}
	else {
		ghost.top += 32 * type;
		ghost.left = offset;
	}
	if(state != GOHOME && state != ENTERHOME)
		ghost.left += animate.ghost_frame_2 * 32;

	return ghost;
}
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

	for (int i = 0; i < 4; i++)
	{
		RItems.ghosts[i].setTexture(RTextures.sprites);
		RItems.ghosts[i].setScale({ 0.5,0.5 });
		RItems.ghosts[i].setOrigin({ 16,16 });
	}

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
	
	if (gState.game_state == GAMESTART){
        gState.sem_b_input = false;
		MakeText("Ready!", 11, 20, { 255,255,00 });
    }
	else if (gState.game_state == GAMEOVER){
        gState.sem_b_input = false;
		MakeText("GAME  OVER", 9, 20, { 255,0,00 });
    }

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
	
	for (int i = 0; i < 4; i++) {
		if (!gState.ghosts[i]->enable_draw)
			continue;
		RItems.ghosts[i].setPosition(gState.ghosts[i]->pos.x * TSIZE, gState.ghosts[i]->pos.y * TSIZE + YOFFSET);
		RItems.ghosts[i].setTextureRect(GetGhostFrame(gState.ghosts[i]->type, gState.ghosts[i]->target_state, gState.ghosts[i]->cur_dir));
		gState.window->draw(RItems.ghosts[i]);
	}

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

	for (int i = 0; i < 4; i++) {
		RItems.ghosts[i].setPosition(gState.ghosts[i]->pos.x * TSIZE, gState.ghosts[i]->pos.y * TSIZE + YOFFSET);
		RItems.ghosts[i].setTextureRect(GetGhostFrame(gState.ghosts[i]->type, gState.ghosts[i]->target_state, gState.ghosts[i]->cur_dir));
		gState.window->draw(RItems.ghosts[i]);
	}

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

////////// THREADINGGG PARTTT

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t ghost_mutexes[4];

// Semaphore to signal thread termination
sem_t semaphore;

// Move Ghost semaphore
sem_t ghostSemaphores[4];
// Semaphore to terminate ghosts
sem_t ghostTerminateSemaphore[4];

// sem_t ghostSemaphores[4];

void* PlayerMovementThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);

        if(gState.sem_b_input){
            PlayerMovement();  
        }

        pthread_mutex_unlock(&mutex);

        sf::sleep(sf::milliseconds(10));

        int semValue;
        sem_getvalue(&semaphore, &semValue);
        if (semValue == 1) {
            break;
        }
    }
    return nullptr;
}

void* GhostMovementThread(void* arg) {
    int ghostNum = *static_cast<int*>(arg);
    delete static_cast<int*>(arg);
    
    printf("Updating ghost %d in thread\n", ghostNum);

    // Wait for the semaphore to be signaled before starting
	sem_wait(&ghostSemaphores[ghostNum]);

    while (true) {
        pthread_mutex_lock(&ghost_mutexes[ghostNum]);
        
        UpdateGhosts(ghostNum);

        pthread_mutex_unlock(&ghost_mutexes[ghostNum]);

        sf::sleep(sf::milliseconds(20));

        // Check if the terminate semaphore has been signaled
        int semValue;
        sem_getvalue(&ghostTerminateSemaphore[ghostNum], &semValue);
        if (semValue == 1) {
            // If semaphore was signaled, exit the loop and terminate the thread
            printf("Thread %d terminating\n", ghostNum);
            pthread_exit(nullptr);
        }
    }

    return nullptr;
}

void InitGhostSemaphores() {
    // Reinitialize ghostSemaphores with initial value 0
    for (int i = 0; i < 4; ++i) {
        sem_init(&ghostSemaphores[i], 0, 0);
    }
    // Reinitialize ghostTerminateSemaphore with initial value 0
    for (int i = 0; i < 4; ++i) {
        sem_init(&ghostTerminateSemaphore[i], 0, 0);
    }
}

void InitGhostThreads() {
    printf("We got here\n");
    for (int i = 0; i < 4; i++) {
        sem_init(&ghostSemaphores[i], 0, 0);
        pthread_mutex_init(&ghost_mutexes[i], nullptr);
        pthread_t ghostThread;
        
        int* ghostNum = new int(i);
        
        int threadErr = pthread_create(&ghostThread, nullptr, GhostMovementThread, static_cast<void*>(ghostNum));
        if (threadErr!= 0) {
            std::cerr << "Failed to create ghost movement thread: " << std::endl;
            return;
        }
        printf("Ghost thread %d created\n", i);
    }
}

// Function for threading CheckPelletCollision
void* CheckPelletCollisionThread(void* arg) {
    while (true) {
        // Lock mutex before accessing shared state
        pthread_mutex_lock(&mutex);
        
        CheckPelletCollision();
        
        // Unlock mutex after accessing shared state
        pthread_mutex_unlock(&mutex);
        
        // Sleep for a short duration to prevent busy-waiting
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for threading CheckGhostCollision
void* CheckGhostCollisionThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        
        CheckGhostCollision();
        
        pthread_mutex_unlock(&mutex);
        
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for threading UpdateWave
void* UpdateWaveThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        
        UpdateWave(*static_cast<int*>(arg));
        
        pthread_mutex_unlock(&mutex);
        
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for threading UpdateEnergizerTime
void* UpdateEnergizerTimeThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        
        UpdateEnergizerTime(*static_cast<int*>(arg));
        
        pthread_mutex_unlock(&mutex);
        
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for threading CheckHighScore
void* CheckHighScoreThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        
        CheckHighScore();
        
        pthread_mutex_unlock(&mutex);
        
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for threading CheckWin
void* CheckWinThread(void* arg) {
    while (true) {
        pthread_mutex_lock(&mutex);
        
        CheckWin();
        
        pthread_mutex_unlock(&mutex);
        
        sf::sleep(sf::milliseconds(10));
    }
    return nullptr;
}

// Function for initializing and starting the Pellet Collision thread
void InitPelletCollisionThread() {
    pthread_t pelletCollisionThread;
    int threadErr = pthread_create(&pelletCollisionThread, nullptr, CheckPelletCollisionThread, nullptr);
    if (threadErr != 0) {
        std::cerr << "Failed to create CheckPelletCollision thread: " << std::endl;
        return;
    }
}

// Function for initializing and starting the Ghost Collision thread
void InitGhostCollisionThread() {
    pthread_t ghostCollisionThread;
    int threadErr = pthread_create(&ghostCollisionThread, nullptr, CheckGhostCollisionThread, nullptr);
    if (threadErr != 0) {
        std::cerr << "Failed to create CheckGhostCollision thread: " << std::endl;
        return;
    }
}

// Function for initializing and starting the Update Wave thread
void InitUpdateWaveThread(int ms_elapsed) {
    pthread_t updateWaveThread;
    int threadErr = pthread_create(&updateWaveThread, nullptr, UpdateWaveThread, static_cast<void*>(&ms_elapsed));
    if (threadErr != 0) {
        std::cerr << "Failed to create UpdateWave thread: " << std::endl;
        return;
    }
}

// Function for initializing and starting the Update Energizer Time thread
void InitUpdateEnergizerTimeThread(int ms_elapsed) {
    pthread_t updateEnergizerTimeThread;
    int threadErr = pthread_create(&updateEnergizerTimeThread, nullptr, UpdateEnergizerTimeThread, static_cast<void*>(&ms_elapsed));
    if (threadErr != 0) {
        std::cerr << "Failed to create UpdateEnergizerTime thread: " << std::endl;
        return;
    }
}

// Function for initializing and starting the Check High Score thread
void InitCheckHighScoreThread() {
    pthread_t checkHighScoreThread;
    int threadErr = pthread_create(&checkHighScoreThread, nullptr, CheckHighScoreThread, nullptr);
    if (threadErr != 0) {
        std::cerr << "Failed to create CheckHighScore thread: " << std::endl;
        return;
    }
}

// Function for initializing and starting the Check Win thread
void InitCheckWinThread() {
    pthread_t checkWinThread;
    int threadErr = pthread_create(&checkWinThread, nullptr, CheckWinThread, nullptr);
    if (threadErr != 0) {
        std::cerr << "Failed to create CheckWin thread: " << std::endl;
        return;
    }
}


///////////////////////////////

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
void CheckGhostCollision();
void UpdateWave(int ms_elapsed);
void UpdateEnergizerTime(int ms_elasped);
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

// GLOBAL SHARED RESOURCE
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

	// Ghosts init

	Ghost* temp = new Ghost();
	temp->type = RED;
	gState.ghosts.push_back(temp);

	temp = new Ghost();
	temp->type = PINK;
	gState.ghosts.push_back(temp);

	temp = new Ghost();
	temp->type = BLUE;
	gState.ghosts.push_back(temp);

	temp = new Ghost();
	temp->type = ORANGE;
	gState.ghosts.push_back(temp);

	InitBoard();
	InitRender();
	ResetGhostsAndPlayer();
		
	SetupMenu();
	gState.game_state = MENU;
	gState.pause_time = 2000;

    gState.sem_b_input = false;
}
void ResetGhostsAndPlayer()
{
	Ghost* temp = gState.ghosts[0];
	temp->pos = { 14, 11.5 };
	temp->cur_dir = LEFT;
	temp->target_state = CORNER;
	temp->in_house = false;
	temp->move_speed = ghost_speed;
	//temp->dot_counter = 0;
	temp->enable_draw = true;

	temp = gState.ghosts[1];
	temp->pos = { 14, 14.5 };
	temp->cur_dir = UP;
	temp->target_state = HOMEBASE;
	temp->in_house = true;
	temp->move_speed = inhome_speed;
	//temp->dot_counter = 0;
	temp->enable_draw = true;

	temp = gState.ghosts[2];
	temp->pos = { 12, 14.5 };
	temp->cur_dir = DOWN;
	temp->target_state = HOMEBASE;
	temp->in_house = true;
	temp->move_speed = inhome_speed;
	//temp->dot_counter = 0;
	temp->enable_draw = true;

	temp = gState.ghosts[3];
	temp->pos = { 16, 14.5 };
	temp->cur_dir = DOWN;
	temp->target_state = HOMEBASE;
	temp->in_house = true;
	temp->move_speed = inhome_speed;
	//temp->dot_counter = 0;
	temp->enable_draw = true;

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
	InitGhostSemaphores();
	InitGhostThreads();
}
void ResetBoard()
{
	gState.board.clear();
	InitBoard();

	gState.pellets_left = 244;

	gState.first_life = true;
	gState.using_global_counter = false;
	for (int i = 0; i < 4; i++)
		gState.ghosts[i]->dot_counter = 0;
}

void IncrementGhostHouse()
{
	Ghost* first_ghost = nullptr;
	// using global counter, increment it
	if (gState.using_global_counter) {
		gState.global_dot_counter++;
	}
	for (int i = 0; i < 4; i++) {
		if (gState.ghosts[i]->target_state == HOMEBASE) {
			first_ghost = gState.ghosts[i];
			break;
		}
	}
	if (first_ghost == nullptr) {
		// no more ghosts in house, switch back to local counters
		if (gState.using_global_counter) {
			gState.using_global_counter = false;
		}
	}
	// if not using global and ghost is in house, use local counter
	else if(!gState.using_global_counter) {
		first_ghost->dot_counter++;
	}
}
void CheckPelletCollision()
{
	char tile = GetTile(gState.player->pos.x, gState.player->pos.y);
	bool collided = false;
	if (tile == '.') {
		collided = true;
		// play sound
		gState.game_score += 10;
	}
	else if (tile == 'o' && !gState.powerPelletMode) {
		collided = true;
		gState.powerPelletMode = true;
		gState.game_score += 50;
		gState.energizer_time = fright_time*1000;

		gState.ghosts_eaten_in_powerup = 0;

		SetAllGhostState(FRIGHTENED);
	}

	if (collided) {
		RemovePellet(gState.player->pos.x, gState.player->pos.y);
		SetTile(gState.player->pos.x, gState.player->pos.y, ' ');
		IncrementGhostHouse();
		gState.pellet_eaten = true;
		gState.pellets_left--;
	}
}
void CheckGhostCollision()
{
	int px = (int)gState.player->pos.x;
	int py = (int)gState.player->pos.y;

	for (int i = 0; i < 4; i++) {
		if ((int)gState.ghosts[i]->pos.x == px && (int)gState.ghosts[i]->pos.y == py) {
			if (gState.ghosts[i]->target_state == FRIGHTENED) {
				SetGhostState(*gState.ghosts[i], GOHOME);
				gState.recent_eaten = gState.ghosts[i];
				gState.ghosts_eaten_in_powerup++;
				gState.game_score += (pow(2,gState.ghosts_eaten_in_powerup) * 100);
				
				gState.player_eat_ghost = true;
				gState.pause_time = 500;

				gState.ghosts[i]->enable_draw = false;
				gState.player->enable_draw = false;

			}
			else if (gState.ghosts[i]->target_state != GOHOME) {
				gState.game_state = GAMELOSE;
				gState.pause_time = 2000;
				gState.player_lives -= 1;
                gState.sem_b_input = false;
				gState.first_life = false;
				StartPacManDeath();
				printf("RESET\n");
				for(int i=0;i<4;i++)
					sem_post(&ghostTerminateSemaphore[i]);
				printf("Destroyed semaphores\n");
			}
		}
	}
}
void UpdateWave(int ms_elapsed)
{
	// indefinte chase mode
	if (gState.wave_counter >= 7)
		return;

	gState.wave_time += ms_elapsed;
	if (gState.wave_time / 1000 >= wave_times[gState.wave_counter]) {
		gState.wave_counter++;
		printf("New wave\n");
		if(gState.energizer_time <= 0){
			SetAllGhostState(GetGlobalTarget());
			gState.powerPelletMode = false;
		}
		gState.wave_time = 0;
	}

}
void UpdateEnergizerTime(int ms_elasped)
{
	if (gState.energizer_time <= 0){
		gState.powerPelletMode = false;
		return;
	}

	gState.energizer_time -= ms_elasped;
	if (gState.energizer_time <= 0) {
		gState.powerPelletMode = false;
		SetAllGhostState(GetGlobalTarget());
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
		for (int i = 0; i < 4; i++) {
			gState.ghosts[i]->enable_draw = false;
		}
        gState.sem_b_input = false;
		gState.player->stopped = true;
		gState.pause_time = 2000;
		
		SetPulseFrequency(200);
		for(int i=0;i<4;i++)
			sem_post(&ghostTerminateSemaphore[i]);
		printf("Destroyed semaphores\n");

	}
}

void MainLoop(int ms_elapsed)
{
	if (gState.player_eat_ghost) {
		gState.pause_time -= ms_elapsed;
		if (gState.pause_time < 0) {
			gState.recent_eaten->enable_draw = true;
			gState.player->enable_draw = true;
			gState.player_eat_ghost = false;
		}
		DrawFrame();

		return;
	}

    if(gState.pellet_eaten)
        gState.pellet_eaten = false;

	// PACMAN MOVEMENT()
	// GHOSTS MOVEMENT()

	CheckGhostCollision();
	CheckPelletCollision();
	
	UpdateWave(ms_elapsed);
	UpdateEnergizerTime(ms_elapsed);
	CheckHighScore();
	CheckWin();
	AnimateUpdate(ms_elapsed);
	DrawFrame();
}

void GameStart(int ms_elasped)
{
    gState.sem_b_input = false;
	
	gState.pause_time -= ms_elasped;
	if (gState.pause_time <= 0) {
		printf("Now start movement\n");
		for(int i=0;i<4;i++){
			sem_post(&ghostSemaphores[i]);
		}
		gState.game_state = MAINLOOP;
		SetPulseFrequency(150);
	}

	DrawFrame();
}
void GameLose(int ms_elapsed)
{
    gState.sem_b_input = false;
	gState.pause_time -= ms_elapsed;
	if (gState.pause_time <= 0) {
		if (gState.player_lives < 0) {
			gState.game_state = GAMEOVER;
			gState.pause_time = 5000;
			for (int i = 0; i < 4; i++)
				gState.ghosts[i]->enable_draw = false;

			for(int i=0;i<4;i++)
				sem_destroy(&ghostSemaphores[i]);
			
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
    gState.sem_b_input = false;
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
    gState.sem_b_input = false;
	for (int i = 0; i < 4; i++) {
		gState.ghosts[i]->enable_draw = true;
		gState.ghosts[i]->pos = { 6,5.5f + i * 3.f };
		gState.ghosts[i]->cur_dir = RIGHT;
		gState.ghosts[i]->target_state = CHASE;
		gState.ghosts[i]->in_house = false;
	}
	gState.player->enable_draw = true;
	gState.player->pos = { 6,17.5f };
	gState.player->cur_dir = RIGHT;
	SetPacManMenuFrame();
	SetPulseFrequency(200);
}
void Menu(int ms_elapsed)
{
    gState.sem_b_input = false;
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
        gState.sem_b_input = true;
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

    // Initialize the semaphore with an initial value of 0
    sem_init(&semaphore, 0, 0);

    // Create a thread for player movement
    pthread_t playerThread;
    int threadErr = pthread_create(&playerThread, nullptr, PlayerMovementThread, nullptr);
    if (threadErr != 0) {
        std::cerr << "Failed to create player movement thread: " << std::endl;
        return 1;
    }
	InitGhostSemaphores();
	InitGhostThreads();

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

        pthread_mutex_lock(&mutex);
        Dir inputDir = NONE;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            inputDir = UP;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            inputDir = DOWN;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            inputDir = RIGHT;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            inputDir = LEFT;
        
        gState.inputDir = inputDir;
        pthread_mutex_unlock(&mutex);

        elapsed = clock.restart();
        GameLoop(elapsed.asMilliseconds());
    }

    // Signal the semaphore to indicate thread termination
    sem_post(&semaphore);

    // Wait for the player thread to terminate
    pthread_join(playerThread, nullptr);

    OnQuit();

    return 0;
}
