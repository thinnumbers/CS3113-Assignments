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

SDL_Window* displayWindow;
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
	float lastFrameTicks = 0.0f;
	
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	// Position variables
	float pos1 = 0.0f;
	float pos2 = 0.0f;
	float ballx = -1.6f;
	float bally = 0.1f;
	// vector and velocity variables
	float vectorx = 1.0f;
	float vectory = 1.0f;
	float velocity = 2.5f;

	// textures 
	GLuint player1 = LoadTexture(RESOURCE_FOLDER"bat.png");
	GLuint player2 = LoadTexture(RESOURCE_FOLDER"bat.png");
	GLuint ball = LoadTexture(RESOURCE_FOLDER"baseball.png");
	GLuint win1 = LoadTexture(RESOURCE_FOLDER"left.png");
	GLuint win2 = LoadTexture(RESOURCE_FOLDER"right.png");

	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);


	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	glUseProgram(program.programID);
	


    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
        glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		// PLAYER 1  (LEFT) //
		glBindTexture(GL_TEXTURE_2D, player1);
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.76f, pos1, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 1.0f));

		// PLAYER 1 USES W to move up and S to move down
		if (keys[SDL_SCANCODE_W]) {
			pos1 += (2.0f * elapsed);
			if (pos1 >= 0.8f) {
				pos1 = 0.8f;
			}
		}
		else if (keys[SDL_SCANCODE_S]) {
			pos1 -= (2.0f * elapsed);
			if (pos1 <= -0.8f) {
				pos1 = -0.8f;
			}
		}
		program.SetModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		// PLAYER 2 (RIGHT) //
		glBindTexture(GL_TEXTURE_2D, player2);
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(1.76f, pos2, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 1.0f));

		// PLAYER 2 USES U to move up and J to move down
		if (keys[SDL_SCANCODE_U]) {
			pos2 += (2.0f * elapsed);
			if (pos2 >= 0.8f) {
				pos2 = 0.8f;
			}
		}
		else if (keys[SDL_SCANCODE_J]) {
			pos2 -= (2.0f * elapsed);
			if (pos2 <= -0.8f) {
				pos2 = -0.8f;
			}
		}
		program.SetModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// BALL //

		// COLLISION WITH CEILING //
		if (bally <= -1.0) {
			vectory = -vectory;
			bally = -0.9;
		}
		else if (bally >= 1.0) {
			vectory = -vectory;
			bally = 0.9;
		}
		else {
			// COLLISION WITH PLAYER 2 (RIGHT) //
			if (ballx >= 1.69) {
				if (bally >= pos2 -0.3f && bally <= pos2 + 0.3f) {
					vectorx = -vectorx;
				}
				// COLLISION WITH WALL 2 (When the player 2 misses the ball) //
				else if(ballx > 1.8){
					velocity = 0.0;
					// Displays player 1 as the winner (LEFT WINS)
					glBindTexture(GL_TEXTURE_2D, win1);
					modelMatrix = glm::mat4(1.0f);
					modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 1.0f, 1.0f));
					program.SetModelMatrix(modelMatrix);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}
			}
			// COLLISION WITH PLAYER 1 (LEFT) //
			else if (ballx <= -1.69) {
				if (bally >= pos1 - 0.3f  && bally <= pos1 + 0.3f) {
					vectorx = -vectorx;
				}
				// COLLISION WITH WALL 1 (When the player 1 misses the ball) //
				else if (ballx < -1.8){
					velocity = 0.0;
					// Displays player 2 as the winner (RIGHT WINS)
					glBindTexture(GL_TEXTURE_2D, win2);
					modelMatrix = glm::mat4(1.0f);
					modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 1.0f, 1.0f));
					program.SetModelMatrix(modelMatrix);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}
			}
		}
		glBindTexture(GL_TEXTURE_2D, ball);
		modelMatrix = glm::mat4(1.0f);
		ballx += vectorx * elapsed * velocity;
		bally += vectory * elapsed * velocity;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(ballx, bally, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.08f, 0.08f, 1.0f));
		program.SetModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	
		
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
