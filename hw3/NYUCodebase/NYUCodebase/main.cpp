#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

#define MAX_BULLETS 5
#define FIXED_TIMESTEP 0.0166666f
#define LEFTWALL_POS -1.777f
#define RIGHTWALL_POS 1.777f

SDL_Window* displayWindow;
class Entity;
GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}

class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}
	void Draw(ShaderProgram &program);
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
	glm::vec3 textureSize= glm::vec3(0.5f*size*(width/height), 0.5*size, 1.0f);
};

// GAMESTATE INITIALIZATION //
struct GameState {
	std::vector<Entity> entities;
	std::vector<Entity> bullets;
	int bulletNum = 0;
};

// GLOBAL VARIABLES //
enum GameMode { MAIN_MENU, GAME_LEVEL };
GameMode gamemode = MAIN_MENU;
GameState spaceInv;
ShaderProgram program;
ShaderProgram program2;
GLuint textSheet;
int enemyCount = 12;
const Uint8 *keys = SDL_GetKeyboardState(NULL);


enum Type {PLAYER, ENEMY, BULLET};
class Entity {
public:
	void Update(float elapsed) {
		if (type == ENEMY && isAlive) {
			// BULLET COLLISION //
			for (int i = 0;i < MAX_BULLETS;i++) {
				if (collision(spaceInv.bullets[i])) {
					isAlive = false;
					spaceInv.bullets[i].position.x = 200.0f;
					enemyCount--;
					if (enemyCount = 0) {
						gamemode = MAIN_MENU;
					}
					break;
				}
			}
			// ENEMY COLLISION DETECTION OF WALL //
			if (leftCollision && vtimer > 0.0f) {
				vtimer -= elapsed;
				position.y -= velocity.y*elapsed;
				if (vtimer <= 0.0f) {
					for (Entity& entity : spaceInv.entities) {
						if (entity.type == ENEMY) {
							entity.leftCollision = false;
							entity.vtimer = 0.6f;
						}
					}
				}
			}
			else if (rightCollision && vtimer > 0.0f) {
				vtimer -= elapsed;
				position.y -= velocity.y*elapsed;
				if (vtimer <= 0.0f) {
					for (Entity& entity : spaceInv.entities) {
						if (entity.type == ENEMY) {
							entity.rightCollision = false;
							entity.vtimer = 0.6f;
						}
					}
				}
			}
			else {
				position.x += velocity.x*elapsed;
			}
			if (position.x + size.x >= RIGHTWALL_POS && isAlive) {

				for (Entity& entity : spaceInv.entities) {
					if (entity.type == ENEMY) {
						entity.rightCollision = true;
						entity.velocity.x = -entity.velocity.x;

					}
				}
			}
			if (position.x - size.x <= LEFTWALL_POS && isAlive) {
				for (Entity& entity : spaceInv.entities) {
					if (entity.type == ENEMY) {
						entity.leftCollision = true;
						entity.velocity.x = -entity.velocity.x;
					}
				}
			}
		}
		// PLAYER CONTROL //
		else if (type == PLAYER) {
			if (keys[SDL_SCANCODE_D]) {
				if (position.x + (size.x) <= RIGHTWALL_POS) {
					position.x += velocity.x*elapsed;
				}
			}
			if (keys[SDL_SCANCODE_A]) {
				if (position.x - (size.x) >= LEFTWALL_POS) {
					position.x -= velocity.x*elapsed;
				}
			}
			if (keys[SDL_SCANCODE_SPACE]) {
					shootBullet();
			}

			// BULLET MOVEMENT //
		}
		else if (type == BULLET) {
			position.y += elapsed * velocity.y;
		}
	}

	// COLLISION DETECTION //
	bool collision(Entity& entity) {
		float collidey;
		float collidex;
		collidey = abs(position.y - entity.position.y) - ((size.y + entity.size.y));
		collidex= abs(position.x - entity.position.x) - ((size.x + entity.size.x));
		return (collidey <= 0.0f && collidex <= 0.0f);
	}

	// RENDERING //
	void render(ShaderProgram &program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		program.SetModelMatrix(modelMatrix);
		sprite.Draw(program);
	}

