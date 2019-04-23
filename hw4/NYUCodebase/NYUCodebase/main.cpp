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
#include "FlareMap.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>

#define FIXED_TIMESTEP 0.0166666f
#define LEVEL_HEIGHT 32
#define LEVEL_WIDTH 128
#define SPRITE_COUNT_X 32
#define SPRITE_COUNT_Y 32
#define MAX_RIGHT -1.777f
#define MAX_LEFT 1.777f
#define TILE_SIZE 1/16.0f

SDL_Window* displayWindow;
class Entity;
GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		//assert(false);
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



// shader program
ShaderProgram program;


// texture sheet
GLuint platforms;
GLuint entitiesText;

// keyboard
const Uint8 *keys = SDL_GetKeyboardState(NULL);

// matrix
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

// map for platformer
FlareMap map;

std::vector<Entity> allEntities;

// entity type
enum Type {PLAYER, ITEM};

// entity class
class Entity {
public:
	void Draw() {
		float u = (float)(((int)entityIndex) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
		float v = (float)(((int)entityIndex) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
		float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
		float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
		float vertexData[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
		 -0.5f, 0.5f, -0.5f };
		float texCoordData[] = {
		   u, v + spriteHeight,
		   u + spriteWidth, v,
		   u, v,
		   u + spriteWidth, v,
		   u, v + spriteHeight,
		   u + spriteWidth, v + spriteHeight
		};
		glBindTexture(GL_TEXTURE_2D, entitiesText);
		

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData);
		glEnableVertexAttribArray(program.texCoordAttribute);

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::scale(modelMatrix, glm::vec3(TILE_SIZE, TILE_SIZE, 1.0f));
		program.SetModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}

	void update(float elapsed) {
		velocity.y += fallSpeed.y * elapsed;
		if (type == PLAYER) {
			for (int i = 0; i < allEntities.size(); i++) {
				if (collisionEntity(allEntities[i]) == true && allEntities[i].type == ITEM) {
					respawn();
				}
			}

		
		if (keys[SDL_SCANCODE_SPACE]) {
			velocity.y += 5.0f * elapsed;
		}
		if (keys[SDL_SCANCODE_D]) {
			velocity.x += 0.5*elapsed;
		}
		if (keys[SDL_SCANCODE_A]) {
			velocity.x += -0.5*elapsed;
		}
		else {
			velocity.x -= stopSpeed.x * elapsed;
		}
		position.x += velocity.x * elapsed;
		position.y += velocity.y * elapsed;
	}
		else if (type == ITEM) {
			position.x += itemVelocity.x *elapsed;
		}
	}

	bool collisionEntity(const Entity& collidedEntity) {
		float x = abs(collidedEntity.position.x - position.x) - (TILE_SIZE* ((collidedEntity.size.x + size.x)/2));
		float y = abs(collidedEntity.position.y - position.y) - (TILE_SIZE* ((collidedEntity.size.y + size.y) / 2));
		if (x <= 0.0f && y <= 0.0f) {
			return true;
		}
		else {
			return false;
		}
	}

	void respawn() {
		for (int i = 0; i < allEntities.size(); i++) {
			allEntities[i].position = allEntities[i].spawn;
		}
	}
	glm::vec3 spawn;
	glm::vec3 position;
	glm::vec3 tile;
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);;
	glm::vec3 itemVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 size = glm::vec3(1.0f);
	Type type;
	// fall speed and stop speed for entity
	glm::vec3 fallSpeed = glm::vec3(0.0, -1.0f, 0.0f);
	glm::vec3 stopSpeed = glm::vec3(0.5, 0.0f, 0.0f);
	int entityIndex;
};


void flareEntity() {
	for (FlareMapEntity& flare : map.entities) {
		float postion_x = flare.x* TILE_SIZE;
		float postion_y = flare.y* TILE_SIZE;
		Entity temp;
		temp.position = glm::vec3(postion_x, postion_y, 0.0f);
		if (flare.type == "Start") {
			temp.type = PLAYER;
			temp.entityIndex = 19;
		}
		else if (flare.type == "item") {
			temp.type = ITEM;
			temp.entityIndex = 28;
		}
		
		temp.spawn = temp.position;
		allEntities.push_back(temp);
	}
}

void renderMap() {

	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (map.mapData[y][x] == 0) {
				map.mapData[y][x] = 24;
			}
			float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

			float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
			float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

			vertexData.insert(vertexData.end(), {
				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,

				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});

			texCoordData.insert(texCoordData.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),
				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
				});

		}
	}
	glBindTexture(GL_TEXTURE_2D, platforms);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	program.SetModelMatrix(modelMatrix);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, LEVEL_HEIGHT * LEVEL_WIDTH * 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}
void render() {
	renderMap();
	for (int i = 0; i < allEntities.size(); i++) {
		allEntities[i].Draw();
	}
}

void update(float elapsed) {
	for (int i = 0; i < allEntities.size(); i++) {
		allEntities[i].update(elapsed);
	}
}
void camera() {
	viewMatrix = glm::mat4(1.0f);
	int index=0;
	for (int i = 0; i < allEntities.size(); i++) {
		if (allEntities[i].type == PLAYER) {
			index = i;
			break;
		}
	}
	viewMatrix = glm::translate(viewMatrix, -allEntities[index].position);
	program.SetViewMatrix(viewMatrix);
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
	
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);

	glUseProgram(program.programID);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	map.Load(RESOURCE_FOLDER"map.txt");
	platforms = LoadTexture(RESOURCE_FOLDER"dirt-tiles.png");
	entitiesText = LoadTexture(RESOURCE_FOLDER"characters_3.png");
	
	flareEntity();
	render();
	float accumulator = 0.0f;
	float lastFrameTicks = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
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
			update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		render();
		//camera();

		//glDisableVertexAttribArray(program.positionAttribute);
		//glDisableVertexAttribArray(program.texCoordAttribute);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}

	



