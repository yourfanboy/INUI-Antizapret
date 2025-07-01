#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Для простоты используем библиотеку stb_image
// https://github.com/nothings/stb/blob/master/stb_image.h
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "stb_image.h"

void ConvertToRaw(const std::string& inputFile, const std::string& outputFile) {
    int width, height, channels;
    
    // Загружаем изображение и принудительно конвертируем в RGBA
    unsigned char* data = stbi_load(inputFile.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cerr << "Failed to load image: " << inputFile << std::endl;
        std::cerr << "Reason: " << stbi_failure_reason() << std::endl;
        return;
    }
    
    std::cout << "Loaded image:" << std::endl;
    std::cout << "  Width: " << width << std::endl;
    std::cout << "  Height: " << height << std::endl;
    std::cout << "  Original channels: " << channels << std::endl;
    
    // Сохраняем в RAW формат
    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to create output file: " << outputFile << std::endl;
        stbi_image_free(data);
        return;
    }
    
    // Записываем размеры
    outFile.write(reinterpret_cast<char*>(&width), sizeof(width));
    outFile.write(reinterpret_cast<char*>(&height), sizeof(height));
    
    // Записываем данные изображения (уже в формате RGBA)
    size_t dataSize = width * height * 4;
    outFile.write(reinterpret_cast<char*>(data), dataSize);
    
    outFile.close();
    stbi_image_free(data);
    
    std::cout << "Successfully converted to RAW format: " << outputFile << std::endl;
    std::cout << "Total size: " << (dataSize + 8) << " bytes" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <input_image> <output.raw>" << std::endl;
        std::cout << "Example: " << argv[0] << " screenshot.png custom_screenshot.raw" << std::endl;
        std::cout << std::endl;
        std::cout << "Supported formats: JPG, PNG, BMP" << std::endl;
        std::cout << std::endl;
        std::cout << "To use with the DLL, copy the output file to:" << std::endl;
        std::cout << "C:\\hook_logs\\custom_screenshot.raw" << std::endl;
        return 1;
    }
    
    ConvertToRaw(argv[1], argv[2]);
    return 0;
} 