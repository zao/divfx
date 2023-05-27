#include "Cards.hpp"

#include <d3dcompiler.h>
#include <glm/gtc/type_ptr.hpp>

#include <map>
#include <span>
#include <string>

struct UiVertex {
    glm::vec2 pos;
    glm::vec2 uv;
    glm::u8vec4 color;
    glm::vec4 satScaleLocalUv;
};

CardLayer::CardLayer(Dx &dx, DivFxCompiler const &divFxCompiler) : dx_(dx), vsCbCpu_{} {
    HRESULT hr{S_OK};
    D3D11_INPUT_ELEMENT_DESC ieds[]{
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(UiVertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(UiVertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, offsetof(UiVertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(UiVertex, satScaleLocalUv),
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    {
        auto bytecode = divFxCompiler.VSBytecode();
        hr = dx.dev->CreateInputLayout(std::data(ieds), std::size(ieds), bytecode->GetBufferPointer(),
                                       bytecode->GetBufferSize(), &il_);

        hr = dx.dev->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), nullptr, &vs_);

        {
            D3D11_BUFFER_DESC vbd{
                .ByteWidth = sizeof(UiVertex) * 4,
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            };
            hr = dx.dev->CreateBuffer(&vbd, nullptr, &vb_);

            uint16_t indices[]{0, 1, 2, 1, 3, 2};
            D3D11_BUFFER_DESC ibd{
                .ByteWidth = std::span(indices).size_bytes(),
                .Usage = D3D11_USAGE_IMMUTABLE,
                .BindFlags = D3D11_BIND_INDEX_BUFFER,
            };
            D3D11_SUBRESOURCE_DATA isrd{std::data(indices)};
            hr = dx.dev->CreateBuffer(&ibd, &isrd, &ib_);
        }

        {
            D3D11_BUFFER_DESC cbd{
                .ByteWidth = sizeof(VsCbData),
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            };
            hr = dx.dev->CreateBuffer(&cbd, nullptr, &vsCb_);
        }
    }

    {
        D3D11_BLEND_DESC bd{
            .AlphaToCoverageEnable = FALSE,
            .IndependentBlendEnable = FALSE,
            .RenderTarget =
                {
                    {
                        .BlendEnable = TRUE,
                        .SrcBlend = D3D11_BLEND_ONE,
                        .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
                        .BlendOp = D3D11_BLEND_OP_ADD,
                        .SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA,
                        .DestBlendAlpha = D3D11_BLEND_ONE,
                        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                    },
                    {
                        .BlendEnable = FALSE,
                        .SrcBlend = D3D11_BLEND_ONE,
                        .DestBlend = D3D11_BLEND_ZERO,
                        .BlendOp = D3D11_BLEND_OP_ADD,
                        .SrcBlendAlpha = D3D11_BLEND_ONE,
                        .DestBlendAlpha = D3D11_BLEND_ZERO,
                        .BlendOpAlpha = D3D11_BLEND_OP_ADD,
                        .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
                    },
                },
        };
        for (int i = 2; i < 8; ++i) {
            bd.RenderTarget[i] = bd.RenderTarget[1];
        }
        dx.dev->CreateBlendState(&bd, &divBlend_);
    }

    {
        D3D11_RASTERIZER_DESC rd{
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .FrontCounterClockwise = FALSE,
            .DepthBias = 0,
            .DepthBiasClamp = 0.0f,
            .SlopeScaledDepthBias = 0.0f,
            .DepthClipEnable = TRUE,
            .ScissorEnable = TRUE,
            .MultisampleEnable = FALSE,
            .AntialiasedLineEnable = FALSE,
        };
        dx.dev->CreateRasterizerState(&rd, &divRaster_);
    }
}

void CardLayer::SetViewTransform(glm::mat4 transform) {
    vsCbCpu_.uiScale = glm::transpose(transform);
    vsCbDirty_ = true;
}

void CardLayer::Draw(glm::ivec2 pos, glm::ivec2 size) {
    auto &ctx = dx_.ctx;

    {
        UiVertex verts[4]{};
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {
                auto &v = verts[col + row * 2];
                glm::vec2 fpos = pos, fsize = size;
                v.pos = glm::mix(fpos, fpos + fsize, glm::vec2{col, row});
                v.uv = glm::vec2(col, row);
                v.color = glm::u8vec4(255, 255, 255, 255);
                v.satScaleLocalUv = glm::vec4(1.0f, 1.0f, v.uv);
            }
        }
        dx_.ctx->UpdateSubresource(vb_, 0, nullptr, std::data(verts), 0, 0);
    }

    if (vsCbDirty_) {
        ctx->UpdateSubresource(vsCb_, 0, nullptr, &vsCbCpu_, 0, 0);
        vsCbDirty_ = false;
    }

    ctx->RSSetState(divRaster_);
    ctx->OMSetBlendState(divBlend_, glm::value_ptr(glm::vec4{}), 0xFFFF'FFFF);

    ctx->IASetInputLayout(il_);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->IASetIndexBuffer(ib_, DXGI_FORMAT_R16_UINT, 0);
    UINT vtxStride = sizeof(UiVertex), vtxOffset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb_.p, &vtxStride, &vtxOffset);

    ctx->VSSetShader(vs_, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &vsCb_.p);
}

void CardLayer::SetPsCbData(void const *data, size_t size) {
    HRESULT hr{S_OK};

    if (!psCbDirty_) {
        return;
    }

    D3D11_BUFFER_DESC cbd{
        .ByteWidth = (UINT)size,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
    };
    if (psCb_) {
        psCb_.Release();
    }
    D3D11_SUBRESOURCE_DATA srd{.pSysMem = data};
    hr = dx_.dev->CreateBuffer(&cbd, data ? &srd : nullptr, &psCb_);
    psCbDirty_ = false;
}

void CardLayer::AddTexture(std::string_view name, std::string_view path, bool viewAsSrgb) {
    if (auto slotI = texSlotByName_.find((std::string)name); slotI != texSlotByName_.end()) {
        Dx::LoadTextureResult res;
        CComPtr<ID3D11ShaderResourceView> srv;
        if (!path.empty()) {
            res = dx_.LoadTexture(path, viewAsSrgb);
            srv = res.srv;
        }
        SrvBind bind{.slot = slotI->second, .srv{srv}};
        psSrvs_.push_back(bind);
    }
}

void CardLayer::GenTexture(std::string_view name, glm::ivec2 size, uint32_t value) {
    if (auto slotI = texSlotByName_.find((std::string)name); slotI != texSlotByName_.end()) {
        std::vector<uint32_t> data(size.x * size.y, value);
        UINT pitch = 4 * size.x;
        UINT slicePitch = pitch * size.y;

        CComPtr<ID3D11Texture2D> tex;
        D3D11_TEXTURE2D_DESC td{.Width = (UINT)size.x,
                                .Height = (UINT)size.y,
                                .MipLevels = 0,
                                .ArraySize = 1,
                                .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
                                .SampleDesc = {1, 0},
                                .Usage = D3D11_USAGE_DEFAULT,
                                .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                                .MiscFlags = 0};
        glm::ivec2 shrink = size;
        std::vector<D3D11_SUBRESOURCE_DATA> srds;
        while (true) {
            srds.push_back(
                D3D11_SUBRESOURCE_DATA{.pSysMem = data.data(), .SysMemPitch = pitch, .SysMemSlicePitch = slicePitch});
            if (shrink == glm::ivec2{1, 1}) {
                break;
            }
            shrink = (glm::max)(shrink / 2, {1, 1});
        }
        dx_.dev->CreateTexture2D(&td, srds.data(), &tex);

        CComPtr<ID3D11ShaderResourceView> srv;
        dx_.dev->CreateShaderResourceView(tex, nullptr, &srv);

        SrvBind bind{.slot = slotI->second, .srv{srv}};
        psSrvs_.push_back(bind);
    }
}

void CardLayer::ReflectPixelShader(CComPtr<ID3DBlob> bytecode) {
    CComPtr<ID3D11ShaderReflection> refl;
    HRESULT hr = D3DReflect(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), IID_PPV_ARGS(&refl));
    D3D11_SHADER_DESC desc{};
    hr = refl->GetDesc(&desc);
    for (UINT resIdx = 0; resIdx < desc.BoundResources; ++resIdx) {
        D3D11_SHADER_INPUT_BIND_DESC inputBindDesc{};
        hr = refl->GetResourceBindingDesc(resIdx, &inputBindDesc);
        if (inputBindDesc.Type == D3D_SIT_TEXTURE) {
            texNameBySlot_[inputBindDesc.BindPoint] = inputBindDesc.Name;
            texSlotByName_[inputBindDesc.Name] = inputBindDesc.BindPoint;
        }
    }
}

