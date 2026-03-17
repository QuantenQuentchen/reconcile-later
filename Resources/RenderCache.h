//
// Created by quanti on 13.03.26.
//

#pragma once
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <Texture.hpp>
#include <vector>
#include <expected>
#include <unordered_map>

#define NANOSVG_IMPLEMENTATION
#include "../external/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "../external/nanosvgrast.h"

#include "../Constants/RenderConstants.h"

namespace Cache
{
    using TextureID = uint16_t;
    using ShaderID = uint16_t;
    using MaterialID = uint64_t;
    struct TextureCache {
        std::vector<RTexture> textures;  // flat, cache-friendly

        TextureID load(const std::filesystem::path& path, const int targetWidth = 0, const int targetHeight = 0) {
            if (path.extension() == ".svg")
            {
                std::error_code ec;
                std::cout << "[SVG] Requested load" << std::endl;
                std::cout << "  path(raw): " << path << std::endl;
                std::cout << "  path(absolute): " << std::filesystem::absolute(path, ec) << std::endl;
                if (ec) std::cout << "  absolute path error: " << ec.message() << std::endl;
                std::cout << "  extension: " << path.extension() << std::endl;
                std::cout << "  target size: " << targetWidth << "x" << targetHeight << std::endl;

                const Image svgImage = LoadImageSVG(path, targetWidth, targetHeight);
                if (svgImage.data == nullptr || svgImage.width <= 0 || svgImage.height <= 0) {
                    std::cout << "[SVG] LoadImageSVG returned an invalid image" << std::endl;
                } else {
                    std::cout << "[SVG] Rasterized image ready: " << svgImage.width << "x" << svgImage.height << std::endl;
                }

                textures.emplace_back(svgImage);
            }
            else
            {
                std::cout << "Loading Texture: " << path << std::endl;
                textures.emplace_back(LoadTexture(path.string().c_str()));
            }
            return static_cast<TextureID>(textures.size() - 1);
        }

        [[nodiscard]] const RTexture& get(const TextureID id) const { return textures[id]; }
    private:
        static std::vector<char> ReadFileToBuffer(const std::filesystem::path& path) {
            std::error_code ec;
            const auto absolutePath = std::filesystem::absolute(path, ec);
            if (ec) {
                std::cout << "[SVG] Failed to compute absolute path for " << path << ": " << ec.message() << std::endl;
                ec.clear();
            }

            const bool exists = std::filesystem::exists(path, ec);
            if (ec) {
                std::cout << "[SVG] exists() failed for " << path << ": " << ec.message() << std::endl;
                ec.clear();
            }

            const bool regularFile = std::filesystem::is_regular_file(path, ec);
            if (ec) {
                std::cout << "[SVG] is_regular_file() failed for " << path << ": " << ec.message() << std::endl;
                ec.clear();
            }

            const auto fileSizeOnDisk = std::filesystem::file_size(path, ec);
            if (ec) {
                std::cout << "[SVG] file_size() failed for " << path << ": " << ec.message() << std::endl;
                ec.clear();
            }

            std::cout << "[SVG] File preflight" << std::endl;
            std::cout << "  path(raw): " << path << std::endl;
            std::cout << "  path(absolute): " << absolutePath << std::endl;
            std::cout << "  exists: " << exists << std::endl;
            std::cout << "  regular_file: " << regularFile << std::endl;
            std::cout << "  file_size(bytes): " << fileSizeOnDisk << std::endl;

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                std::cout << "[SVG] Failed to open file: " << path
                          << " errno=" << errno << " (" << std::strerror(errno) << ")"
                          << " rdstate=" << file.rdstate() << std::endl;
                return {};
            }

            const auto fileSize = file.tellg();
            if (fileSize < 0) {
                std::cout << "[SVG] tellg() returned negative size for: " << path
                          << " rdstate=" << file.rdstate() << std::endl;
                return {};
            }

            std::vector<char> buffer(static_cast<size_t>(fileSize) + 1, '\0');
            file.seekg(0, std::ios::beg);
            if (!file) {
                std::cout << "[SVG] seekg() failed for: " << path
                          << " rdstate=" << file.rdstate() << std::endl;
                return {};
            }

            if (fileSize > 0) {
                file.read(buffer.data(), fileSize);
                if (!file) {
                    std::cout << "[SVG] read() failed for: " << path
                              << " requested=" << fileSize
                              << " read=" << file.gcount()
                              << " rdstate=" << file.rdstate() << std::endl;
                    return {};
                }
            }

            std::cout << "[SVG] Read success: " << fileSize << " bytes" << std::endl;
            return buffer;
        }

