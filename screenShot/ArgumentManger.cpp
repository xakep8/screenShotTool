#include "ArgumentManager.h"
#include <iostream>
#include <string>

namespace ArgumentManager {
	int captureWidth = 200;  // Default width
	int captureHeight = 200; // Default height
	std::string savePath = "screenshot.png"; // Default save path
	void parseArguments(int argc, char* argv[]) {
		for (int i = 1; i < argc; ++i) {
			std::string arg = argv[i];
			if (arg == "--width" && i + 1 < argc) {
				captureWidth = std::stoi(argv[++i]);
			} else if (arg == "--height" && i + 1 < argc) {
				captureHeight = std::stoi(argv[++i]);
			} else if (arg == "--output" && i + 1 < argc) {
				savePath = argv[++i];
			} else {
				std::cerr << "Unknown argument: " << arg << "\n";
			}
		}
		// Validate dimensions
		if (captureWidth <= 0 || captureHeight <= 0) {
			std::cerr << "Invalid dimensions specified. Using default 200x200.\n";
			captureWidth = 200;
			captureHeight = 200;
		}
		std::cout << "Capture Width: " << captureWidth << "\n";
		std::cout << "Capture Height: " << captureHeight << "\n";
		std::cout << "Save Path: " << savePath << "\n";
	}
} // namespace ArgumentManager