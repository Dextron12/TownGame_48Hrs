#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "window.hpp"

#include <unordered_map>
#include <vector>

#include <ctime>
#include <fstream>

enum Direction {
	NORTH, 
	EAST,
	SOUTH,
	WEST
};

float Lerp(float start, float end, float t) {
	return start + (end - start) * t;
}
template <typename T>
SDL_Texture* loadTexture(SDL_Renderer* renderer, std::string texturePath, T *texW, T *texH) {
	SDL_Surface* img = IMG_Load(texturePath.c_str());

	if (img == nullptr) {
		std::printf("Unable to load image from path: %s\n", texturePath.c_str());
		return nullptr;
	}

	//Get img data:
	*texW = img->w; *texH = img->h;


	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, img);
	if (tex == nullptr) {
		std::printf("Failed to convert image (%s) to a texture\n", texturePath.c_str());
		return nullptr;
	}
	SDL_FreeSurface(img);
	return tex;
}

class TextureAtlas {
public:
	TextureAtlas(SDL_Renderer* renderer, std::string pathToAtlas, SDL_Point subTextureSize)
		: subTextureSize(subTextureSize)
	{
		// Load texture atlas.
		atlas = loadTexture(renderer, pathToAtlas, &atlasSize.x, &atlasSize.y);
		if (atlas == nullptr) {
			std::cerr << "Failed to load texture atlas: " << pathToAtlas << std::endl;
			return; // Quit function if texture is not loaded
		}
	}

	// Adds a subTexture. The UV coords should be provided in texture space.
	void addSubTexture(int textureID, int U, int V) {
		if (subTextures.count(textureID)) {
			std::printf("The texture %d already exists\n", textureID);
			return;
		}
		subTextures[textureID] = { U, V };
	}

	// Use a subtexture by ID, and render it to the screen at the given position.
	void useSubTexture(int textureID, SDL_Renderer* renderer, SDL_Point pos) {
		if (subTextures.count(textureID)) {
			// Calculate the coordinates for the subtexture in texture space.
			render_intPos = {
				subTextures[textureID].x * subTextureSize.x,
				subTextures[textureID].y * subTextureSize.y,
				subTextureSize.x,
				subTextureSize.y
			};
			// Position to render the subtexture on the screen.
			render_Pos = { pos.x, pos.y, subTextureSize.x, subTextureSize.y };
			// Render the subtexture
			SDL_RenderCopy(renderer, atlas, &render_intPos, &render_Pos);
		}
		else {
			//std::printf("No texture with the ID %d exists\n", textureID);
		}
	}

	// Automatically generate subtextures based on the atlas dimensions.
	void autoGenerateTextures(int stopAt = -1) {
		int noOfTextures = 0;
		for (int y = 0; y < atlasSize.y / subTextureSize.y; y++) {
			for (int x = 0; x < atlasSize.x / subTextureSize.x; x++) {
				if (stopAt >= 0 && noOfTextures >= stopAt) {
					return; // Stop when we've reached the stopAt limit
				}
				noOfTextures++;
				subTextures[noOfTextures] = { x, y };
			}
		}
		std::cout << "Number of textures identified: " << noOfTextures << std::endl;
	}

	// Destructor to clean up the loaded texture
	~TextureAtlas() {
		if (atlas) {
			SDL_DestroyTexture(atlas);
		}
	}

public:
	std::unordered_map<int, SDL_Point> subTextures;

	SDL_Point atlasSize;
	SDL_Point subTextureSize;

	SDL_Texture* atlas;
	SDL_Rect render_intPos, render_Pos;
};


class animatedSprite {
public:
	//Loads an animated sprite from a texture atlas.
	animatedSprite(SDL_Renderer* renderer, std::string texturePath, SDL_Point frameSize, float timeBetweenFrame) : m_frameSize(frameSize), renderer(renderer), m_frameTime(timeBetweenFrame) {
		texImg = loadTexture(renderer, texturePath, &m_texSize.x, &m_texSize.y);

		//Calculate amx frame ticks:
		m_maxFrameTick = m_texSize.x / m_frameSize.x; // Divides the texture width by the horiz size of the frame.
	}

	void addSequence(std::string seqName, std::vector<int> seqArr) {
		if (Sequences.find(seqName) == Sequences.end()) {
			//No key of that value exists, create a new one.
			Sequences.insert_or_assign(seqName, seqArr);
		}
		else {
			std::printf("%s is already a valid sequence for this animated object.\n", seqName.c_str());
		}
	}