	// FUNCTION TO SHOOT BULLETS //
	void shootBullet() {
		spaceInv.bullets[spaceInv.bulletNum].position.x = position.x;
		spaceInv.bullets[spaceInv.bulletNum].position.y = position.y + size.y + spaceInv.bullets[spaceInv.bulletNum].size.y;
		spaceInv.bulletNum++;
		if (spaceInv.bulletNum > MAX_BULLETS - 1) {
			spaceInv.bulletNum = 0;
		}
	}

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;

	float vtimer = 0.6f;
	bool isAlive = true;
	bool leftCollision = false;
	bool rightCollision = false;
	
	SheetSprite sprite;
	Type type;
	
};

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x + character_size, texture_y + character_size,
			texture_x + character_size, texture_y,
			texture_x, texture_y + character_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	// draw this data (use the .data() method of std::vector to get pointer to data)
	// draw this yourself, use text.size() * 6 or vertexData.size()/2 to get number of vertices



	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());



}


void updateGame(GameState& spaceInv, float elapsed) {
	for (Entity& entity : spaceInv.entities) {
		entity.Update(FIXED_TIMESTEP);
	}
	for (int i = 0;i < MAX_BULLETS;i++) {
		spaceInv.bullets[i].Update(FIXED_TIMESTEP);
	}

}

void Update(float elapsed) {
	switch (gamemode) {
	case GAME_LEVEL:
		updateGame(spaceInv, elapsed);
		break;
	case MAIN_MENU:
		break;
	}
}

// GAME RENDER //
void renderGame(GameState& spaceInv) {
	glClear(GL_COLOR_BUFFER_BIT);

	for (Entity& entity : spaceInv.entities) {
		if (entity.isAlive == true) {
			entity.render(program);
		}
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		spaceInv.bullets[i].render(program);
	}
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.65f, -0.85f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f, 0.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);
}

// MAIN MENU RENDER //
void renderMenu() {
	glClear(GL_COLOR_BUFFER_BIT);
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program2.positionAttribute);

	program2.SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 0.25f, 1.0f));
	program2.SetModelMatrix(modelMatrix);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.4f, 0.0f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	DrawText(program, textSheet, "Click Anywhere to Start", 0.25f, 0.0005f);
}

// RENDER ALL // 
void Render() {
	switch (gamemode) {
	case GAME_LEVEL:
		renderGame(spaceInv);
		break;
	case MAIN_MENU:
		renderMenu();
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, 640, 360);
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	program2.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
	textSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);


	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	float x = -0.5f;
	float y = 0.6f;
	for (int i = 0; i <= 12; i++) {
		if (x > 0.5f) {
			x = -0.5f;
			y = 0.4f;
		}
		Entity myEntity;
		myEntity.sprite = SheetSprite(spriteSheet, 425.0f / 1024.0f, 468.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
		myEntity.type = ENEMY;
		myEntity.velocity = glm::vec3(0.3f, 0.2f, 1.0f);
		myEntity.size = myEntity.sprite.textureSize;
		myEntity.position = glm::vec3(x, y, 1.0f);
		spaceInv.entities.push_back(myEntity);
		x += 0.2f;
		
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		Entity bullet;
		spaceInv.bullets.push_back(bullet);
		spaceInv.bullets[i].sprite = SheetSprite(spriteSheet, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.20f);
		spaceInv.bullets[i].velocity = glm::vec3(0.0f, 1.0f, 1.0f);
		spaceInv.bullets[i].size = spaceInv.bullets[i].sprite.textureSize;
		spaceInv.bullets[i].position = glm::vec3(100.0f, 0.0f, 1.0f);
		spaceInv.bullets[i].type = BULLET;
	}
	Entity player;
	player.sprite = SheetSprite(spriteSheet, 736.0f / 1024.0f, 862 / 1024.0f, 40.0f / 1024.0f, 40.0f / 1024.0f, 0.2f);
	player.type = PLAYER;
	player.velocity = glm::vec3(0.7f, 0.0f, 1.0f);
	player.size = player.sprite.textureSize;
	player.position = glm::vec3(0.0f, -0.7f, 1.0f);
	spaceInv.entities.push_back(player);

	float accumulator = 0.0f;
	float lastFrameTicks = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN && gamemode == MAIN_MENU) {
				gamemode = GAME_LEVEL;
			}
		}

		// GAME TICKS //
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}

		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		Render();

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}

// DRAW FUNCTION //
void SheetSprite::Draw(ShaderProgram &program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size, aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f *size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, -0.5f * size };

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

	



