#define GLFW_EXPOSE_NATIVE_WIN32

#include "Cards.hpp"
#include "D3D.hpp"
#include "Util.hpp"

#include <fmt/format.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <span>

#include <shellscalingapi.h>

int main(int argc, char *argv[]) {
    std::filesystem::path assetRoot = R"(F:\Temp\poe\contents-3.20.1b)";
    std::filesystem::path preludeRoot = R"(F:\Temp\poe\prelude)";

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    GLFWwindow *wnd = glfwCreateWindow(eCardWidth * 3, eCardHeight * 3, "DivFX", nullptr, nullptr);
    HWND hWnd = glfwGetWin32Window(wnd);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(wnd, &fbWidth, &fbHeight);

    CComPtr<ID3D11BlendState> divBlend;
    CComPtr<ID3D11RasterizerState> divRaster;

    int cardWidth = 526;
    int cardHeight = 378;

    Dx dx;
    dx.AddResourceRoot(assetRoot);
    dx.AddResourceRoot(preludeRoot);

    HRESULT hr = S_OK;
    {
        D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_0};
        DXGI_SWAP_CHAIN_DESC scd{
            .BufferDesc{
                .Width{(UINT)fbWidth},
                .Height{(UINT)fbHeight},
                .RefreshRate{0, 1},
                .Format{DXGI_FORMAT_B8G8R8A8_UNORM_SRGB},
            },
            .SampleDesc{1, 0},
            .BufferUsage{DXGI_USAGE_RENDER_TARGET_OUTPUT},
            .BufferCount{2},
            .OutputWindow{hWnd},
            .Windowed{TRUE},
            .SwapEffect{DXGI_SWAP_EFFECT_DISCARD},
        };
        UINT flags = D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, std::data(featureLevel),
                                           std::size(featureLevel), D3D11_SDK_VERSION, &scd, &dx.swapChain, &dx.dev,
                                           &dx.featureLevel, &dx.ctx);

        dx.BuildSamplers();

        CComPtr<ID3D11Texture2D> animTex, animStageTex;
        D3D11_TEXTURE2D_DESC td{
            .Width = (UINT)cardWidth,
            .Height = (UINT)cardHeight,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            .SampleDesc = {1, 0},
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_RENDER_TARGET,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
        };
        hr = dx.dev->CreateTexture2D(&td, nullptr, &animTex);

        td.Usage = D3D11_USAGE_STAGING;
        td.BindFlags = 0;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        hr = dx.dev->CreateTexture2D(&td, nullptr, &animStageTex);

        CComPtr<ID3D11RenderTargetView> animRtv;
        hr = dx.dev->CreateRenderTargetView(animTex, nullptr, &animRtv);

        CComPtr<ID3D11Resource> backBuffer;
        dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        dx.dev->CreateRenderTargetView(backBuffer, nullptr, &dx.bbRtv);
    }

    std::string dxPrelude = SlurpTextFile(preludeRoot / "dx11_prelude.inc");
    std::string aeFragment = SlurpTextFile(assetRoot / "Shaders/AtlasEffects.hlsl");
    std::string draw2dFragment = SlurpTextFile(assetRoot / "Shaders/Draw2D.hlsl");

    std::map<std::string, DivFx> divEffects;
    DivFxCompiler divFxCompiler(assetRoot, dxPrelude);
    auto compileAe = std::bind(&DivFxCompiler::Compile, &divFxCompiler, aeFragment, std::placeholders::_1);
    divEffects["crusader"] = compileAe("PShad_CrusaderBackgroundDivEffect");
    divEffects["redeemer"] = compileAe("PShad_EyrieBackgroundDivEffect");
    divEffects["hunter"] = compileAe("PShad_BasiliskBackgroundDivEffect");
    divEffects["warlord"] = compileAe("PShad_ConquerorBackgroundDivEffect");
    divEffects["shaper"] = divFxCompiler.Compile(draw2dFragment, "PShad_Tentacles");
    divEffects["elder"] = divFxCompiler.Compile(draw2dFragment, "PShad_Tentacles");

    std::map<std::string, CComPtr<ID3DBlob>> divPsBytecode;
    std::map<std::string, CComPtr<ID3D11PixelShader>> divPs;
    for (auto &[name, dfx] : divEffects) {
        CComPtr<ID3D11PixelShader> ps;
        hr = dx.dev->CreatePixelShader(dfx.psBytecode->GetBufferPointer(), dfx.psBytecode->GetBufferSize(), nullptr,
                                       &ps);
        divPs[name] = ps;
        divPsBytecode[name] = dfx.psBytecode;
    }

    Draw2DVariant shaper{.name = "shaper", .ps = divPs["shaper"], .psBytecode = divPsBytecode["shaper"]};
    Draw2DVariant elder{.name = "elder", .ps = divPs["elder"], .psBytecode = divPsBytecode["elder"]};
    AtlasEffectsVariant crusader{.name = "crusader", .ps = divPs["crusader"], .psBytecode = divPsBytecode["crusader"]};
    AtlasEffectsVariant hunter{.name = "hunter", .ps = divPs["hunter"], .psBytecode = divPsBytecode["hunter"]};
    AtlasEffectsVariant redeemer{.name = "redeemer", .ps = divPs["redeemer"], .psBytecode = divPsBytecode["redeemer"]};
    AtlasEffectsVariant warlord{.name = "warlord", .ps = divPs["warlord"], .psBytecode = divPsBytecode["warlord"]};

    std::vector<std::shared_ptr<CardLayer>> atlasCards{
        std::make_shared<Draw2DLayer>(dx, divFxCompiler, shaper),
        std::make_shared<Draw2DLayer>(dx, divFxCompiler, elder),
        std::make_shared<AtlasEffectsLayer>(dx, divFxCompiler, crusader),
        std::make_shared<AtlasEffectsLayer>(dx, divFxCompiler, hunter),
        std::make_shared<AtlasEffectsLayer>(dx, divFxCompiler, redeemer),
        std::make_shared<AtlasEffectsLayer>(dx, divFxCompiler, warlord),
    };

    static int atlasCardIndex = 0, atlasCardCount = std::size(atlasCards);
    glfwSetKeyCallback(wnd, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_RELEASE) {
            atlasCardIndex = (atlasCardIndex + 1) % atlasCardCount;
        } else if (key == GLFW_KEY_PAGE_UP && action == GLFW_RELEASE) {
            atlasCardIndex = (atlasCardIndex + atlasCardCount - 1) % atlasCardCount;
        }
    });

    while (!glfwWindowShouldClose(wnd)) {
        glfwPollEvents();

        double now = glfwGetTime();

        auto atlasCard = atlasCards[atlasCardIndex];
        atlasCard->SetViewTransform(glm::mat4{2.0f / fbWidth, 0, 0, 0,   //
                                              0, -2.0f / fbHeight, 0, 0, //
                                              0, 0, 1, 0,                //
                                              -1, 1, 0, 1});
        atlasCard->SetTime(now);

        float clearColor[4]{0.0f, 0.0f, 0.0f, 0.0f};
        dx.ctx->ClearRenderTargetView(dx.bbRtv, clearColor);

        dx.ctx->OMSetRenderTargets(1, &dx.bbRtv.p, nullptr);

        D3D11_VIEWPORT viewport{.TopLeftX = 0,
                                .TopLeftY = 0,
                                .Width = (float)fbWidth,
                                .Height = (float)fbHeight,
                                .MinDepth = 0.0f,
                                .MaxDepth = 1.0f};

        dx.ctx->RSSetState(divRaster);
        dx.ctx->RSSetViewports(1, &viewport);

        D3D11_RECT scissor{.left = 0, .top = 0, .right = fbWidth, .bottom = fbHeight};
        dx.ctx->RSSetScissorRects(1, &scissor);

        atlasCard->Draw({0, 0}, {cardWidth, cardHeight});

        dx.swapChain->Present(1, 0);
    }
    glfwTerminate();
}