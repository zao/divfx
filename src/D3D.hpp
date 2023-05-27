#pragma once

#include <d3d11.h>
#include <dxgi.h>

#include <atlbase.h>
#include <atlcom.h>

#include <filesystem>
#include <string>
#include <unordered_map>

struct Dx {
    CComPtr<ID3D11Device> dev;
    CComPtr<ID3D11DeviceContext> ctx;
    CComPtr<IDXGISwapChain> swapChain;
    CComPtr<ID3D11RenderTargetView> bbRtv;
    D3D_FEATURE_LEVEL featureLevel{};
    std::vector<ID3D11SamplerState *> samplers;
    std::vector<CComPtr<ID3D11SamplerState>> samplerStorage;
    std::vector<std::string> samplerNames;

    struct LoadTextureResult {
        CComPtr<ID3D11Resource> resource;
        CComPtr<ID3D11ShaderResourceView> srv;
    };

    std::unordered_map<std::filesystem::path, LoadTextureResult> textures;
    std::vector<std::filesystem::path> resourceRoots_;

    LoadTextureResult LoadTexture(std::filesystem::path const &path, bool viewAsSrgb = false);

    void AddResourceRoot(std::filesystem::path const &path);

    void BuildSamplers();
};

struct DivFx {
    CComPtr<ID3DBlob> vsBytecode, psBytecode;
};

struct DirectoryIncluder;

struct DivFxCompiler {
    DivFxCompiler(std::filesystem::path assetRoot, std::string prelude);

    DivFx Compile(std::string psFragment, std::string psEntrypoint);

    CComPtr<ID3DBlob> VSBytecode() const;

  private:
    std::filesystem::path assetRoot_;
    std::string prelude_;
    CComPtr<ID3DBlob> vsBytecode_;
    std::shared_ptr<DirectoryIncluder> includer_;
};

CComPtr<ID3D11ShaderResourceView> LoadDDS(Dx &dx, std::filesystem::path const &path);