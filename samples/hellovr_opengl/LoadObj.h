// ObjLoader.cpp : Definiert den Einstiegspunkt f��ie Konsolenanwendung.
#include "Vectors.h"

#include <iostream>
#include <fstream>

#include <string>
#include <sstream>
#include <vector>
#include <iterator>

#include <vector>

void LoadObj(const char* path, std::vector<Vector4> * vertices, std::vector<Vector2> * textureCoords, std::vector<Vector3> * triangles, std::vector<Vector3> * textureIndexTriplet);
void HandleVertexLine(std::vector<Vector4> * vertices);
void HandleTextureCoordLine(std::vector<Vector2> * textureCoords);
void HandleFaceLine(std::vector<Vector3> * triangles, std::vector<Vector3> * trianglesTxtrIndices);
std::vector<std::string> SplitStringAt(const std::string &s, char delim);

std::vector<std::string> splittetLine = std::vector<std::string>();
std::vector<std::string> faceIndices = std::vector<std::string>();
std::vector<std::string> txtrIndices = std::vector<std::string>();

void LoadObj(const char* path, std::vector<Vector4> * vertices, std::vector<Vector2> * textureCoords, std::vector<Vector3> * triangles, std::vector<Vector3> * trianglesTxtrIndices) {
	std::ifstream file(path);
	std::string str;
	std::cout << "File output: " << std::endl;

	while (std::getline(file, str)) {
		std::cout << str << std::endl;

		splittetLine = SplitStringAt(str, ' ');

		//delete empty strings 
		for (unsigned i = 0; i < splittetLine.size(); i++) {
			if (splittetLine[i] == "") splittetLine.erase(splittetLine.begin() + i);
		}

		std::string lineType = splittetLine[0];
		splittetLine.erase(splittetLine.begin());

		if (lineType == "v") {
			HandleVertexLine(vertices);
		}
		else if (lineType == "vt") {
			HandleTextureCoordLine(textureCoords);
		}
		else if (lineType == "f") {
			HandleFaceLine(triangles, trianglesTxtrIndices);
		}

	}
}

void HandleVertexLine(std::vector<Vector4> * vertices) {
	//get vertices and write them as triplets in the vertices with a last 0 in the end

	if (splittetLine.size() == 3) {
		float v1 = std::stof(splittetLine[0]),
			v2 = std::stof(splittetLine[1]),
			v3 = std::stof(splittetLine[2]);
		vertices->push_back(Vector4(v1, v2, v3, 0.0));
	}
}

void HandleTextureCoordLine(std::vector<Vector2> * textureCoords) {
	if (splittetLine.size() == 2) {
		float vt1 = std::stof(splittetLine[0]),
			vt2 = std::stof(splittetLine[1]);
		textureCoords->push_back(Vector2(vt1, vt2));
	}
}

void HandleFaceLine(std::vector<Vector3> * triangles, std::vector<Vector3> * trianglesTxtrIndices) {
	faceIndices.clear();
	txtrIndices.clear();

	for (unsigned i = 1; i < splittetLine.size(); i++) {
		std::vector<std::string> indexTriplet = SplitStringAt(splittetLine[i], '/');
		faceIndices.push_back(indexTriplet[0]);
		txtrIndices.push_back(indexTriplet[1]);
	}

	switch (faceIndices.size()) {
	case 3: //push back one triangle if only 3 vertex indices are provided
		triangles->push_back(
			Vector3(std::stof(faceIndices[0]) - 1,
				std::stof(faceIndices[1]) - 1,
				std::stof(faceIndices[2]) - 1)
		);
		trianglesTxtrIndices->push_back(
			Vector3(std::stof(txtrIndices[0]) - 1,
				std::stof(txtrIndices[1]) - 1,
				std::stof(txtrIndices[2]) - 1)
		);
		break;
	case 4: // split a quadruplet of vertex indices into two triangles
		triangles->push_back(
			Vector3(std::stof(faceIndices[0]) - 1,
				std::stof(faceIndices[1]) - 1,
				std::stof(faceIndices[2]) - 1)
		);
		triangles->push_back(
			Vector3(std::stof(faceIndices[0]) - 1,
				std::stof(faceIndices[2]) - 1,
				std::stof(faceIndices[3]) - 1)
		);
		trianglesTxtrIndices->push_back(
			Vector3(std::stof(txtrIndices[0]) - 1,
				std::stof(txtrIndices[1]) - 1,
				std::stof(txtrIndices[2]) - 1)
		);
		trianglesTxtrIndices->push_back(
			Vector3(std::stof(txtrIndices[0]) - 1,
				std::stof(txtrIndices[2]) - 1,
				std::stof(txtrIndices[3]) - 1)
		);
		break;
	default: // don't push if more indices are found
		break;
	}
}

template<typename Out>
void split(const std::string &s, char delim, Out result) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}

std::vector<std::string> SplitStringAt(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}