	void Render(std::string seqID, bool flipHorix=false, float frameTimeOverride = NULL) {
		if (frameTimeOverride != NULL) {
			m_frameTimeOvd = m_frameTime;
			m_frameTime = frameTimeOverride;
		}
		if (!clock.isStarted()) {
			clock.start();
		}
		m_maxFrameTick = Sequences[seqID].size() / 2;
		m_subPos = { Sequences[seqID][m_frameTick * 2] * m_frameSize.x, Sequences[seqID][m_frameTick * 2 + 1] * m_frameSize.y, m_frameSize.x, m_frameSize.y };
		
		if (clock.getTicks() >= m_frameTime) {
			m_frameTick += 1;
			clock.stop();
			clock.start();
		}

		if (m_frameTick == m_maxFrameTick) {
			m_frameTick = 0;
		}

		if (flipHorix) {
			SDL_RenderCopyExF(renderer, texImg, &m_subPos, &m_pos, 0, NULL, SDL_FLIP_HORIZONTAL);
		}
		else {
			SDL_RenderCopyF(renderer, texImg, &m_subPos, &m_pos);
		}

		if (frameTimeOverride != NULL) {
			m_frameTime = m_frameTimeOvd;
		}
	}

	

	SDL_FRect m_pos; // The position of the rendererd texture
private:
	SDL_Renderer* renderer;
	SDL_Texture* texImg;
	SDL_Point m_frameSize; // the size of the individual frames.
	SDL_FPoint m_texSize; // The size of the overall texture atlas.
	SDL_Rect m_subPos; // The internal size of the frame texture.

	float m_frameTime;
	float m_frameTimeOvd;
	int m_frameTick = 0;
	int m_maxFrameTick = 0;

	Timer clock;

	std::unordered_map<std::string, std::vector<int>> Sequences;
};

class Player {
public:
	Player(Window* window, std::string texturePath, SDL_Point startPos, float maxSpeed, float acceleration, float friction) : win(window), maxSpeed(maxSpeed), acceleration(acceleration), deacceleration(friction) {
		sprite = new animatedSprite(window->renderer, texturePath, { 32,32 }, 180);
		sprite->m_pos = { (float)startPos.x, (float)startPos.y, 32, 32};

		//Sequences:
		sprite->addSequence("idle_forward", { 0,0, 1,0, 2,0, 3,0, 4,0, 5,0 });
		sprite->addSequence("idle_right", { 0,1, 1,1, 2,1, 3,1, 4,1, 5,1 });
		sprite->addSequence("idle_back", { 0,2, 1,2, 2,2, 3,2, 4,2, 5,2 });

	}

	SDL_FPoint getPos() {
		return SDL_FPoint{ sprite->m_pos.x, sprite->m_pos.y };
	}

