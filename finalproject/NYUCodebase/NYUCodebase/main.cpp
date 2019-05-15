#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>

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
#define MAX_BULLETS2 5
#define MAX_TRAP1 2
#define MAX_TRAP2 2
#define FIXED_TIMESTEP 0.0166666f
#define LEFTWALL_POS -1.777f
#define RIGHTWALL_POS 1.777f
#define TOPWALL_POS 1.0f
#define BOTTOMWALL_POS -1.0f

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
	void DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY);
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
	// ALL ENTITIES
	std::vector<Entity> entities;
	std::vector<Entity> bullets;
	std::vector<Entity> bullets2;
	std::vector<Entity> trap1;
	std::vector<Entity> trap2;
	std::vector<Entity> shield;
	std::vector<Entity> swords;
	std::vector<Entity> beams;

	// INDEX OF BULLET
	int bulletNum = 0;
	int bulletNum2 = 0;

	// INDEX OF TRAP
	int trap1Num = 0;
	int trap2Num = 0; 

	// CHECKS IF THEY SELECTED A CLASS
	bool playerSelected = false;  //
	bool opponentSelected = false; //

	// HEALTH
	int playerHealth = 100; //
	int opponentHealth = 100; //

	// ULTIMATE ABILITY
	float playerCharge = 100.0f; //
	float opponentCharge = 100.0f;

	// Knockback AND freeze VALUES
	float knockback = 0.6f;
	float freeze = 1.8f;
	glm::vec3 iceEffect = glm::vec3(100.0f, 0.0f, 1.0f);
	// OPTION SELECT
	int levelNum = 1;
	int menuOption = 1;

	// SOUND
	bool shoot1 = false;
	bool shoot2 = false;
	bool beam = false;
	bool blockSound = false;
	bool freezeSound = false;
	bool option = false;
	bool confirm = false;
	bool trapSound = false;

};

// GLOBAL VARIABLES //
enum GameMode { MAIN_MENU, LEVEL_SELECT, CLASS_SELECT, GAME_LEVEL, GAME_OVER};
GameMode gamemode = MAIN_MENU;
GameState battlegame;
ShaderProgram program;
ShaderProgram program2;
GLuint textSheet;
GLuint minecraft;
GLuint selectClass;
GLuint levelSelect;

const int runAnimation[] = {226, 227, 228, 229, 230, 231, 8, 40, 72, 104};
const int numFrames = 10;
float animationElapsed = 0.0f;
float framesPerSecond = 120.0f;
int currentIndex = 0;

const int runAnimation2[] = { 0, 1, 32, 33 };
const int runAnimation3[] = { 179, 211, 179, 211 };
const int numFrames2 = 4;
int currentIndex2 = 0;

const Uint8 *keys = SDL_GetKeyboardState(NULL);
bool play = false;
bool quit = false;


