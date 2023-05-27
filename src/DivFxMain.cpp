#define GLFW_EXPOSE_NATIVE_WIN32

#include "Cards.hpp"
#include "D3D.hpp"
#include "Util.hpp"

#include <DirectXTex.h>
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
#include <wincodec.h>

struct CardLayers {
    explicit CardLayers(Dx &dx, std::string dxPrelude, std::filesystem::path assetRoot) : dx_(dx) {
        std::string aeFragment = SlurpTextFile(assetRoot / "Shaders/AtlasEffects.hlsl");
        std::string draw2dFragment = SlurpTextFile(assetRoot / "Shaders/Draw2D.hlsl");

        std::map<std::string, DivFx> divEffects;
        DivFxCompiler divFxCompiler(assetRoot, dxPrelude);
        auto compileAe = std::bind(&DivFxCompiler::Compile, &divFxCompiler, aeFragment, std::placeholders::_1);
        divEffects["shaper"] = divFxCompiler.Compile(draw2dFragment, "PShad_Tentacles");
        divEffects["elder"] = divFxCompiler.Compile(draw2dFragment, "PShad_Tentacles");
        divEffects["crusader"] = compileAe("PShad_CrusaderBackgroundDivEffect");
        divEffects["redeemer"] = compileAe("PShad_EyrieBackgroundDivEffect");
        divEffects["hunter"] = compileAe("PShad_BasiliskBackgroundDivEffect");
        divEffects["warlord"] = compileAe("PShad_ConquerorBackgroundDivEffect");

        std::map<std::string, CComPtr<ID3DBlob>> divPsBytecode;
        std::map<std::string, CComPtr<ID3D11PixelShader>> divPs;
        for (auto &[name, dfx] : divEffects) {
            CComPtr<ID3D11PixelShader> ps;
            HRESULT hr = dx.dev->CreatePixelShader(dfx.psBytecode->GetBufferPointer(), dfx.psBytecode->GetBufferSize(),
                                                   nullptr, &ps);
            divPs[name] = ps;
            divPsBytecode[name] = dfx.psBytecode;
        }

        for (auto &name : {"shaper", "elder"}) {
            Draw2DVariant var{.name = name, .ps = divPs[name], .psBytecode = divPsBytecode[name]};
            atlasCards_.push_back(std::make_shared<Draw2DLayer>(dx, divFxCompiler, var));
            names_.push_back(name);
        }
        for (auto &name : {"crusader", "redeemer", "hunter", "warlord"}) {
            AtlasEffectsVariant var{.name = name, .ps = divPs[name], .psBytecode = divPsBytecode[name]};
            atlasCards_.push_back(std::make_shared<AtlasEffectsLayer>(dx, divFxCompiler, var));
            names_.push_back(name);
        }
    }

    Dx &dx_;

    std::vector<std::shared_ptr<CardLayer>> atlasCards_;
    std::vector<std::string> names_;
};

struct App {
    virtual ~App() {}

    virtual void Run(CardLayers const &cardLayers) {}

    glm::mat4 UiMatrix(glm::ivec2 uiSize) {
        return glm::mat4{{2.0f / uiSize.x, 0, 0, 0},  //
                         {0, -2.0f / uiSize.y, 0, 0}, //
                         {0, 0, 1, 0},                //
                         {-1, 1, 0, 1}};
    }
};

struct InteractiveState : App {
    explicit InteractiveState(Dx &dx, glm::ivec2 fbSize) : dx_(dx), fbSize_(fbSize) {
        SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
        wnd_ = glfwCreateWindow(fbSize.x, fbSize.y, "DivFX", nullptr, nullptr);
        HWND hWnd = glfwGetWin32Window(wnd_);

        HRESULT hr = S_OK;
        {
            D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_0};
            DXGI_SWAP_CHAIN_DESC scd{
                .BufferDesc{
                    .Width{(UINT)fbSize.x},
                    .Height{(UINT)fbSize.y},
                    .RefreshRate{0, 1},
                    .Format{DXGI_FORMAT_R8G8B8A8_UNORM_SRGB},
                },
                .SampleDesc{1, 0},
                .BufferUsage{DXGI_USAGE_RENDER_TARGET_OUTPUT},
                .BufferCount{2},
                .OutputWindow{hWnd},
                .Windowed{TRUE},
                .SwapEffect{DXGI_SWAP_EFFECT_DISCARD},
            };
            UINT flags = D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                               std::data(featureLevel), std::size(featureLevel), D3D11_SDK_VERSION,
                                               &scd, &dx.swapChain, &dx.dev, &dx.featureLevel, &dx.ctx);

            dx.BuildSamplers();