	/*
	void Update() {
		SDL_Point movement{ 0, 0 };

		// Handle key inputs for movement direction
		if (win->Keys[SDL_SCANCODE_W] || win->Keys[SDL_SCANCODE_UP]) {
			movement.y -= 1; // Move up (NORTH)
		}
		if (win->Keys[SDL_SCANCODE_A] || win->Keys[SDL_SCANCODE_LEFT]) {
			movement.x -= 1; // Move left (WEST)
		}
		if (win->Keys[SDL_SCANCODE_S] || win->Keys[SDL_SCANCODE_DOWN]) {
			movement.y += 1; // Move down (SOUTH)
		}
		if (win->Keys[SDL_SCANCODE_D] || win->Keys[SDL_SCANCODE_RIGHT]) {
			movement.x += 1; // Move right (EAST)
		}

		// Normalize the movement vector to prevent faster diagonal movement
		if (movement.x != 0 || movement.y != 0) {
			float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
			if (length != 0) {
				movement.x /= length;  // Normalize X
				movement.y /= length;  // Normalize Y
			}
		}

		// Get the deltaTime in seconds (assuming it's in milliseconds)
		float deltaTimeInSeconds = win->deltaTime / 1000.0f;

		// Calculate the target velocity based on movement direction
		float targetVelocityX = movement.x * speed;
		float targetVelocityY = movement.y * speed;

		// Smooth acceleration towards target velocity
		if (movement.x != 0 || movement.y != 0) {
			velocity.x = Lerp(velocity.x, targetVelocityX, accelFactor * deltaTimeInSeconds);
			velocity.y = Lerp(velocity.y, targetVelocityY, accelFactor * deltaTimeInSeconds);
		}
		else {
			// Decelerate smoothly when no keys are pressed
			velocity.x = Lerp(velocity.x, 0, deaccelFactor * deltaTimeInSeconds);
			velocity.y = Lerp(velocity.y, 0, deaccelFactor * deltaTimeInSeconds);
		}

		// Update the player position based on velocity
		sprite->m_pos.x += velocity.x;
		sprite->m_pos.y += velocity.y;

		// Convert to integer positions for rendering (snap to nearest pixel)
		SDL_Rect renderPos = { static_cast<int>(sprite->m_pos.x), static_cast<int>(sprite->m_pos.y), sprite->m_pos.w, sprite->m_pos.h };

		// Update the direction the player is facing based on movement
		if (movement.y < 0) {
			lookingAt = NORTH;
		}
		if (movement.y > 0) {
			lookingAt = SOUTH;
		}
		if (movement.x < 0) {
			lookingAt = WEST;
		}
		if (movement.x > 0) {
			lookingAt = EAST;
		}

		std::cout << "Velocity X: " << velocity.x << ", Velocity Y: " << velocity.y << std::endl;

		if (velocity.x > 0) {
			std::ofstream logFile("velocity_log.txt", std::ios::app);
			if (logFile.is_open()) {
				logFile << "Timestamp: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << " |-| " << "Velocity X: " << velocity.x << ", Velocity Y: " << velocity.y << std::endl;
				logFile.close();
			}
			else {
				std::cout << "faield to open log file" << std::endl;
			}
		}

		// Render the sprite based on direction
		switch (lookingAt) {
		case NORTH:
			sprite->Render("idle_back");
			break;
		case EAST:
			sprite->Render("idle_right");
			break;
		case SOUTH:
			sprite->Render("idle_forward");
			break;
		case WEST:
			sprite->Render("idle_right", true);
			break;
		}
	}

	*/

	void Update() {

		double dt = win->deltaTime / 1000;

		if (win->Keys[SDL_SCANCODE_W] || win->Keys[SDL_SCANCODE_UP]) {
			velocity.y -= acceleration * dt;
			lookingAt = NORTH;
		}
		if (win->Keys[SDL_SCANCODE_A] || win->Keys[SDL_SCANCODE_LEFT]) {
			velocity.x -= acceleration * dt;
			lookingAt = WEST;
		}
		if (win->Keys[SDL_SCANCODE_S] || win->Keys[SDL_SCANCODE_DOWN]) {
			velocity.y += acceleration * dt;
			lookingAt = SOUTH;
		}
		if (win->Keys[SDL_SCANCODE_D] || win->Keys[SDL_SCANCODE_RIGHT]) {
			velocity.x += acceleration * dt;
			lookingAt = EAST;
		}

		//deacceleration on no input
		if (velocity.y > 0) {
			velocity.y -= deacceleration * dt;
			if (velocity.y < 0) velocity.y = 0; // clamp to 0
		}
		else if (velocity.y < 0) {
			velocity.y += deacceleration * dt;
			if (velocity.y > 0) velocity.y = 0;
		}

		if (velocity.x > 0) {
			velocity.x -= deacceleration * dt;
			if (velocity.x < 0) velocity.x = 0;
		}
		else if (velocity.x < 0) {
			velocity.x += deacceleration * dt;
			if (velocity.x > 0) velocity.x = 0;
		}

		//appply max speed limit
		if (velocity.x > maxSpeed) velocity.x = maxSpeed;
		if (velocity.x < -maxSpeed) velocity.x = -maxSpeed;
		if (velocity.y > maxSpeed) velocity.y = maxSpeed;
		if (velocity.y < -maxSpeed) velocity.y = -maxSpeed;

		//Update player pos absed on velocity vector:
		sprite->m_pos.x += velocity.x * dt;
		sprite->m_pos.y += velocity.y * dt;

		m_Render();
	}

private:
	Window* win;
	animatedSprite* sprite;
	Direction lookingAt = NORTH;

	SDL_FPoint velocity = { 0, 0 };
	float acceleration = 0.0f;
	float deacceleration = 0.0f; //(friction)
	float maxSpeed = 0.0f;

	void m_Render() {
		if (lookingAt == NORTH) {
			sprite->Render("idle_back");
		}
		if (lookingAt == SOUTH) {
			sprite->Render("idle_forward");
		}
		if (lookingAt == EAST) {
			sprite->Render("idle_right");
		}
		if (lookingAt == WEST) {
			sprite->Render("idle_right", true);
		}
	}
};

#endif