        static Image LoadImageSVG(const std::filesystem::path& filePath, const int targetWidth, const int targetHeight) {
            Image image = {};

            // Parse from in-memory SVG bytes so Windows wide paths are handled by std::filesystem streams.
            auto svgBuffer = ReadFileToBuffer(filePath);
            if (svgBuffer.empty()) {
                std::cout << "[SVG] Aborting load because input buffer is empty for: " << filePath << std::endl;
                return image;
            }

            NSVGimage *svg = nsvgParse(svgBuffer.data(), "px", 96.0f);
            if (svg == nullptr) {
                std::cout << "[SVG] nsvgParse() failed for: " << filePath
                          << " (invalid SVG data or unsupported content)" << std::endl;
                return image;
            }

            std::cout << "[SVG] Parsed SVG dimensions: " << svg->width << "x" << svg->height << std::endl;
            if (svg->width <= 0.0f || svg->height <= 0.0f) {
                std::cout << "[SVG] Invalid SVG dimensions after parse: " << svg->width << "x" << svg->height << std::endl;
                nsvgDelete(svg);
                return image;
            }

            const int width = (targetWidth > 0) ? targetWidth : static_cast<int>(std::ceil(svg->width));
            const int height = (targetHeight > 0) ? targetHeight : static_cast<int>(std::ceil(svg->height));
            if (width <= 0 || height <= 0) {
                std::cout << "[SVG] Computed invalid raster target size: " << width << "x" << height
                          << " (requested " << targetWidth << "x" << targetHeight << ")" << std::endl;
                nsvgDelete(svg);
                return image;
            }

            NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
            if (rasterizer == nullptr) {
                std::cout << "[SVG] nsvgCreateRasterizer() failed for: " << filePath << std::endl;
                nsvgDelete(svg);
                return image;
            }

            auto *pixels = static_cast<unsigned char *>(std::malloc(static_cast<size_t>(width) * height * 4));
            if (pixels == nullptr) {
                std::cout << "[SVG] Pixel buffer allocation failed for size: " << width << "x" << height
                          << " (" << static_cast<size_t>(width) * height * 4 << " bytes)" << std::endl;
                nsvgDeleteRasterizer(rasterizer);
                nsvgDelete(svg);
                return image;
            }

            const auto widthF = static_cast<float>(width);
            const auto heightF = static_cast<float>(height);
            const float scale = std::min(widthF / svg->width, heightF / svg->height);
            if (!(scale > 0.0f) || !std::isfinite(scale)) {
                std::cout << "[SVG] Invalid raster scale: " << scale
                          << " widthF=" << widthF << " heightF=" << heightF
                          << " svg=" << svg->width << "x" << svg->height << std::endl;
                std::free(pixels);
                nsvgDeleteRasterizer(rasterizer);
                nsvgDelete(svg);
                return image;
            }

            const float tx = (widthF - svg->width * scale) * 0.5f;
            const float ty = (heightF - svg->height * scale) * 0.5f;
            std::cout << "[SVG] Rasterize params: scale=" << scale << " tx=" << tx << " ty=" << ty
                      << " output=" << width << "x" << height << std::endl;

            nsvgRasterize(rasterizer, svg, tx, ty, scale, pixels, width, height, width * 4);

            image.data = pixels;
            image.width = width;
            image.height = height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

            std::cout << "[SVG] Rasterization completed successfully for: " << filePath << std::endl;

            nsvgDeleteRasterizer(rasterizer);
            nsvgDelete(svg);
            return image;
        }
    };

    struct Material
    {
        BlendMode blendMode;
        RenderPass renderPass;
        TextureID texture;
        ShaderID shader;
    };

    struct ShaderCache
    {
        std::vector<Shader> shaders{
            LoadShader(nullptr, nullptr) // default shader
        };
        std::unordered_map<std::filesystem::path, ShaderID> shaderMap;

        ShaderID get(const std::filesystem::path& path) {
            const auto it = shaderMap.find(path);
            if (it != shaderMap.end()) return it->second;

            shaders.emplace_back(LoadShader(path.string().c_str(), nullptr));
            const auto id = static_cast<ShaderID>(shaders.size() - 1);
            shaderMap[path] = id;
            return id;
        }

        static ShaderID get()
        {
            return 0; //No shader
        }

        const Shader& get(const ShaderID id) const { return shaders[id]; }

    };

    struct MaterialCache
    {
        enum class Error
        {
            NOT_FOUND
        };
        std::vector<Material> materials;
        std::unordered_map<std::string, MaterialID> materialMap;

        std::expected<MaterialID, Error> get(const std::filesystem::path& path) {
            const auto key = path.string();
            const auto it = materialMap.find(key);
            if (it != materialMap.end()) return it->second;
            return std::unexpected(Error::NOT_FOUND);
        }

        MaterialID getOrEmplace(const std::string& path, const Material& material) {
            const auto it = materialMap.find(path);
            if (it != materialMap.end()) return it->second;
            materials.push_back(material);
            const auto id = materials.size() - 1;
            materialMap[path] = id;
            return id;
        }

        const Material& get(const MaterialID id) const { return materials[id]; }

    };

}