            CComPtr<ID3D11Resource> backBuffer;
            dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
            dx.dev->CreateRenderTargetView(backBuffer, nullptr, &dx.bbRtv);
        }

        glfwSetWindowUserPointer(wnd_, this);

        glfwSetKeyCallback(wnd_, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            auto &self = *(InteractiveState *)glfwGetWindowUserPointer(window);
            self.KeyCallback(window, key, scancode, action, mods);
        });
    }

    void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
        if (atlasCardCount_ > 0) {
            if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_RELEASE) {
                atlasCardIndex_ = (atlasCardIndex_ + 1) % atlasCardCount_;
            } else if (key == GLFW_KEY_PAGE_UP && action == GLFW_RELEASE) {
                atlasCardIndex_ = (atlasCardIndex_ + atlasCardCount_ - 1) % atlasCardCount_;
            }
        }
    }

    ~InteractiveState() { glfwTerminate(); }

    void Run(CardLayers const &cardLayers) override {
        while (!glfwWindowShouldClose(wnd_)) {
            glfwPollEvents();

            double now = glfwGetTime();

            float clearColor[4]{0.0f, 0.0f, 0.0f, 0.0f};
            dx_.ctx->ClearRenderTargetView(dx_.bbRtv, clearColor);

            dx_.ctx->OMSetRenderTargets(1, &dx_.bbRtv.p, nullptr);

            D3D11_VIEWPORT viewport{.TopLeftX = 0,
                                    .TopLeftY = 0,
                                    .Width = (float)fbSize_.x,
                                    .Height = (float)fbSize_.y,
                                    .MinDepth = 0.0f,
                                    .MaxDepth = 1.0f};

            dx_.ctx->RSSetViewports(1, &viewport);

            D3D11_RECT scissor{.left = 0, .top = 0, .right = fbSize_.x, .bottom = fbSize_.y};
            dx_.ctx->RSSetScissorRects(1, &scissor);

            auto &atlasCards = cardLayers.atlasCards_;

            for (int cardIdx = 0; cardIdx < std::size(atlasCards); ++cardIdx) {
                auto atlasCard = atlasCards[cardIdx];
                atlasCard->SetViewTransform(UiMatrix(fbSize_));
                atlasCard->SetTime(now);

                int col = cardIdx % 3;
                int row = cardIdx / 3;
                auto coord = glm::vec2(col, row);
                auto relPos = coord - glm::vec2(2 / 2.0f, 1 / 2.0f);
                auto halfCard = glm::vec2(eCardWidth, eCardHeight) / 2.0f;
                auto cardSpacing = glm::vec2((float)eCardWidth + 20.0f, (float)eCardHeight + 20.0f);
                auto screenMid = glm::vec2(fbSize_) / 2.0f;
                auto cardMid = screenMid + relPos * cardSpacing;
                auto cardOrigin = cardMid - halfCard;
                atlasCard->Draw(cardOrigin, {eCardWidth, eCardHeight});
            }

            dx_.swapChain->Present(1, 0);
        }
    }

    Dx &dx_;

    GLFWwindow *wnd_;
    int atlasCardIndex_{}, atlasCardCount_{};
    glm::ivec2 fbSize_;
};

struct BatchState : App {
    explicit BatchState(Dx &dx, glm::ivec2 animSize, std::filesystem::path exportRoot)
        : dx_(dx), animSize_(animSize), exportRoot_(exportRoot) {
        HRESULT hr = S_OK;
        D3D_FEATURE_LEVEL featureLevel[] = {D3D_FEATURE_LEVEL_11_0};
        UINT flags = D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, std::data(featureLevel),
                               std::size(featureLevel), D3D11_SDK_VERSION, &dx.dev, &dx.featureLevel, &dx.ctx);

        dx.BuildSamplers();

        D3D11_TEXTURE2D_DESC td{
            .Width = (UINT)animSize.x,
            .Height = (UINT)animSize.y,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .SampleDesc = {1, 0},
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_RENDER_TARGET,
            .CPUAccessFlags = 0,
            .MiscFlags = 0,
        };
        hr = dx.dev->CreateTexture2D(&td, nullptr, &animTex_);

        td.Usage = D3D11_USAGE_STAGING;
        td.BindFlags = 0;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        hr = dx.dev->CreateTexture2D(&td, nullptr, &animStageTex_);

        hr = dx.dev->CreateRenderTargetView(animTex_, nullptr, &animRtv_);

