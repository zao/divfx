#include "D3D.hpp"
#include "Util.hpp"

#include <d3dcompiler.h>
#include <directxtk/DDSTextureLoader.h>
#include <fmt/format.h>

#include <memory>

struct DirectoryIncluder : ID3DInclude {
    explicit DirectoryIncluder(std::filesystem::path root) : root(root) {}

    HRESULT __stdcall Open(D3D_INCLUDE_TYPE type, LPCSTR fileName, LPCVOID parentData, LPCVOID *outData,
                           UINT *outSize) override {
        auto candidatePath = root / fileName;
        if (!exists(candidatePath)) {
            return E_FAIL;
        }
        auto text = SlurpTextFile(candidatePath);
        auto cch = text.size();
        auto buf = std::make_unique<char[]>(cch + 1);
        memcpy(buf.get(), text.c_str(), cch + 1);
        *outData = buf.release();
        *outSize = cch;
        return S_OK;
    }

    HRESULT __stdcall Close(LPCVOID data) override {
        std::unique_ptr<char[]> _((char *)data);
        return S_OK;
    }

    std::filesystem::path root;
};

DivFxCompiler::DivFxCompiler(std::filesystem::path assetRoot, std::string prelude)
    : assetRoot_(assetRoot), prelude_(prelude), includer_(std::make_shared<DirectoryIncluder>(assetRoot)) {
    if (prelude_.back() != '\n') {
        prelude += "\n";
    }
    std::string draw2dFragment = SlurpTextFile(assetRoot / "Shaders/Draw2D.hlsl");
    std::string vsSource = prelude_ + draw2dFragment;

    CComPtr<ID3DBlob> errors;

    HRESULT hr{S_OK};
    DWORD flags = 0;
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0;
    hr = D3DCompile(std::data(vsSource), std::size(vsSource), nullptr, nullptr, includer_.get(), "VShad", "vs_5_0",
                    flags, 0, &vsBytecode_, &errors);
    if (SUCCEEDED(hr)) {
        fmt::print("Shader compilation success.\n");
    } else {
        fmt::print("Shader compilation failure: {}", hr);
        std::string msgs((char const *)errors->GetBufferPointer(), errors->GetBufferSize());
        fmt::print("===\n{}===\n", msgs);
    }
}

DivFx DivFxCompiler::Compile(std::string psFragment, std::string psEntrypoint) {
    DivFx ret;
    HRESULT hr{S_OK};
    CComPtr<ID3DBlob> errors;

    std::string psSource = prelude_ + psFragment;

    errors = {};
    DWORD flags = 0;
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_OPTIMIZATION_LEVEL0;
    hr = D3DCompile(std::data(psSource), std::size(psSource), nullptr, nullptr, includer_.get(), psEntrypoint.c_str(),
                    "ps_5_0", flags, 0, &ret.psBytecode, &errors);
    if (SUCCEEDED(hr)) {
        fmt::print("Shader compilation success.\n");
    } else {
        fmt::print("Shader compilation failure: {}", hr);
        std::string msgs((char const *)errors->GetBufferPointer(), errors->GetBufferSize());
        fmt::print("===\n{}===\n", msgs);
    }

    return ret;
}

CComPtr<ID3DBlob> DivFxCompiler::VSBytecode() const { return vsBytecode_; }

Dx::LoadTextureResult Dx::LoadTexture(std::filesystem::path const &path, bool viewAsSrgb) {
    if (auto I = textures.find(path); I != textures.end()) {
        return I->second;
    }
    CComPtr<ID3D11Resource> resource;
    CComPtr<ID3D11ShaderResourceView> srv;
    for (auto &root : resourceRoots_) {
        auto finalPath = root / path;
        if (exists(finalPath)) {
            DirectX::DDS_LOADER_FLAGS loadFlags =
                viewAsSrgb ? DirectX::DDS_LOADER_FORCE_SRGB : DirectX::DDS_LOADER_DEFAULT;
            HRESULT hr = DirectX::CreateDDSTextureFromFileEx(dev, ctx, finalPath.generic_wstring().c_str(), 0,
                                                             D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                                                             loadFlags, &resource, &srv);
            if (SUCCEEDED(hr)) {
                textures[path] = {resource, srv};
                break;
            }
        }
    }
    return {resource, srv};
}

