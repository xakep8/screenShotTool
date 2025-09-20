// screenShot.cpp : Defines the entry point for the application.
//

#include "screenShot.h"
#include "Duplication.h"
#include <iostream>
#include <ShlObj.h>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char* argv[])
{
	PWSTR documentsPath = NULL;
	std::string path_str;
	if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &documentsPath))) {
		std::wcout << L"[CAPTURER] Documents folder path: " << documentsPath << L"\n";

		// Create the full path with proper concatenation
		std::wstring savePath = std::wstring(documentsPath) + L"\\screenshot.png";
		CoTaskMemFree(documentsPath);

		// Convert from wide string to regular string for stbi_write_png
		
		path_str.resize(savePath.length());
		std::wcstombs(&path_str[0], savePath.c_str(), path_str.size());
	} else {
		std::cerr << "[CAPTURER] Failed to get Documents folder path\n";
	}
	Duplication Capturer;
	if (!Capturer.init()) {
		std::cerr << "[CAPTURER] error starting capturer\n";
		//CoUninitialize();
		return -1;
	}
	
	// Get actual dimensions from m_dxgiOutDesc
	INT width = Capturer.GetWidth();
	INT height = Capturer.GetHeight();
	INT regionWidth = 200;
	INT regionHeight = 200;
	
	if (width <= 0 || height <= 0) {
		std::cerr << "[CAPTURER] Invalid dimensions: " << width << "x" << height << "\n";
		return -1;
	}
	
	std::cout << "[CAPTURER] Capturing screen with resolution: " << width << "x" << height << "\n";
	
	// Add before capture attempt
	std::cout << "[CAPTURER] Waiting for frame..." << std::endl;
	Sleep(1000); // Wait 1 second before capturing
	
	INT bufferSize = regionWidth * regionHeight * 4;
	std::unique_ptr<unsigned char[]> buffer(new unsigned char[bufferSize]);
	
	if (Capturer.CaptureRegion(0,0, regionWidth, regionHeight, buffer.get(), bufferSize)) {
		// Check if buffer contains data
		bool hasData = false;
		for (int i = 0; i < 100 && !hasData; i++) {
			if (buffer[i] != 0) {
				hasData = true;
			}
		}
		
		if (!hasData) {
			std::cerr << "[CAPTURER] Warning: Buffer appears to be empty\n";
		}
		
		INT channels = 4;
		std::unique_ptr<unsigned char[]> convertedBuffer(new unsigned char[bufferSize]);

		// Convert BGRA to RGBA (swap B and R channels)
		for (int i = 0; i < regionWidth * regionHeight; i++) {
			convertedBuffer[i * 4 + 0] = buffer[i * 4 + 2]; // R (from B)
			convertedBuffer[i * 4 + 1] = buffer[i * 4 + 1]; // G (unchanged)
			convertedBuffer[i * 4 + 2] = buffer[i * 4 + 0]; // B (from R)
			convertedBuffer[i * 4 + 3] = buffer[i * 4 + 3]; // A (unchanged)
		}
		INT result = stbi_write_png(path_str.c_str(), regionWidth, regionHeight, channels, convertedBuffer.get(), regionWidth * channels);
		if (result) {
			std::cout << "[CAPTURER] screenshot saved to screenshot.png\n";
		}
		else {
			std::cerr << "[CAPTURER] error saving screenshot\n";
		}
	} else {
		std::cerr << "[CAPTURER] Failed to capture image\n";
	}
	
	std::cin >> std::ws;
}
