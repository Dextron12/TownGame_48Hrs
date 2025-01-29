#ifndef MAPREADER_HPP
#define MAPREADER_HPP

#include <Player.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cctype>
#include <regex> //Note: There are faster regular expression libs out in the wild tahn regex. Loading maps with regex will have a noticable delay.

std::string find_strip(std::string findStr, std::string operStr) {
	size_t tPos = operStr.find(findStr);
	if (tPos != std::string::npos) {
		// Remove the part of the string that matches findStr
		return operStr.substr(0, tPos) + operStr.substr(tPos + findStr.length());
	}
	else {
		std::printf("%s could not be found in %s\n", findStr.c_str(), operStr.c_str());
		return operStr;  // return original string if not found
	}
}

std::string trimWhitespace(const std::string& str) {
	size_t first = str.find_first_not_of(" \t"); //Fins first non-whitespace char
	if (first == std::string::npos) return ""; //If no non-whitespace, return
	size_t last = str.find_last_not_of(" \t"); //Find last non-whitespace char
	return str.substr(first, last - first + 1);
}

struct Tile {
	std::string textureName;
	int textureID;

	bool usingTextureAtlas;
	bool isEntity; //If the tile is an entity, it will become a rigidBody
	SDL_Point pos;
};

class Map {
public:
	Map(std::string pathToMap, std::string breakTileOnChar) {
		//Attempt to open map:
		std::ifstream mapFile(pathToMap.c_str());
		if (!mapFile.is_open()) {
			std::printf("Failed to open the map: %s.\n Aborting Map Creation...\n", pathToMap.c_str());
			return;
		}

		std::stringstream buffer;
		buffer << mapFile.rdbuf();
		std::string fileContents = buffer.str();

		//Splits the file into respective lines:
		std::istringstream lineStream(fileContents);
		std::string line;

		while (std::getline(lineStream, line)) {
			//Trim leading and trailing whitepaces:
			std::string trimmedLine = trimWhitespace(line);
			
			if (!trimmedLine.empty()) {
				if (trimmedLine[0] != '#') {
					//Gets the tile size for the map.
					if (trimmedLine.find("Tile Size:") != std::string::npos) {
						//std::cout << trimmedLine << std::endl;
						std::string tileSize = find_strip("Tile Size: ", trimmedLine);
						size_t tPos = tileSize.find('x');
						if (tPos != std::string::npos) {
							std::string widthStr = tileSize.substr(0, tPos);
							std::string heightStr = tileSize.substr(tPos + 1);
							m_tileSize.x = std::stoi(widthStr);
							m_tileSize.y = std::stoi(heightStr);
						}
					}
					else {
						//Only raw tile rows should be left ;(
						std::vector<std::string> rawTiles;
						size_t startPos = 0;
						size_t endPos = trimmedLine.find("), ");
						while (endPos != std::string::npos) {
							rawTiles.push_back(trimmedLine.substr(startPos, endPos + 2 - startPos));

							//Move start pos for next search:
							startPos = endPos + 3; //Skips apst the "), " delim
							endPos = line.find("), ", startPos); //Finds next delim
						}
						//Adds last tile
						rawTiles.push_back(trimmedLine.substr(startPos));

						std::string rwTile;
						for (auto& tile : rawTiles) {
							//std::cout << tile << std::endl;
							//Identify the tile types here:

							///Special Tile Filters here
							if (tile.find("(entity):") != std::string::npos) {
								//Tile is an entity:
								std::regex entityWAtlas(R"re(^\(entity\):\s*(\w+)->\((\d+)\)\((\d+),(\d+),(\d+x\d+)\)$)re"); // Entity pattern for entties using a texture atlas
								std::regex entityTexture(R"re(^\(entity\):\s*"([^"]+)"\((\d+),(\d+)\)\?$)re");					//Entity pattern without a texture atlas, instead uses double qoutes to sshow the texture name
								std::smatch matches;

								//Entity wiht a textureAtlas
								if (std::regex_search(tile, matches, entityWAtlas)) {
									//Create a new entity witha  texture atlas.
									Tile new_tile;
									new_tile.isEntity = true;
									new_tile.usingTextureAtlas = true;
									new_tile.textureID = std::stoi(matches[2]); //Internal textureID.
									new_tile.textureName = matches[1]; //Texture Atlas name
									new_tile.pos.x = std::stoi(matches[3]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[4]) * m_tileSize.y;
									//matches[5], matches[6] -> width & height of internal texture.
									tileMap.push_back(new_tile);
									continue;
								}
								//Entity without a textureAtlas.
								if (std::regex_search(tile, matches, entityTexture)) {
									Tile new_tile;
									new_tile.isEntity = true;
									new_tile.usingTextureAtlas = false;
									new_tile.textureName = matches[1];
									new_tile.pos.x = std::stoi(matches[2]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[3]) * m_tileSize.y;
									tileMap.push_back(new_tile);
									continue;
								}
							}
							else {
								//All other tiles filtered here:

								std::regex tileWAtlas(R"((\w+)->\((\d+)\)\((\d+),(\d+)\))");
								std::smatch matches;

								//A standard tile with a textureAtlas.
								if (std::regex_search(tile, matches, tileWAtlas)) {
									Tile new_tile;
									new_tile.isEntity = false;
									new_tile.usingTextureAtlas = true;
									new_tile.textureName = matches[1];
									new_tile.textureID = std::stoi(matches[2]);
									new_tile.pos.x = std::stoi(matches[3]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[4]) * m_tileSize.y;
									tileMap.push_back(new_tile);
									continue;
								}

								//A standard tile with a standard texture (no textureAtlas)
								std::regex tilePattern(R"(^(\w+)\((\d+),\s*(\d+)\),?\s*$)");
								if (std::regex_search(tile, matches, tilePattern)) {
									Tile new_tile;
									new_tile.isEntity = false;
									new_tile.usingTextureAtlas = false;
									new_tile.textureName = matches[1];
									new_tile.pos.x = std::stoi(matches[2]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[3]) * m_tileSize.y;
									tileMap.push_back(new_tile);
									continue;
								}
								else {
									std::cout << "This tile does not match any known pattern: " << tile << std::endl;
								}
							}

							/*
							//If the tile is using a textureAtlas:
							if (tile.find("->") != std::string::npos) {
								std::regex pattern(R"((\w+)->\((\d+)\)\((\d+),(\d+)\))");
								std::smatch matches;

								// Search for the pattern in the input string
								if (std::regex_search(tile, matches, pattern)) {
									//Extract the components usign the captured groups
									Tile new_tile;
									new_tile.textureName = matches[1];	//the atlasID
									new_tile.textureID = std::stoi(matches[2]);	//The internal textureID
									new_tile.pos.x = std::stoi(matches[3]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[4]) * m_tileSize.x; //pos of tile
									new_tile.usingTextureAtlas = true;
									tileMap.push_back(new_tile);
								}
								else {
									std::cout << tile << " does not match the texture atlas pattern" << std::endl;
								}
							}
							else if (tile.find("(entity): ") != std::string::npos) {
								std::regex pattern(R"re(^(\w+):\s*"([^"]+)"\((\d+),(\d+)\)$)re");
								std::smatch matches;
								std::cout << tile << std::endl;
								if (std::regex_search(tile, matches, pattern)) {
									Tile new_tile;
									new_tile.textureName = matches[2];
									new_tile.pos = { std::stoi(matches[3]) * m_tileSize.x, std::stoi(matches[4]) * m_tileSize.x };

									new_tile.isEntity = true;
									new_tile.usingTextureAtlas = false; // Need to add further code, to account for an entity that uses a texture atlas.
									tileMap.push_back(new_tile);
								}
								else {
									std::cout << "Entity: " << tile << " does not match the filter" << std::endl;
								}
							}
							else {
								std::regex pattern(R"(^(\w+)\((\d+),\s*(\d+)\),?\s*$)");
								std::smatch matches;

								if (std::regex_match(tile, matches, pattern)) {
									Tile new_tile;
									new_tile.textureName = matches[1]; // Any string before the paranthesis
									new_tile.pos.x = std::stoi(matches[2]) * m_tileSize.x; new_tile.pos.y = std::stoi(matches[3]) * m_tileSize.y;

									new_tile.isEntity = false;
									new_tile.usingTextureAtlas = false;
									tileMap.push_back(new_tile);
								}
								else {
									std::cout << "This tile has no filter: " << tile << std::endl;
								}
							}*/
						}
					}
				}
			}
		}
		mapFile.close();
	}

	std::vector<Tile> tileMap;

private:
	std::string breakChar; // The string that tells the reader when to stop reading that tile. So if the tile input in the map ends in '),', the break on char should be that.

	int errorCode = 0;
	SDL_Point m_tileSize;
};

#endif