enum Type {PLAYER, OPPONENT, BULLET, BULLET2, TRAP1, TRAP2, SWORD1, SWORD2, SHIELD, BEAM1, BEAM2, OBSTACLE, EFFECT, ANIMATION};
enum Job {KNIGHT, ARCHER, WALL, HAZARD, TILE};
enum Direction {LEFT, RIGHT, UP, DOWN};
class Entity {
public:
	void Update(float elapsed) {
		if (gamemode == GAME_LEVEL) {
		
			// PLAYER 1
			if (type == PLAYER) {
				if (battlegame.playerHealth <= 0) {
					position.x = 100.0f;
				}
				// COLLISION DETECTION FOR P1 //
				else {
					for (Entity& bullet : battlegame.bullets2) { // BULLET COLLISION P1
						if (collision(bullet)) {
							if (bullet.type == BULLET2) {
								bullet.position.x = 100.0f;
								battlegame.playerHealth -= 10;
							}
						}
						for (Entity& bullet : battlegame.bullets) { // DEFLECTED BULLET COLLISION P1
							if (collision(bullet)) {
								if (bullet.type == BULLET && bullet.deflected == true) {
									bullet.position.x = 100.0f;
									battlegame.playerHealth -= 10;
									bullet.deflected = false;
								}
							}
						}
						for (Entity& sword : battlegame.swords) { // SWORD COLLISION P1
							if (collision(sword) == true && shielded == false) {
								if (sword.type == SWORD2) {
									sword.position.y = 100.0f;
									battlegame.playerHealth -= 10;
									if (sword.direction == UP) {
										position.y += battlegame.knockback;
										if (position.y + (size.y) >= TOPWALL_POS) {
											position.y = TOPWALL_POS - size.y;
										}
									}
									else if (sword.direction == DOWN) {
										position.y -= battlegame.knockback;
										if (position.y - (size.y) <= BOTTOMWALL_POS) {
											position.y = BOTTOMWALL_POS + size.y;
										}
									}
									else if (sword.direction == LEFT) {
										position.x -= battlegame.knockback;
										if (position.x - (size.x) <= LEFTWALL_POS) {
											position.x = LEFTWALL_POS + size.y;
										}
									}
									else if (sword.direction == RIGHT) {
										position.x += battlegame.knockback;
										if (position.x + (size.y) >= RIGHTWALL_POS) {
											position.x = RIGHTWALL_POS - size.y;
										}
									}
								}
							}
						}
						for (Entity& trap : battlegame.trap2) { // TRAP COLLISION P1
							if (collision(trap)) {
								if (trap.type == TRAP2) {
									battlegame.trapSound = true;
									trap.position.x = 100.0f;
									battlegame.playerHealth -= 30;
								}
							}
						}
						for (Entity& beam : battlegame.beams) { // BEAM COLLISION P1
							if (collision(beam)) {
								if (beam.type == BEAM2) {
									beam.position.x = -100.0f;
									battlegame.playerHealth -= 50;
								}
							}
						}
						for (Entity& hazard : battlegame.entities) { // HAZARD COLLISION P1
							if (hazard.type == OBSTACLE && hazard.job == HAZARD && collision(hazard) == true) {
								battlegame.trapSound = true;
								position.x = 100.0f;
								battlegame.playerHealth = 0.0f;
							}
						}
						for (Entity& wall : battlegame.entities) { // WALL COLLISION P1
							if (wall.type == OBSTACLE && wall.job == WALL && collision(wall) == true) {
								if (direction == RIGHT) {
									position.x -= velocity.x *elapsed;
								}
								else if (direction == LEFT) {
									position.x += velocity.x *elapsed;
								}
								else if (direction == UP) {
									position.y -= velocity.y * elapsed;
								}
								else if (direction == DOWN) {
									position.y += velocity.y * elapsed;
								}
							}
						}
					}
					// P1 CONTROLS
					if (frozen == false) {
						if (keys[SDL_SCANCODE_D]) {
							if (position.x + (size.x) <= RIGHTWALL_POS) {
								position.x += velocity.x*elapsed;
								direction = RIGHT;
								hitboxFix(sprite, direction);
								angle = 270.0f * (3.1415926f / 180.0f);
							}
						}
						else if (keys[SDL_SCANCODE_A]) {
							if (position.x - (size.x) >= LEFTWALL_POS) {
								position.x -= velocity.x*elapsed;
								direction = LEFT;
								hitboxFix(sprite, direction);
								angle = 90.0f * (3.1415926f / 180.0f);
							}
						}
						else if (keys[SDL_SCANCODE_W]) {
							if (position.y + (size.y) <= TOPWALL_POS) {
								position.y += velocity.y*elapsed;
								direction = UP;
								hitboxFix(sprite, direction);
								angle = 0.0f;
							}
						}
						else if (keys[SDL_SCANCODE_S]) {
							if (position.y - (size.y) >= BOTTOMWALL_POS) {
								position.y -= velocity.y*elapsed;
								direction = DOWN;
								hitboxFix(sprite, direction);
								angle = 180.0f * (3.1415926f / 180.0f);
							}
						}

						// PRIMARY FIRE P1
						if (keys[SDL_SCANCODE_C]) {
							battlegame.shield[0].position = glm::vec3(-100.0f, 0.0f, 1.0f);
							if (cooldownTimer(elapsed) == true) {
								primaryAttack(direction);
							}
						}
						else {
							battlegame.swords[0].position = glm::vec3(-100.0f, 0.0f, 1.0f);
							cooldown -= elapsed;
						}
						// SECONDARY FIRE P1
						if (keys[SDL_SCANCODE_V]) {
							battlegame.swords[0].position = glm::vec3(-100.0f, 0.0f, 1.0f);
							if (cooldownTimer(elapsed) == true) {
								secondaryAttack(direction);
							}
						}
						else {
							battlegame.shield[0].position = glm::vec3(-100.0f, 0.0f, 1.0f);
							cooldown -= elapsed;
						}
						// ULTIMATE P1
						if (keys[SDL_SCANCODE_B]) {
							if (battlegame.playerCharge == 100.0f) {
								ultimateAttack(direction);
								battlegame.playerCharge = 0.0f;
							}
						}
					}
					else if (frozen == true) {
						frozenTimer -= 0.3f * elapsed;
						if (frozenTimer <= 0.0f) {
							frozen = false;
							frozenTimer = battlegame.freeze;
							battlegame.iceEffect = glm::vec3(100.0f, 0.0f, 1.0f);
						}
					}
				}
			}
			// PLAYER 2
			else if (type == OPPONENT) {
				if (battlegame.opponentHealth <= 0) {
					position.x = 100.0f;
				}
				else {
					// COLLISION DETECTION FOR PLAYER 2

					for (Entity& bullet : battlegame.bullets) { // BULLET COLLISION P2
						if (collision(bullet)) {
							if (bullet.type == BULLET) {
								bullet.position.x = 100.0f;
								battlegame.opponentHealth -= 10;
							}
						}
					}
					for (Entity& bullet : battlegame.bullets2) { // DEFLECTED BULLET COLLISION P2
						if (collision(bullet)) {
							if (bullet.type == BULLET2 && bullet.deflected == true) {
								bullet.position.x = 100.0f;
								battlegame.opponentHealth -= 10;
								bullet.deflected = false;
							}
						}
					}
					for (Entity& sword : battlegame.swords) { // SWORD COLLISION P2
						if (collision(sword) == true && shielded == false) {
							if (sword.type == SWORD1) {
								sword.position.y = 100.0f;
								battlegame.opponentHealth -= 10;
								if (sword.direction == UP) {
									position.y += battlegame.knockback;
									if (position.y + (size.y) >= TOPWALL_POS) {
										position.y = TOPWALL_POS - size.y;
									}
								}
								else if (sword.direction == DOWN) {
									position.y -= battlegame.knockback;
									if (position.y - (size.y) <= BOTTOMWALL_POS) {
										position.y = BOTTOMWALL_POS + size.y;
									}
								}
								else if (sword.direction == LEFT) {
									position.x -= battlegame.knockback;
									if (position.x - (size.x) <= LEFTWALL_POS) {
										position.x = LEFTWALL_POS + size.y;
									}
								}
								else if (sword.direction == RIGHT) {
									position.x += battlegame.knockback;
									if (position.x + (size.y) >= RIGHTWALL_POS) {
										position.x = RIGHTWALL_POS - size.y;
									}
								}
							}
						}
					}
					for (Entity& trap : battlegame.trap1) { // TRAP COLLISION P2
						if (collision(trap)) {
							if (trap.type == TRAP1) {
								battlegame.trapSound = true;
								trap.position.x = 100.0f;
								battlegame.opponentHealth -= 30;
							}
						}
					}

					for (Entity& beam : battlegame.beams) { // BEAM COLLISION P2
						if (collision(beam)) {
							if (beam.type == BEAM1) {
								beam.position.x = -100.0f;
								battlegame.opponentHealth -= 50;
							}
						}
					}

					for (Entity& hazard : battlegame.entities) { // HAZARD COLLISION P2
						if (hazard.type == OBSTACLE && hazard.job == HAZARD && collision(hazard) == true) {
							battlegame.trapSound = true;
							position.x = 100.0f;
							battlegame.opponentHealth = 0.0f;
						}
					}

					for (Entity& wall : battlegame.entities) { // WALL COLLISION P2
						if (wall.type == OBSTACLE && wall.job == WALL && collision(wall) == true) {
							if (direction == RIGHT) {
								position.x -= velocity.x *elapsed;
							}
							else if (direction == LEFT) {
								position.x += velocity.x *elapsed;
							}
							else if (direction == UP) {
								position.y -= velocity.y * elapsed;
							}
							else if (direction == DOWN) {
								position.y += velocity.y * elapsed;
							}
						}
					}
					if (frozen == false) {
						// PLAYER 2 CONTROLS
						if (keys[SDL_SCANCODE_RIGHT]) {
							if (position.x + (size.x) <= RIGHTWALL_POS) {
								position.x += velocity.x*elapsed;
								direction = RIGHT;
								hitboxFix(sprite, direction);
								angle = 270.0f * (3.1415926f / 180.0f);
							}
						}
						else if (keys[SDL_SCANCODE_LEFT]) {
							if (position.x - (size.x) >= LEFTWALL_POS) {
								position.x -= velocity.x*elapsed;
								direction = LEFT;
								hitboxFix(sprite, direction);
								angle = 90.0f * (3.1415926f / 180.0f);
							}
						}
						else if (keys[SDL_SCANCODE_UP]) {
							if (position.y + (size.y) <= TOPWALL_POS) {
								position.y += velocity.y*elapsed;
								direction = UP;
								hitboxFix(sprite, direction);
								angle = 0.0f;
							}
						}
						else if (keys[SDL_SCANCODE_DOWN]) {
							if (position.y - (size.y) >= BOTTOMWALL_POS) {
								position.y -= velocity.y*elapsed;
								direction = DOWN;
								hitboxFix(sprite, direction);
								angle = 180.0f * (3.1415926f / 180.0f);
							}
						}
						// PRIMARY FIRE P2
						if (keys[SDL_SCANCODE_J]) {
							battlegame.shield[1].position = glm::vec3(0.0f, -100.0f, 1.0f);
							if (cooldownTimer(elapsed) == true) {
								primaryAttack(direction);
							}
						}
						else {
							battlegame.swords[1].position = glm::vec3(0.0f, -100.0f, 1.0f);
							cooldown -= elapsed;
						}
						// SECONDARY FIRE P2
						if (keys[SDL_SCANCODE_K]) {
							battlegame.swords[1].position = glm::vec3(0.0f, -100.0f, 1.0f);
							if (cooldownTimer(elapsed) == true) {
								secondaryAttack(direction);
							}
						}
						else {
							battlegame.shield[1].position = glm::vec3(0.0f, 100.0f, 1.0f);
							cooldown -= elapsed;
						}
						// ULTIMATE P2
						if (keys[SDL_SCANCODE_L]) {
							if (battlegame.opponentCharge == 100.0f) {
								ultimateAttack(direction);
								battlegame.opponentCharge = 0.0f;
							}
						}
					}
					else if (frozen == true) {
						frozenTimer -= 0.3f * elapsed;
						if (frozenTimer <= 0.0f) {
							frozen = false;
							frozenTimer = battlegame.freeze;
							battlegame.iceEffect = glm::vec3(100.0f, 0.0f, 1.0f);
						}
					}
				}
			}
			// PROJECTILE ( BULLET AND BEAM )
			else if (type == BULLET || type == BULLET2 || type == BEAM1 || type == BEAM2) {

			
				if (direction == UP) {
					hitboxFix(sprite, direction);
					position.y += elapsed * velocity.y;
					angle = 0.0f;
				}
				else if (direction == DOWN) {
					hitboxFix(sprite, direction);
					position.y += elapsed * -velocity.y;
					angle = 180.0f * (3.1415926f / 180.0f);
				}
				else if (direction == RIGHT) {
					hitboxFix(sprite, direction);
					position.x += elapsed * velocity.x;
					angle = 270.0f * (3.1415926f / 180.0f);
				}
				else if (direction == LEFT) {
					hitboxFix(sprite, direction);
					position.x += elapsed * -velocity.x;
					angle = 90.0f * (3.1415926f / 180.0f);
				}

				for (Entity& wall : battlegame.entities) {
					if (wall.type == OBSTACLE && wall.job == WALL) {
						if ((type == BULLET || type == BULLET2) && collision(wall) == true) {
							position.x = 100.0f;
						}
					}
				}

			}
			// SHIELD
			else if (type == SHIELD) {
				// SHIELD BLOCK COLLISION
				for (Entity& bullet : battlegame.bullets2) { // BULLET DEFLECT
					if (collision(bullet)) {
						if (bullet.type == BULLET2) {
							battlegame.blockSound = true;
							reverseDirection(bullet.direction);
							bullet.deflected = true;
						}
					}
				}
				for (Entity& bullet : battlegame.bullets) { 
					if (collision(bullet)) {
						if (bullet.type == BULLET) {
							battlegame.blockSound = true;
							reverseDirection(bullet.direction);
							bullet.deflected = true;
						}
					}
				}

				for (Entity& sword : battlegame.swords) {  // SWORD BLOCK
					if (collision(sword)) {
						if (sword.type == SWORD1) {
							for (Entity& player : battlegame.entities) {
								if (player.type == OPPONENT) {
									player.shielded = true;
									sword.position.y = 100.0f;
								}
							}
						}
						else if (sword.type == SWORD2) {
							for (Entity& player : battlegame.entities) {
								if (player.type == PLAYER) {
									player.shielded = true;
									sword.position.y = 100.0f;
								}
							}
						}
					}
					else if (collision(sword) == false) {
						if (sword.type == SWORD1) {
							for (Entity& player : battlegame.entities) {
								if (player.type == OPPONENT) {
									player.shielded = false;
								}
							}
						}
						else if (sword.type == SWORD2) {
							for (Entity& player : battlegame.entities) {
								if (player.type == PLAYER) {
									player.shielded = false;
								}
							}
						}
					}

				}

				// SHIELD ORIENTATION
				if (direction == UP) {
					hitboxFix(sprite, direction);
					angle = 90.0f * (3.1415926f / 180.0f);
				}
				else if (direction == DOWN) {
					hitboxFix(sprite, direction);
					angle = 270.0f * (3.1415926f / 180.0f);
				}
				else if (direction == RIGHT) {
					hitboxFix(sprite, direction);
					angle = 0.0f;
				}
				else if (direction == LEFT) {
					hitboxFix(sprite, direction);
					angle = 180.0f * (3.1415926f / 180.0f);
				}
			}
			// SWORD
			else if (type == SWORD1 || type == SWORD2) {
			if (direction == UP) {
				hitboxFix(sprite, direction);
				angle = 180.0f * (3.1415926f / 180.0f);
			}
			else if (direction == DOWN) {
				hitboxFix(sprite, direction);
				angle = 0.0f;
			}
			else if (direction == RIGHT) {
				hitboxFix(sprite, direction);
				angle = 90.0f * (3.1415926f / 180.0f);
			}
			else if (direction == LEFT) {
				hitboxFix(sprite, direction);
				angle = 270.0f * (3.1415926f / 180.0f);
			}
			}
			// OBSTACLE
			else if (type == OBSTACLE) {
				if (direction == UP) {
					hitboxFix(sprite, direction);
					angle = 0.0f;
				}
				else if (direction == DOWN) {
					hitboxFix(sprite, direction);
					angle = 180.0f * (3.1415926f / 180.0f);
				}
				if (battlegame.levelNum == 1){
					if (job == HAZARD) {
						if (position.y + (size.y) <= TOPWALL_POS && position.y - (size.y) >= BOTTOMWALL_POS) {
							position.y += elapsed * velocity.y;
						}
						else {
							velocity.y = -velocity.y;
							position.y += elapsed * velocity.y;
						}
					}
				}
				else if (battlegame.levelNum == 2) {
					if (job == HAZARD) {
						if (position.y + (size.y) <= TOPWALL_POS && position.y - (size.y) >= BOTTOMWALL_POS) {
							position.y += elapsed * velocity.y;
						}
						else {
							velocity.y = -velocity.y;
							reverseDirection(direction);
							position.y += elapsed * velocity.y;
						}
					}
				}
				else if (battlegame.levelNum == 3) {
					if (job == HAZARD) {
						position.x += elapsed * velocity.x;
					}
				}
			}
			
		}
		// CLASS SELECT CONTROLS
		else if (gamemode == CLASS_SELECT) {
			if (battlegame.playerSelected == true && battlegame.opponentSelected == true) {
				gamemode = LEVEL_SELECT;
			}
			else {
				if (type == PLAYER && battlegame.playerSelected == false) {
					if (keys[SDL_SCANCODE_A]) {
						job = KNIGHT;
						battlegame.option = true;
						battlegame.playerSelected = true;
					}
					else if (keys[SDL_SCANCODE_D]) {
						job = ARCHER;
						battlegame.option = true;
						battlegame.playerSelected = true;
					}
				}
				else if (type == OPPONENT && battlegame.opponentSelected == false) {
					if (keys[SDL_SCANCODE_LEFT]) {
						job = KNIGHT;
						battlegame.option = true;
						battlegame.opponentSelected = true;
					}
					else if (keys[SDL_SCANCODE_RIGHT]) {
						job = ARCHER;
						battlegame.option = true;
						battlegame.opponentSelected = true;
					}
				}
			}
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

	// REVERSE DIRECTION
	void reverseDirection(Direction& direction) {
		if (direction == LEFT) {
			direction = RIGHT;
		}
		else if (direction == RIGHT) {
			direction = LEFT;
		}
		else if (direction == UP) {
			direction = DOWN;
		}
		else if (direction == DOWN) {
			direction = UP;
		}
	}

	// HIT BOX FIX //
	void hitboxFix(SheetSprite& sprite, Direction& direction) {
		if (direction == LEFT || direction == RIGHT) {
			sprite.textureSize = glm::vec3(0.5f*sprite.size, 0.5*sprite.size*(sprite.width / sprite.height), 1.0f);
			size = sprite.textureSize;
		}
		else if (direction == UP || direction == DOWN) {
			sprite.textureSize = glm::vec3(0.5f*sprite.size*(sprite.width / sprite.height), 0.5*sprite.size, 1.0f);
			size = sprite.textureSize;
		}
	}

	// COOLDOWN //
	bool cooldownTimer(float elapsed) {
		if (job == ARCHER) {
			if (cooldown <= 0.0f) {
				cooldown = 0.7f;
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return true;
		}
	}

	// RENDERING //
	void render(ShaderProgram &program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		program.SetModelMatrix(modelMatrix);
		sprite.Draw(program);
	}
	// RENDER ANIMATION //
	void renderAnimation(ShaderProgram &program, int index) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		program.SetModelMatrix(modelMatrix);
		sprite.DrawSpriteSheetSprite(program, index, 32, 32); // 32 by 
	}

	// Primary Attack Function
	void primaryAttack(Direction direction) {
		if (job == ARCHER) {
			// BOW AND ARROW
			if (type == PLAYER) {
				battlegame.shoot1 = true;
				battlegame.bullets[battlegame.bulletNum].position.x = position.x;
				battlegame.bullets[battlegame.bulletNum].position.y = position.y;
				battlegame.bullets[battlegame.bulletNum].direction = direction;
				battlegame.bulletNum++;
				if (battlegame.bulletNum > MAX_BULLETS - 1) {
					battlegame.bulletNum = 0;
				}
			}
			else if (type == OPPONENT) {
				battlegame.shoot2 = true;
				battlegame.bullets2[battlegame.bulletNum2].position.x = position.x;
				battlegame.bullets2[battlegame.bulletNum2].position.y = position.y;
				battlegame.bullets2[battlegame.bulletNum2].direction = direction;
				battlegame.bulletNum2++;
				if (battlegame.bulletNum2 > MAX_BULLETS2 - 1) {
					battlegame.bulletNum2 = 0;
				}
			}
		}
		else if (job == KNIGHT) {
			// SWORD ATTACK
			int playerNum = -1;
			if (type == PLAYER) {
				playerNum = 0;
			}
			else if (type == OPPONENT) {
				playerNum = 1;
			}
			if (playerNum >= 0) {
				if (direction == RIGHT) {
					battlegame.swords[playerNum].position.x = position.x + 0.15f;
					battlegame.swords[playerNum].position.y = position.y;
					battlegame.swords[playerNum].direction = direction;
				}
				else if (direction == LEFT) {
					battlegame.swords[playerNum].position.x = position.x - 0.15f;
					battlegame.swords[playerNum].position.y = position.y;
					battlegame.swords[playerNum].direction = direction;
				}
				else if (direction == UP) {
					battlegame.swords[playerNum].position.x = position.x;
					battlegame.swords[playerNum].position.y = position.y + 0.15f;
					battlegame.swords[playerNum].direction = direction;
				}
				else if (direction == DOWN) {
					battlegame.swords[playerNum].position.x = position.x;
					battlegame.swords[playerNum].position.y = position.y - 0.15f;
					battlegame.swords[playerNum].direction = direction;
				}
			}
		}
	}

	// Secondary Attack Function
	void secondaryAttack(Direction direction) {
		// TRAPS
		if (job == ARCHER) {
			if (type == PLAYER) {
				battlegame.trap1[battlegame.trap1Num].position.x = position.x;
				battlegame.trap1[battlegame.trap1Num].position.y = position.y;
				battlegame.trap1Num++;
				if (battlegame.trap1Num > MAX_TRAP1 - 1) {
					battlegame.trap1Num = 0;
				}
			}
			else if (type == OPPONENT) {
				battlegame.trap2[battlegame.trap2Num].position.x = position.x;
				battlegame.trap2[battlegame.trap2Num].position.y = position.y;
				battlegame.trap2Num++;
				if (battlegame.trap2Num > MAX_TRAP2 - 1) {
					battlegame.trap2Num = 0;
				}
			}
			

		}
		else if (job == KNIGHT) {
			// SHIELD
			int playerNum = -1;
			if (type == PLAYER) {
				playerNum = 0;
			}
			else if (type == OPPONENT) {
				playerNum = 1;
			}
			if(playerNum >= 0 ){
				if (direction == RIGHT) {
					battlegame.shield[playerNum].position.x = position.x + 0.15f;
					battlegame.shield[playerNum].position.y = position.y;
					battlegame.shield[playerNum].direction = direction;
				}
				else if (direction == LEFT) {
					battlegame.shield[playerNum].position.x = position.x - 0.15f;
					battlegame.shield[playerNum].position.y = position.y;
					battlegame.shield[playerNum].direction = direction;
				}
				else if (direction == UP) {
					battlegame.shield[playerNum].position.x = position.x;
					battlegame.shield[playerNum].position.y = position.y + 0.15f;
					battlegame.shield[playerNum].direction = direction;
				}
				else if (direction == DOWN) {
					battlegame.shield[playerNum].position.x = position.x;
					battlegame.shield[playerNum].position.y = position.y - 0.15f;
					battlegame.shield[playerNum].direction = direction;
				}
			}
		}
	}

	// ULTIMATE ATTACK
	void ultimateAttack(Direction direction) {
		// FREEZE
		if (job == ARCHER) {
			if (type == PLAYER) {
				for (Entity& entity : battlegame.entities) {
					if (entity.type == OPPONENT) {
						battlegame.freezeSound = true;
						entity.frozen = true;
						battlegame.iceEffect = entity.position;
					}
				}
			}
			else if (type == OPPONENT) {
				for (Entity& entity : battlegame.entities) {
					if (entity.type == PLAYER) {
						battlegame.freezeSound = true;
						entity.frozen = true;
						battlegame.iceEffect = entity.position;
					}
				}
			}
		}
		// BEAM
		if (job == KNIGHT) {
			int playerNum = -1;
			if (type == PLAYER) {
				playerNum = 0;
			}
			else if (type == OPPONENT) {
				playerNum = 1;
			}
			if (playerNum >= 0) {
				if (direction == RIGHT) {
					battlegame.beam = true;
					battlegame.beams[playerNum].position.x = position.x + 0.15f;
					battlegame.beams[playerNum].position.y = position.y;
					battlegame.beams[playerNum].direction = direction;
				}
				else if (direction == LEFT) {
					battlegame.beam = true;
					battlegame.beams[playerNum].position.x = position.x - 0.15f;
					battlegame.beams[playerNum].position.y = position.y;
					battlegame.beams[playerNum].direction = direction;
				}
				else if (direction == UP) {
					battlegame.beam = true;
					battlegame.beams[playerNum].position.x = position.x;
					battlegame.beams[playerNum].position.y = position.y + 0.15f;
					battlegame.beams[playerNum].direction = direction;
				}
				else if (direction == DOWN) {
					battlegame.beam = true;
					battlegame.beams[playerNum].position.x = position.x;
					battlegame.beams[playerNum].position.y = position.y - 0.15f;
					battlegame.beams[playerNum].direction = direction;
				}
			}
		}
	}
	// VARIABLES
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;

	float angle = 0.0f;
	float cooldown = 0.0f;
	float frozenTimer = battlegame.freeze;
	bool isAlive = true;
	bool leftCollision = false;
	bool rightCollision = false;
	
	SheetSprite sprite;
	Type type;
	Job job;
	Direction direction;
	int obstacleID = 0;

	//STATUS 
	bool frozen = false;
	bool shielded = false;
	bool deflected = false;
	bool leftright = false;
	bool blocked_left = false;
	bool blocked_right = false;
	bool blocked_up = false;
	bool blocked_down = false;
	
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


void updateGame(GameState& battlegame, float elapsed) {
	for (Entity& entity : battlegame.entities) {
		entity.Update(FIXED_TIMESTEP);
	}
	for (int i = 0;i < MAX_BULLETS;i++) {
		battlegame.bullets[i].Update(FIXED_TIMESTEP);
	}

	for (int i = 0;i < MAX_BULLETS2;i++) {
		battlegame.bullets2[i].Update(FIXED_TIMESTEP);
	} 

	for (Entity& entity : battlegame.shield) {
		entity.Update(FIXED_TIMESTEP);
	}

	for (Entity& entity : battlegame.swords) {
		entity.Update(FIXED_TIMESTEP);
	}
	for (Entity& entity : battlegame.beams) {
		entity.Update(FIXED_TIMESTEP);
	}
	if (keys[SDL_SCANCODE_ESCAPE]) {
		quit = true;
	}

}

void updateLevelSelect(float elapsed) {
	if (keys[SDL_SCANCODE_1]) {
		battlegame.levelNum = 1;
		battlegame.option = true;
		gamemode = GAME_LEVEL;
	}
	else if (keys[SDL_SCANCODE_2]) {
		battlegame.levelNum = 2;
		battlegame.option = true;
		gamemode = GAME_LEVEL;
	}
	else if (keys[SDL_SCANCODE_3]) {
		battlegame.levelNum = 3;
		battlegame.option = true;
		gamemode = GAME_LEVEL;
	}
	if (keys[SDL_SCANCODE_ESCAPE]) {
		quit = true;
	}
}

void updateMenu(float elapsed) {
	if (keys[SDL_SCANCODE_SPACE]) {
		battlegame.confirm = true;
		play = true;
	}
	if (keys[SDL_SCANCODE_ESCAPE]) {
		quit = true;
	}
}

void updateClass(GameState& battlegame, float elapsed) {
	for (Entity& entity : battlegame.entities) {
		entity.Update(FIXED_TIMESTEP);
	}
	if (keys[SDL_SCANCODE_ESCAPE]) {
		quit = true;
	}
}
void updateGameover(float elapsed) {
	if (keys[SDL_SCANCODE_ESCAPE]) {
		quit = true;
	}
}
void Update(float elapsed) {
	switch (gamemode) {
	case GAME_LEVEL:
		updateGame(battlegame, elapsed);
		break;
	case MAIN_MENU:
		updateMenu(elapsed);
		break;
	case LEVEL_SELECT:
		updateLevelSelect(elapsed);
		break;
	case CLASS_SELECT:
		updateClass(battlegame, elapsed);
		break;
	case GAME_OVER:
		updateGameover(elapsed);
		break;
	}
}

// GAME RENDER //
void renderGame(GameState& battlegame) {
	glClear(GL_COLOR_BUFFER_BIT);
	for (Entity& entity : battlegame.entities) {
		if (battlegame.levelNum == 1 && entity.type == OBSTACLE && entity.job != TILE && entity.obstacleID == 1) {
			if (entity.job == HAZARD) {
				entity.renderAnimation(program, runAnimation2[currentIndex2]);
			}
			else {
				entity.render(program);
			}
		}
		else if (battlegame.levelNum == 2 && entity.type == OBSTACLE && entity.job == TILE && entity.obstacleID == 2) {
			entity.render(program);
		}
		else if (battlegame.levelNum == 3 && entity.type == OBSTACLE && entity.job != TILE && entity.obstacleID == 3) {
			entity.render(program);
		}
		else if (battlegame.levelNum == 1 && entity.type == OBSTACLE && entity.job != TILE && entity.obstacleID != 1) {
			entity.position.x = 300.0f;
			entity.position.y = 300.0f;
		}
		else if (battlegame.levelNum == 2 && entity.type == OBSTACLE && entity.job != TILE && entity.obstacleID != 2) {
			entity.position.x = 300.0f;
			entity.position.y = 300.0f;
		}
		else if (battlegame.levelNum == 3 && entity.type == OBSTACLE && entity.job != TILE && entity.obstacleID != 3) {
			entity.position.x = 300.0f;
			entity.position.y = 300.0f;
		}
		
	}

	for (Entity& entity : battlegame.entities) {
		if (battlegame.levelNum == 1 && entity.type == OBSTACLE && entity.job == TILE) {
			if (entity.obstacleID == 1) {
				entity.render(program);
			}
		}
		if (battlegame.levelNum == 2 && entity.type == OBSTACLE && entity.job != TILE) {
			if (entity.obstacleID == 2) {
				entity.renderAnimation(program, runAnimation3[currentIndex2]);
			}
		}
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		battlegame.bullets[i].render(program);
	}
	for (int i = 0; i < MAX_BULLETS2; i++) {
		battlegame.bullets2[i].render(program);
	}

	for (int i = 0; i < MAX_TRAP1; i++) {
		battlegame.trap1[i].render(program);
	}

	for (int i = 0; i < MAX_TRAP2; i++) {
		battlegame.trap2[i].render(program);
	}

	for (Entity& entity : battlegame.shield) {
		entity.render(program);
	}

	for (Entity& entity : battlegame.swords) {
		entity.render(program);
	}
	for (Entity& entity : battlegame.beams) {
		entity.render(program);
	}
	for (Entity& entity : battlegame.entities) {
		if (entity.type == PLAYER || entity.type == OPPONENT) {
			entity.render(program);
		}
	}
	for (Entity& entity : battlegame.entities) {
		if (entity.type == EFFECT) {
			entity.position = battlegame.iceEffect;
			entity.render(program);
		}
	}
	for (Entity& entity : battlegame.entities) {
		if (entity.type == ANIMATION) {
			entity.position = battlegame.iceEffect;
			entity.renderAnimation(program, runAnimation[currentIndex]);
		}
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

	program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.2f, 0.5f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.7f, 0.7f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	
	DrawText(program, textSheet, "Braindead Arena", 0.25f, 0.0005f);
	SheetSprite play = SheetSprite(minecraft, 0.0f / 1024.0f, 97.0f / 1024.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.5f);
	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.5f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	play.Draw(program);
}

void renderClass() {
	glClear(GL_COLOR_BUFFER_BIT);
	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);
	
	program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.35f, 0.8f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	DrawText(program, textSheet, "Select Class", 0.25f, 0.0005f);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(2.3f, 2.3f, 1.0f));
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetModelMatrix(modelMatrix);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	glBindTexture(GL_TEXTURE_2D, selectClass);
	
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void renderLevelSelect(){
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
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.35f, 0.8f, 1.0f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
	program.SetModelMatrix(modelMatrix);
	DrawText(program, textSheet, "Select Level" , 0.25f, 0.0005f);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(2.5f, 2.5f, 1.0f));
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	program.SetModelMatrix(modelMatrix);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	glBindTexture(GL_TEXTURE_2D, levelSelect);

	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void renderGameOver() {
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
	if (battlegame.playerHealth <= 0) {
		DrawText(program, textSheet, "GAME OVER, PLAYER 2 WINS", 0.25f, 0.0005f);
	}
	else if (battlegame.opponentHealth <= 0) {
		DrawText(program, textSheet, "GAME OVER, PLAYER 1 WINS", 0.25f, 0.0005f);
	}
	else if (battlegame.playerHealth <= 0 && battlegame.opponentHealth <= 0) {
		DrawText(program, textSheet, "GAME OVER, IT'S A TIE", 0.25f, 0.0005f);
	}
		
	
}


// RENDER ALL // 
void Render() {
	switch (gamemode) {
	case GAME_LEVEL:
		renderGame(battlegame);
		break;
	case CLASS_SELECT:
		renderClass();
		break;
	case LEVEL_SELECT:
		renderLevelSelect();
		break;
	case MAIN_MENU:
		renderMenu();
		break;
	case GAME_OVER:
		renderGameOver();
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("BRAINDEAD ARENA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
#ifdef _WINDOWS
	glewInit();
#endif
	glViewport(0, 0, 640, 360);
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	program2.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
	minecraft = LoadTexture(RESOURCE_FOLDER"minecraft.png");
	textSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	selectClass = LoadTexture(RESOURCE_FOLDER"class.png");
	levelSelect = LoadTexture(RESOURCE_FOLDER"level.png");
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);


	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	// MUSIC AND SOUND
	Mix_Music *music;
	music = Mix_LoadMUS(RESOURCE_FOLDER"smashkazoo.mp3");
	Mix_PlayMusic(music, -1);
	Mix_VolumeMusic(40);

	// ARROW SOUND
	Mix_Chunk *pew;
	pew = Mix_LoadWAV(RESOURCE_FOLDER"pew.wav");

	// FREEZE ULTIMATE SOUND
	Mix_Chunk *freeze;
	freeze = Mix_LoadWAV(RESOURCE_FOLDER"freeze.wav");

	// GAMEOVER SOUND
	Mix_Chunk *trap;
	trap = Mix_LoadWAV(RESOURCE_FOLDER"gameover.wav");

	// OPTION SELECTION SOUND
	Mix_Chunk *select;
	select = Mix_LoadWAV(RESOURCE_FOLDER"select.wav");

	// OPTION CONFIRM SOUND
	Mix_Chunk *confirm;
	confirm = Mix_LoadWAV(RESOURCE_FOLDER"confirm.wav");

	// BEAM ULTIMATE SOUND
	Mix_Chunk *beam;
	beam = Mix_LoadWAV(RESOURCE_FOLDER"beam.wav");

	// SHIELD BLOCK SOUND
	Mix_Chunk *block;
	block = Mix_LoadWAV(RESOURCE_FOLDER"block.wav");
	
	float x = -0.5f;
	float y = 0.6f;
	for (int i = 0; i <= 12; i++) {
		if (x > 0.5f) {
			x = -0.5f;
			y = 0.4f;
		}
	
		
	}
	// OBSTACLES
	//                                                                       STAGE 1

	// LAVA
		Entity obstacle;
		obstacle.sprite = SheetSprite(minecraft, 0.0f / 1024.0f, 0.0f / 1024.0f, 64.0f / 1024.0f, 64.0f / 1024.0f, 1.2f);
		obstacle.type = OBSTACLE;
		obstacle.job = HAZARD;
		obstacle.obstacleID = 1;
		obstacle.size = obstacle.sprite.textureSize;
		obstacle.position = glm::vec3(0.0f, 0.0f, 1.0f);
		obstacle.velocity = glm::vec3(0.0f, 0.2f, 1.0f);
		battlegame.entities.push_back(obstacle);
	
	
	// HAYBALE WALLS
	for (int i = 0; i < 4; i++) {
		Entity obstacle;
		obstacle.sprite = SheetSprite(minecraft, 65.0f / 1024.0f, 384.0f / 1024.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.3f);
		obstacle.type = OBSTACLE;
		obstacle.job = WALL;
		obstacle.obstacleID = 1;
		obstacle.velocity = glm::vec3(0.0f, 0.0f, 1.0f);
		obstacle.size = obstacle.sprite.textureSize;
		if (i == 0) {
			obstacle.position = glm::vec3(-1.2f, -0.85f, 1.0f);
		}
		else if (i == 1) {
			obstacle.position = glm::vec3(-1.2f, -0.55f, 1.0f);
		}
		else if (i == 2) {
			obstacle.position = glm::vec3(1.2f, 0.85f, 1.0f);
		}
		else {
			obstacle.position = glm::vec3(1.2f, 0.55f, 1.0f);
		}
		battlegame.entities.push_back(obstacle);
	}
	//                                                                       STAGE 2 

	// MOVING TRAIN
	for (int i = 0; i < 3; i++) {
		Entity obstacle;
		obstacle.sprite = SheetSprite(minecraft, 65.0f / 1024.0f, 192.0f / 1024.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.3f);
		obstacle.type = OBSTACLE;
		obstacle.job = HAZARD;
		obstacle.obstacleID = 2;
		obstacle.size = obstacle.sprite.textureSize;
		if (i == 0) {
			obstacle.position = glm::vec3(-0.8f, 0.85f, 1.0f);
			obstacle.angle = 180.0f * (3.1415926f / 180.0f);
			obstacle.direction = DOWN;
			obstacle.velocity = glm::vec3(0.0f, -0.5f, 1.0f);
		}
		else if (i == 1) {
			obstacle.position = glm::vec3(0.0f, -0.85f, 1.0f);
			obstacle.direction = UP;
			obstacle.velocity = glm::vec3(0.0f, 0.5f, 1.0f);
		}
		else {
			obstacle.position = glm::vec3(0.8f, 0.85f, 1.0f);
			obstacle.angle = 180.0f * (3.1415926f / 180.0f);
			obstacle.direction = DOWN;
			obstacle.velocity = glm::vec3(0.0f, -0.5f, 1.0f);
		}
		battlegame.entities.push_back(obstacle);
	}

	// RAIL TRACK
	float connect = 0.0f;
	for (int i = 0; i < 21; i++) {
		Entity obstacle;
		obstacle.sprite = SheetSprite(minecraft, 576.0f / 1024.0f, 65.0f / 1024.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.3f);
		obstacle.type = OBSTACLE;
		obstacle.job = TILE;
		obstacle.obstacleID = 2;
		obstacle.velocity = glm::vec3(0.0f, 0.0f, 1.0f);
		obstacle.size = obstacle.sprite.textureSize;
		if (i <= 6) {
			obstacle.position = glm::vec3(-0.8f, 0.85f + connect, 1.0f);
			connect -= 0.3f;
		}
		else if (i > 6 && i <= 13) {
			obstacle.position = glm::vec3(0.0f, -1.15f - connect, 1.0f);
			connect += 0.3f;
		}
		else if (i >13){
			obstacle.position = glm::vec3(0.8f, 0.85f + connect, 1.0f);
			connect -= 0.3f;
		}
		battlegame.entities.push_back(obstacle);
	}
	//                                                                     STAGE 3

	// WALL
	for (int i = 0; i < 2; i++) {
		Entity obstacle;
		obstacle.sprite = SheetSprite(minecraft, 512.0f / 1024.0f, 352.0f / 1024.0f, 32.0f / 1024.0f, 96.0f / 1024.0f, 5.0f);
		obstacle.type = OBSTACLE;
		obstacle.job = HAZARD;
		obstacle.obstacleID = 3;
		obstacle.size = obstacle.sprite.textureSize;
		if (i == 0) {
			obstacle.position = glm::vec3(3.0f, 0.0f, 1.0f);
			obstacle.velocity = glm::vec3(-0.2f, 0.0f, 1.0f);
		}
		else if (i == 1) {
			obstacle.position = glm::vec3(-3.0f, 0.0f, 1.0f);
			obstacle.velocity = glm::vec3(0.2f, 0.0f, 1.0f);
		}
		battlegame.entities.push_back(obstacle);
	}
	// BULLETS/ARROWS
	for (int i = 0; i < MAX_BULLETS; i++) {
		Entity bullet;
		battlegame.bullets.push_back(bullet);
		battlegame.bullets[i].sprite = SheetSprite(spriteSheet, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.20f);
		battlegame.bullets[i].velocity = glm::vec3(2.0f, 2.0f, 1.0f);
		battlegame.bullets[i].size = battlegame.bullets[i].sprite.textureSize;
		battlegame.bullets[i].position = glm::vec3(100.0f, 0.0f, 1.0f);
		battlegame.bullets[i].type = BULLET;
	}
	for (int i = 0; i < MAX_BULLETS2; i++) {
		Entity bullet2;
		battlegame.bullets2.push_back(bullet2);
		battlegame.bullets2[i].sprite = SheetSprite(spriteSheet, 856.0f / 1024.0f, 421.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0.20f);
		battlegame.bullets2[i].velocity = glm::vec3(2.0f, 2.0f, 1.0f);
		battlegame.bullets2[i].size = battlegame.bullets2[i].sprite.textureSize;
		battlegame.bullets2[i].position = glm::vec3(100.0f, 0.0f, 1.0f);
		battlegame.bullets2[i].type = BULLET2;
	}
	// TRAPS
	for (int i = 0; i < MAX_TRAP1; i++) {
		Entity traps1; // P1
		battlegame.trap1.push_back(traps1);
		battlegame.trap1[i].sprite = SheetSprite(spriteSheet, 776.0f / 1024.0f, 895.0f / 1024.0f, 34.0f / 1024.0f, 33.0f / 1024.0f, 0.20f);
		battlegame.trap1[i].velocity = glm::vec3(0.0f, 0.0f, 1.0f);
		battlegame.trap1[i].size = battlegame.trap1[i].sprite.textureSize;
		battlegame.trap1[i].position = glm::vec3(100.0f, 0.0f, 1.0f);
		battlegame.trap1[i].type = TRAP1;
	}

	for (int i = 0; i < MAX_TRAP2; i++) {
		Entity traps2; // P2
		battlegame.trap2.push_back(traps2);
		battlegame.trap2[i].sprite = SheetSprite(spriteSheet, 774.0f / 1024.0f, 977.0f / 1024.0f, 34.0f / 1024.0f, 33.0f / 1024.0f, 0.20f);
		battlegame.trap2[i].velocity = glm::vec3(0.0f, 0.0f, 1.0f);
		battlegame.trap2[i].size = battlegame.trap2[i].sprite.textureSize;
		battlegame.trap2[i].position = glm::vec3(100.0f, 0.0f, 1.0f);
		battlegame.trap2[i].type = TRAP2;
	}
	// SHIELD
	Entity shield; // P1
	battlegame.shield.push_back(shield);
	battlegame.shield[0].sprite = SheetSprite(spriteSheet, 805.0f / 1024.0f, 0.0f / 1024.0f, 26.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
	battlegame.shield[0].type = SHIELD;
	battlegame.shield[0].size = battlegame.shield[0].sprite.textureSize;
	battlegame.shield[0].position = glm::vec3(0.0f, -100.0f, 1.0f);

	Entity shield2; // P2
	battlegame.shield.push_back(shield2);
	battlegame.shield[1].sprite = SheetSprite(spriteSheet, 809.0f / 1024.0f, 712.0f / 1024.0f, 26.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);
	battlegame.shield[1].type = SHIELD;
	battlegame.shield[1].size = battlegame.shield[1].sprite.textureSize;
	battlegame.shield[1].position = glm::vec3(0.0f, -100.0f, 1.0f);

	// SWORD
	Entity sword1; // P1
	battlegame.swords.push_back(sword1);
	battlegame.swords[0].sprite = SheetSprite(spriteSheet, 596.0f / 1024.0f, 892.0f / 1024.0f, 51.0f / 1024.0f, 69.0f / 1024.0f, 0.2f);
	battlegame.swords[0].type = SWORD1;
	battlegame.swords[0].size = battlegame.swords[0].sprite.textureSize;
	battlegame.swords[0].position = glm::vec3(0.0f, 100.0f, 1.0f);

	Entity sword2; // P2
	battlegame.swords.push_back(sword2);
	battlegame.swords[1].sprite = SheetSprite(spriteSheet, 596.0f / 1024.0f, 892.0f / 1024.0f, 51.0f / 1024.0f, 69.0f / 1024.0f, 0.2f);
	battlegame.swords[1].type = SWORD2;
	battlegame.swords[1].size = battlegame.swords[1].sprite.textureSize;
	battlegame.swords[1].position = glm::vec3(0.0f, 100.0f, 1.0f);

	// SWORD BEAM
	Entity beam1; // P1
	battlegame.beams.push_back(beam1);
	battlegame.beams[0].sprite = SheetSprite(spriteSheet, 0.0f / 1024.0f, 78.0f / 1024.0f, 222.0f / 1024.0f, 39.0f / 1024.0f, 0.2f);
	battlegame.beams[0].type = BEAM1;
	battlegame.beams[0].velocity = glm::vec3(1.5f, 1.5f, 1.0f);
	battlegame.beams[0].size = battlegame.beams[0].sprite.textureSize;
	battlegame.beams[0].position = glm::vec3(-100.0f, 0.0f, 1.0f);

	Entity beam2; // P2
	battlegame.beams.push_back(beam2);
	battlegame.beams[1].sprite = SheetSprite(spriteSheet, 0.0f / 1024.0f, 0.0f / 1024.0f, 222.0f / 1024.0f, 39.0f / 1024.0f, 0.2f);
	battlegame.beams[1].type = BEAM2;
	battlegame.beams[1].velocity = glm::vec3(1.5f, 1.5f, 1.0f);
	battlegame.beams[1].size = battlegame.beams[1].sprite.textureSize;
	battlegame.beams[1].position = glm::vec3(-100.0f, 0.0f, 1.0f);
	
	// PLAYER 1
	Entity player;
	player.sprite = SheetSprite(spriteSheet, 736.0f / 1024.0f, 862.0f / 1024.0f, 40.0f / 1024.0f, 40.0f / 1024.0f, 0.2f);
	player.type = PLAYER;
	player.direction = RIGHT;
	player.angle = 270.0f * (3.1415926f / 180.0f);
	player.velocity = glm::vec3(0.7f, 0.7f, 1.0f);
	player.size = player.sprite.textureSize;
	player.position = glm::vec3(-1.5f, 0.0f, 1.0f);
	battlegame.entities.push_back(player);

	// PLAYER 2
	Entity opponent;
	opponent.sprite = SheetSprite(spriteSheet, 736.0f / 1024.0f, 862.0f / 1024.0f, 40.0f / 1024.0f, 40.0f / 1024.0f, 0.2f);
	opponent.type = OPPONENT;
	opponent.direction = LEFT;
	opponent.angle = 90.0f * (3.1415926f / 180.0f);
	opponent.velocity = glm::vec3(0.7f, 0.7f, 1.0f);
	opponent.size = opponent.sprite.textureSize;
	opponent.position = glm::vec3(1.5f, 0.0f, 1.0f);
	battlegame.entities.push_back(opponent);

	// ICE
	Entity ice;
	ice.sprite = SheetSprite(minecraft, 224.0f / 1024.0f, 384.0f / 1024.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.2f);
	ice.type = EFFECT;
	ice.velocity = glm::vec3(0.0f, 0.0f, 1.0f);
	ice.size = ice.sprite.textureSize;
	ice.position = battlegame.iceEffect;
	battlegame.entities.push_back(ice);

	// CRACK EFFECT
	Entity crack;
	crack.sprite = SheetSprite(minecraft, 0.0f, 0.0f, 32.0f / 1024.0f, 32.0f / 1024.0f, 0.2f);
	crack.type = ANIMATION;
	crack.velocity = glm::vec3(0.0f, 0.0f, 1.0f);
	crack.size = crack.sprite.textureSize;
	crack.position = battlegame.iceEffect;
	battlegame.entities.push_back(crack);

	

	float accumulator = 0.0f;
	float lastFrameTicks = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || quit == true) {
				done = true;
			}
			if (play == true && gamemode == MAIN_MENU) {
				gamemode = CLASS_SELECT;
			}
			if (battlegame.playerSelected == true && battlegame.opponentSelected == true) {
				gamemode = LEVEL_SELECT;
				battlegame.playerSelected = false;
				battlegame.opponentSelected = false;
			}
			if (battlegame.playerHealth <= 0 || battlegame.opponentHealth <= 0) {
				gamemode = GAME_OVER;
			}
		}
		if (battlegame.shoot1 == true) {
			Mix_PlayChannel(1, pew, 0);
			battlegame.shoot1 = false;
		}
		if (battlegame.shoot2 == true) {
			Mix_PlayChannel(2, pew, 0);
			battlegame.shoot2 = false;
		}
		if (battlegame.beam == true) {
			Mix_PlayChannel(3, beam, 0);
			battlegame.beam = false;
		}
		if (battlegame.blockSound == true) {
			Mix_PlayChannel(4, block, 0);
			battlegame.blockSound = false;
		}
		if (battlegame.freezeSound == true) {
			Mix_PlayChannel(5, freeze, 0);
			battlegame.freezeSound = false;
		}
		if (battlegame.option == true) {
			Mix_PlayChannel(1, select, 0);
			battlegame.option = false;
		}
		if (battlegame.confirm == true) {
			Mix_PlayChannel(1, confirm, 0);
			battlegame.confirm = false;
		}
		if (battlegame.trapSound == true) {
			Mix_PlayChannel(1, trap, 0);
			battlegame.trapSound = false;
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

		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			currentIndex++;
			currentIndex2++;
			animationElapsed = 0.0;

			if (currentIndex > numFrames - 1) {
				currentIndex = 0;
			}
			if (currentIndex2 > numFrames2 - 1) {
				currentIndex2 = 0;
			}
		}
		
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
		0.5f * size * aspect, 0.5f * size,
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

	
// ANIMATION
void SheetSprite::DrawSpriteSheetSprite(ShaderProgram &program, int index, int spriteCountX, int spriteCountY) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;


	float texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};

	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
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