        std::error_code ec{};
        create_directories(exportRoot_, ec);
    }

    void Run(CardLayers const &cardLayers) override {
        HRESULT hr{S_OK};

        D3D11_VIEWPORT viewport{.TopLeftX = 0,
                                .TopLeftY = 0,
                                .Width = (float)animSize_.x,
                                .Height = (float)animSize_.y,
                                .MinDepth = 0.0f,
                                .MaxDepth = 1.0f};

        dx_.ctx->RSSetViewports(1, &viewport);

        D3D11_RECT scissor{.left = 0, .top = 0, .right = animSize_.x, .bottom = animSize_.y};
        dx_.ctx->RSSetScissorRects(1, &scissor);

        std::vector<std::vector<int>> specs{{0, 1}, {0, 4}, {0}, {1}, {2}, {3}, {4}, {5}};
        // for (int cardIdx = 0; cardIdx < cardLayers.atlasCards_.size(); ++cardIdx) {
        // auto card = cardLayers.atlasCards_[cardIdx];
        // auto cardName = cardLayers.names_[cardIdx];
        for (auto layerSpec : specs) {
            std::vector<std::shared_ptr<CardLayer>> layers;
            std::string compositeName = "div_bg";
            for (auto layerSource : layerSpec) {
                auto card = cardLayers.atlasCards_[layerSource];
                card->SetViewTransform(UiMatrix(animSize_));
                compositeName = fmt::format("{}_{}", compositeName, layerSource);
                layers.push_back(card);
            }

            float baseTime = 0.0f;
            int numFrames = 300;
            float fps = 60.0f;
            int lerpFrames = 60;

            for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
                auto captureAt = [&](DirectX::ScratchImage &img, int frame) {
                    float clearColor[]{0.0f, 0.0f, 0.0f, 1.0f};
                    dx_.ctx->ClearRenderTargetView(animRtv_, clearColor);
                    dx_.ctx->OMSetRenderTargets(1, &animRtv_.p, nullptr);

                    for (auto layer : layers) {
                        layer->SetTime(baseTime + (float)frame / fps);
                        layer->Draw({0, 0}, animSize_);
                    }

                    hr = DirectX::CaptureTexture(dx_.dev, dx_.ctx, animTex_, img);
                };
                DirectX::ScratchImage frame;
                captureAt(frame, frameIdx);
                int oldFrameIdx = frameIdx - numFrames;
                if (oldFrameIdx >= -lerpFrames) {
                    DirectX::ScratchImage oldFrame;
                    captureAt(oldFrame, oldFrameIdx);
                    float lerpFactor = (oldFrameIdx + lerpFrames + 1.0f) / (lerpFrames + 1.0f);
                    auto *newImg = frame.GetImage(0, 0, 0);
                    auto *oldImg = oldFrame.GetImage(0, 0, 0);
                    for (int i = 0; i < newImg->slicePitch; ++i) {
                        newImg->pixels[i] = (uint8_t)glm::mix<float>(newImg->pixels[i], oldImg->pixels[i], lerpFactor);
                    }
                }

                auto animPath = exportRoot_ / fmt::format("{}-{:04d}.png", compositeName, frameIdx);
                DirectX::DDS_FLAGS exportFlags = DirectX::DDS_FLAGS_NONE;
                auto img = frame.GetImage(0, 0, 0);
                hr = DirectX::SaveToWICFile(*img, DirectX::WIC_FLAGS_NONE, DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG),
                                            animPath.generic_wstring().c_str());
            }
        }
    }

    Dx &dx_;

    glm::ivec2 animSize_;
    std::filesystem::path exportRoot_;

    CComPtr<ID3D11Texture2D> animTex_, animStageTex_;
    CComPtr<ID3D11RenderTargetView> animRtv_;
};

int main(int argc, char *argv[]) {
    std::filesystem::path assetRoot = R"(F:\Temp\poe\contents-3.20.1b)";
    std::filesystem::path preludeRoot = R"(F:\Temp\poe\prelude)";

    CoInitialize(nullptr);

    bool interactive = false;
    glm::ivec2 fbSize{1920, 1080};
    glm::ivec2 animSize{eCardWidth, eCardHeight};
    std::filesystem::path exportRoot = R"(F:\Temp\poe\div-export\raw)";

    Dx dx;
    dx.AddResourceRoot(assetRoot);
    dx.AddResourceRoot(preludeRoot);

    std::unique_ptr<App> app;

    if (interactive) {
        app = std::make_unique<InteractiveState>(dx, fbSize);
    } else {
        app = std::make_unique<BatchState>(dx, animSize, exportRoot);
    }

    std::string dxPrelude = SlurpTextFile(preludeRoot / "dx11_prelude.inc");
    CardLayers cardLayers(dx, dxPrelude, assetRoot);

    app->Run(cardLayers);
}