void CardLayer::ResolveTextureBindings() {
    srvSlots_.clear();
    for (auto &bind : psSrvs_) {
        if (bind.slot >= srvSlots_.size()) {
            srvSlots_.resize(bind.slot + 1);
        }
        srvSlots_[bind.slot] = bind.srv.p;
    }
}

AtlasEffectsLayer::AtlasEffectsLayer(Dx &dx, DivFxCompiler &divFxCompiler, AtlasEffectsVariant const &variant)
    : CardLayer(dx, divFxCompiler), ps_(variant.ps) {
    psCbCpu_ = PsCbData{
        .time = 36.001f,
        .image_width = 390.0f,
        .image_height = 280.0f,
        .progress = 0.0f,
        .alpha = 0.0f,
        .bonus_completed = 0.0f,
        .num_segments = 0,
        .radians_per_second = 0.0f,
        .doubled_memory_line_segments = FALSE,
        .num_memory_line_textures = 0,
        .memory_line_texture_datas{},
        .memory_line_texture_uvs{},
    };

    ReflectPixelShader(variant.psBytecode);

    AddTexture("tex", "Art/2DArt/UIEffects/ConquerorItems/paperBG.dds");
    AddTexture("noise_map", "Art/2DArt/Lookup/atlas_lookup.dds");

    ResolveTextureBindings();
}

