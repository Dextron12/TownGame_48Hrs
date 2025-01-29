
#include <iostream>

#include "window.hpp"
#include "Player.hpp"
#include "mapReader.hpp"

FileSystem* fs;

void createFlatMap() {
	int gridWidth = 50;
	int gridHeight = 38;
	int tileSize = 16;

	std::ofstream outFile(fs->joinToExecDir("Assets\\Maps\\default.map"));
	if (!outFile) {
		std::cerr << "Error opening file for writing" << std::endl;
		return;
	}

	outFile << "# All lines begining with # will be ignored.\n#Map For TownGame\n" << std::endl;
	outFile << "Tile Size: 16x16" << std::endl;
	
	for (int y = 0; y < gridHeight; y++) {
		for (int x = 0; x < gridWidth; x++) {
			outFile << "grass(" << x << ", " << y << ")";
			if (x < gridWidth - 1) {
				outFile << ", ";
			}
		}
		//Adds new line at end of each row:
		outFile << std::endl;
	}
	outFile.close();
	std::cout << "New map written to: " << fs->joinToExecDir("Assets\\Maps\\default.map");
}

int main(int argc, char* argv[]) {
	Window window("Town Game 48hrs Challenge", 800, 600, 0);
	fs = &window.fs;

	//createFlatMap();
	
	Player player(&window, fs->joinToExecDir("Assets\\Textures\\Player\\Player_Old\\Player.png"), { 400,300 },
		200.0f, 500.0f, 300.0f);

	TextureAtlas grass(window.renderer, fs->joinToExecDir("Assets\\Textures\\Grass\\Grass_Tiles_1.png"), { 16,16 });
	grass.autoGenerateTextures(80);

	TextureAtlas spruceTree(window.renderer, fs->joinToExecDir("Assets\\Textures\\Trees\\Spruce_Tree_Small.png"), { 96,48 });
	spruceTree.autoGenerateTextures(30);

	SDL_Point texSize;
	SDL_Texture* grass_middle = loadTexture(window.renderer, fs->joinToExecDir("Assets\\Textures\\Grass\\Grass_1_Middle.png"), &texSize.x, &texSize.y);

	Map map(fs->joinToExecDir("Assets\\Maps\\default.map"), "),");

	SDL_Rect destTest = { 200,200,96,48 };

	std::cout << spruceTree.render_Pos.x << ", " << spruceTree.render_Pos.y << std::endl;

	while (window.appState) {
		SDL_SetRenderDrawColor(window.renderer, 100, 149, 237, 255);
		SDL_RenderClear(window.renderer);

		for (const auto& tile : map.tileMap) {
			if (tile.usingTextureAtlas) {
				if (tile.textureName == "grass") {
					//Grass Atlas:
					grass.useSubTexture(tile.textureID, window.renderer, tile.pos);
				}
				if (tile.textureName == "spruceTree_small") {
					spruceTree.useSubTexture(tile.textureID, window.renderer, tile.pos);
				}
			} else if (tile.isEntity) {
				//render entities here:
			}
			else {
				SDL_Rect pos{ tile.pos.x, tile.pos.y, texSize.x, texSize.y };
				SDL_RenderCopy(window.renderer, grass_middle, NULL, &pos);
			}
		}

		SDL_RenderCopy(window.renderer, spruceTree.atlas, NULL, &destTest);

		player.Update();
		
		window.update();
		SDL_RenderPresent(window.renderer);

		if (window.Keys[SDL_SCANCODE_ESCAPE]) {
			window.appState = false;
		}
	}
	return 0;
}