void Dx::AddResourceRoot(std::filesystem::path const &path) {
    if (std::find(resourceRoots_.begin(), resourceRoots_.end(), path) == resourceRoots_.end()) {
        resourceRoots_.push_back(path);
    }
}

void Dx::BuildSamplers() {
    struct SamplerSpec {
        std::string_view name;
        D3D11_FILTER filter;
        D3D11_TEXTURE_ADDRESS_MODE addressU, addressV, addressW;
        std::optional<float> lodBias;
        std::optional<D3D11_COMPARISON_FUNC> comparisonFunc;
    };
    SamplerSpec specs[]{
        {
            "SamplerLinearWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerLinearClamp",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
        },
        {
            "SamplerLinearBorder",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_BORDER,
            D3D11_TEXTURE_ADDRESS_BORDER,
            D3D11_TEXTURE_ADDRESS_BORDER,
        },
        {
            "SamplerPointWrap",
            D3D11_FILTER_MIN_MAG_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerPointClamp",
            D3D11_FILTER_MIN_MAG_MIP_POINT,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
        },
        {
            "SamplerLinearWrapClampWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerLinearWrapBorderWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_BORDER,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerLinearClampWrapWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerLinearBorderWrapWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_BORDER,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            "SamplerDynamicWrap",
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_WRAP,
        },
        {
            .name = "SamplerLinearWrapNoBias",
            .filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            .addressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .addressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .addressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .lodBias = 0.0f,
        },
        {
            .name = "SamplerLinearClampNoBias",
            .filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            .addressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .addressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .addressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .lodBias = 0.0f,
        },
        {
            .name = "SamplerPointWrapNoBias",
            .filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .addressU = D3D11_TEXTURE_ADDRESS_WRAP,
            .addressV = D3D11_TEXTURE_ADDRESS_WRAP,
            .addressW = D3D11_TEXTURE_ADDRESS_WRAP,
            .lodBias = 0.0f,
        },
        {
            .name = "SamplerPointClampNoBias",
            .filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
            .addressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .addressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .addressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .lodBias = 0.0f,
        },
        {
            .name = "SamplerDepth",
            .filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            .addressU = D3D11_TEXTURE_ADDRESS_BORDER,
            .addressV = D3D11_TEXTURE_ADDRESS_BORDER,
            .addressW = D3D11_TEXTURE_ADDRESS_BORDER,
            .lodBias{},
            .comparisonFunc = D3D11_COMPARISON_LESS_EQUAL,
        },
    };

    samplers.clear();
    samplerStorage.clear();
    samplerNames.clear();
    for (auto &spec : specs) {
        D3D11_SAMPLER_DESC sd{
            .Filter = spec.filter,
            .AddressU = spec.addressU,
            .AddressV = spec.addressV,
            .AddressW = spec.addressW,
            .MipLODBias = spec.lodBias.value_or(-0.5f),
            .MaxAnisotropy = 1,
            .ComparisonFunc = spec.comparisonFunc.value_or(D3D11_COMPARISON_NEVER),
            .BorderColor = {1.0f, 1.0f, 1.0f, 1.0f},
            .MinLOD = 0,
            .MaxLOD = +FLT_MAX,
        };
        CComPtr<ID3D11SamplerState> sampler;
        HRESULT hr = dev->CreateSamplerState(&sd, &sampler);
        samplers.push_back(sampler.p);
        samplerStorage.push_back(sampler);
        samplerNames.push_back((std::string)spec.name);
    }
}