void AtlasEffectsLayer::SetTime(double time) {
    psCbCpu_.time = time;
    psCbDirty_ = true;
}

void AtlasEffectsLayer::Draw(glm::ivec2 pos, glm::ivec2 size) {
    CardLayer::Draw(pos, size);
    auto &ctx = dx_.ctx;

    SetPsCbData(&psCbCpu_, sizeof(psCbCpu_));

    ctx->PSSetShader(ps_, nullptr, 0);

    ctx->PSSetShaderResources(0, std::size(srvSlots_), std::data(srvSlots_));
    ctx->PSSetSamplers(0, std::size(dx_.samplers), std::data(dx_.samplers));
    ctx->PSSetConstantBuffers(0, 1, &psCb_.p);
    ctx->DrawIndexed(6, 0, 0);
}

Draw2DLayer::Draw2DLayer(Dx &dx, DivFxCompiler &divFxCompiler, Draw2DVariant const &variant)
    : CardLayer(dx, divFxCompiler), ps_(variant.ps) {
    HRESULT hr{S_OK};
    psCbCpu_ = {
        .time = 23.762f,
        .seed = 0.0f,
        .effect_phase = 0.0f,
        .to_base_resolution_multiplier = 0.0f,
        .has_mask = 1.0f,
        .has_background = 1.0f,
        .is_div_card = 1.0f,
        .aspect_ratio = {1.39286f, 1.0f, 0.0f, 0.0f},
        .tex_scale = {6.96429f, 5.0f, 0.0f, 0.0f},
        .muddle_intensity = {0.1f, 0.1f, 0.0f, 0.0f},
        .tex_clamp{},
        .effect_params{},
        .layers_count = 0.0f,
        .item_size{},
        .flash_glow_color{},
        .hellscape_params{},
        .layer_0_speed = {0.0075f, 0.0025f, 0.0f, 0.0f},
        .layer_1_speed = {0.0225f, 0.0075f, 0.0f, 0.0f},
        .layer_2_speed = {0.03f, 0.01f, 0.0f, 0.0f},
        .layer_3_speed{},
    };

    if (variant.name == "shaper") {
        psCbCpu_.muddle_frequency = 10.0f;
        psCbCpu_.shader_type = 2.0f;
    } else if (variant.name == "elder") {
        psCbCpu_.muddle_frequency = 1.0f;
        psCbCpu_.shader_type = 0.0f;
    }

    ReflectPixelShader(variant.psBytecode);

    AddTexture("tex", "Art/2DArt/UIEffects/ConquerorItems/paperBG.dds", true);
    GenTexture("mask_tex", {1, 1}, 0x7DFFFFFFu); // add 1x1 0x7DFFFFFF texture here
    if (variant.name == "shaper") {
        AddTexture("color_layer_0_tex", "Art/2DArt/Tentacles/ShaperAreaColor1.dds", true);
        AddTexture("color_layer_1_tex", "Art/2DArt/Tentacles/ShaperAreaColor2.dds", true);
        AddTexture("color_layer_2_tex", "Art/2DArt/Tentacles/ShaperAreaColor3.dds", true);
        AddTexture("color_layer_3_tex", {}, true);
        AddTexture("influence_layer_0_tex", "Art/2DArt/Tentacles/ShaperAreaNormal1.dds");
        AddTexture("influence_layer_1_tex", "Art/2DArt/Tentacles/ShaperAreaNormal2.dds");
        AddTexture("influence_layer_2_tex", "Art/2DArt/Tentacles/ShaperAreaNormal3.dds");
        AddTexture("influence_layer_3_tex", {});
        AddTexture("mask_layer_0_tex", {});
        AddTexture("mask_layer_1_tex", {});
        AddTexture("mask_layer_2_tex", {});
        AddTexture("mask_layer_3_tex", {});
    } else if (variant.name == "elder") {
        AddTexture("color_layer_0_tex", "Art/2DArt/Tentacles/PeopleBackground.dds", true);
        AddTexture("color_layer_1_tex", "Art/2DArt/Tentacles/PeopleTentacleColor1.dds", true);
        AddTexture("color_layer_2_tex", "Art/2DArt/Tentacles/PeopleTentacleColor2.dds", true);
        AddTexture("color_layer_3_tex", "Art/2DArt/Tentacles/PeopleTentacleColor3.dds", true);
        GenTexture("influence_layer_0_tex", {16, 16}, 0xFF000000u); // add 16x16 0xFF000000 texture here
        AddTexture("influence_layer_1_tex", "Art/2DArt/Tentacles/PeopleTentacleInfluence.dds");
        AddTexture("influence_layer_2_tex", "Art/2DArt/Tentacles/PeopleTentacleInfluence.dds");
        AddTexture("influence_layer_3_tex", "Art/2DArt/Tentacles/PeopleTentacleInfluence.dds");
        GenTexture("mask_layer_0_tex", {16, 16}, 0xFFFFFFFF); // add 16x16 0xFFFFFFFF texture here
        AddTexture("mask_layer_1_tex", "Art/2DArt/Tentacles/PeopleTentacleMask1.dds");
        AddTexture("mask_layer_2_tex", "Art/2DArt/Tentacles/PeopleTentacleMask2.dds");
        AddTexture("mask_layer_3_tex", "Art/2DArt/Tentacles/PeopleTentacleMask3.dds");
    }
    AddTexture("muddle_tex", "Art/2DArt/Lookup/muddle.dds");
    AddTexture("background_tex", "Art/2DArt/Atlas/AtlasCompletelyBlank.dds", true);

    ResolveTextureBindings();
}

void Draw2DLayer::SetTime(double time) {
    psCbCpu_.time = time;
    psCbDirty_ = true;
}

void Draw2DLayer::Draw(glm::ivec2 pos, glm::ivec2 size) {
    CardLayer::Draw(pos, size);
    auto &ctx = dx_.ctx;

    SetPsCbData(&psCbCpu_, sizeof(psCbCpu_));

    ctx->PSSetShader(ps_, nullptr, 0);
    ctx->PSSetShaderResources(0, std::size(srvSlots_), std::data(srvSlots_));
    ctx->PSSetSamplers(0, std::size(dx_.samplers), std::data(dx_.samplers));
    ctx->PSSetConstantBuffers(0, 1, &psCb_.p);
    ctx->DrawIndexed(6, 0, 0);
}