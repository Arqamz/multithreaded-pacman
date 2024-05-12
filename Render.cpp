#include "SFML/Graphics.hpp"

#define TSIZE 16
#define YOFFSET 0

struct RenderItems {
    sf::VertexArray pellet_va;
    sf::VertexArray wall_va;
    sf::Sprite wall_map;
};

struct Textures {
    sf::Texture pellets;
    sf::Texture wall_map_t;
};

static RenderItems RItems;
static Textures RTextures;

void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h, sf::Color color, sf::FloatRect t_rect);
void InitWalls();
void InitTextures();
void InitPellets();

void MakeQuad(sf::VertexArray& va, float x, float y, int w, int h, sf::Color color, sf::FloatRect t_rect) {
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

void InitWalls() {
    RItems.wall_va.setPrimitiveType(sf::Quads);
    for (int y = 0; y < gState.board.size(); y++) {
        for (int x = 0; x < gState.board.at(y).size(); x++) {
            if(gState.board.at(y).at(x)== '|')
                MakeQuad(RItems.wall_va, x * TSIZE, y * TSIZE + YOFFSET, TSIZE, TSIZE, { 150,150,150 });
        }
    }
}

void InitTextures() {
    RTextures.pellets.loadFromFile("textures/dots.png");
    RTextures.wall_map_t.loadFromFile("textures/map.png");
}

void InitPellets() {
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