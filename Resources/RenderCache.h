//
// Created by quanti on 13.03.26.
//

#pragma once
#include <cstdint>
#include <filesystem>
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
                textures.emplace_back(LoadImageSVG(path.string().c_str(), targetWidth, targetHeight));
            else
                textures.emplace_back(LoadTexture(path.string().c_str()));
            return static_cast<TextureID>(textures.size() - 1);
        }

        [[nodiscard]] const RTexture& get(const TextureID id) const { return textures[id]; }
    private:
        static Image LoadImageSVG(const char *filePath, const int targetWidth, const int targetHeight) {
            Image image = {};

            NSVGimage *svg = nsvgParseFromFile(filePath, "px", 96.0f);
            if (svg == nullptr) return image;

            const int width = (targetWidth > 0) ? targetWidth : static_cast<int>(std::ceil(svg->width));
            const int height = (targetHeight > 0) ? targetHeight : static_cast<int>(std::ceil(svg->height));
            if (width <= 0 || height <= 0) {
                nsvgDelete(svg);
                return image;
            }

            NSVGrasterizer *rasterizer = nsvgCreateRasterizer();
            if (rasterizer == nullptr) {
                nsvgDelete(svg);
                return image;
            }

            auto *pixels = static_cast<unsigned char *>(std::malloc(static_cast<size_t>(width) * height * 4));
            if (pixels == nullptr) {
                nsvgDeleteRasterizer(rasterizer);
                nsvgDelete(svg);
                return image;
            }

            const auto widthF = static_cast<float>(width);
            const auto heightF = static_cast<float>(height);
            const float scale = std::min(widthF / svg->width, heightF / svg->height);
            const float tx = (widthF - svg->width * scale) * 0.5f;
            const float ty = (heightF - svg->height * scale) * 0.5f;

            nsvgRasterize(rasterizer, svg, tx, ty, scale, pixels, width, height, width * 4);

            image.data = pixels;
            image.width = width;
            image.height = height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

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
