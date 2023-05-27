#pragma once

#include "D3D.hpp"
#include <glm/glm.hpp>

#include <map>
#include <string_view>
#include <string>

enum CardSize {
    eCardWidth = 390,
    eCardHeight = 280,
};

struct CardLayer {
    CardLayer(Dx &dx, DivFxCompiler const &divFxCompiler);
    virtual ~CardLayer() = default;

    void SetViewTransform(glm::mat4 transform);

    virtual void SetTime(double time) = 0;
    virtual void Draw(glm::ivec2 pos, glm::ivec2 size) = 0;

  protected:
    void SetPsCbData(void const *data, size_t size);
    void AddTexture(std::string_view name, std::string_view path, bool viewAsSrgb = false);
    void GenTexture(std::string_view name, glm::ivec2 size, uint32_t value);
    void ResolveTextureBindings();
    void ReflectPixelShader(CComPtr<ID3DBlob> bytecode);

    struct VsCbData {
        glm::mat4 uiScale;
    };

    Dx &dx_;
    VsCbData vsCbCpu_;
    bool vsCbDirty_{true};
    bool psCbDirty_{true};

    CComPtr<ID3D11RasterizerState> divRaster_;
    CComPtr<ID3D11BlendState> divBlend_;
    CComPtr<ID3D11InputLayout> il_;
    CComPtr<ID3D11Buffer> ib_, vb_;
    CComPtr<ID3D11VertexShader> vs_;
    CComPtr<ID3D11Buffer> vsCb_, psCb_;

    std::map<UINT, std::string> texNameBySlot_;
    std::map<std::string, UINT> texSlotByName_;

    struct SrvBind {
        UINT slot;
        CComPtr<ID3D11ShaderResourceView> srv;
    };
    std::vector<SrvBind> psSrvs_;
    std::vector<ID3D11ShaderResourceView *> srvSlots_;
};

struct AtlasEffectsVariant {
    std::string name;
    CComPtr<ID3D11PixelShader> ps;
    CComPtr<ID3DBlob> psBytecode;
};

struct AtlasEffectsLayer : CardLayer {
    AtlasEffectsLayer(Dx &dx, DivFxCompiler &divFxCompiler, AtlasEffectsVariant const &variant);

    void SetTime(double time) override;

    void Draw(glm::ivec2 pos, glm::ivec2 size);

  private:
    enum { MAX_MEMORY_LINE_NODE_IMAGES = 5 };
    struct PsCbData {
        float time;
        float image_width;
        float image_height;
        float progress;
        // --
        float alpha;
        float bonus_completed;
        uint32_t num_segments;
        float radians_per_second;
        // --
        BOOL doubled_memory_line_segments;
        uint32_t num_memory_line_textures;
        /* pad two words */ uint32_t pad0[2];
        // --
        glm::vec4 memory_line_texture_datas[MAX_MEMORY_LINE_NODE_IMAGES]; // texture_idx, scale_x, scale_y,
                                                                          // texture_aspect_ratio (width/height)
        // --
        glm::vec4 memory_line_texture_uvs[MAX_MEMORY_LINE_NODE_IMAGES]; // centre_u, centre_v, radius_u, radius_v
    };

    PsCbData psCbCpu_;
    std::vector<CComPtr<ID3D11ShaderResourceView>> psSrvs_;
    CComPtr<ID3D11PixelShader> ps_;
};

struct Draw2DVariant {
    std::string name;
    CComPtr<ID3D11PixelShader> ps;
    CComPtr<ID3DBlob> psBytecode;
};

struct Draw2DLayer : CardLayer {
    Draw2DLayer(Dx &dx, DivFxCompiler &divFxCompiler, Draw2DVariant const &variant);

    void SetTime(double time) override;

    void Draw(glm::ivec2 pos, glm::ivec2 size);

  private:
    struct PsCbData {
        float time;
        float seed;
        float effect_phase;
        float to_base_resolution_multiplier;
        //--
        float has_mask;
        float has_background;
        float is_div_card;
        /* pad one word */ uint32_t pad0[1];
        //--
        glm::vec4 aspect_ratio;
        glm::vec4 tex_scale;
        float muddle_frequency;
        /* pad three words */ uint32_t pad1[3];
        //--
        glm::vec4 muddle_intensity;
        glm::vec4 tex_clamp;
        glm::vec4 effect_params;
        float shader_type;
        float layers_count;
        /* pad two words */ uint32_t pad2[2];
        //--
        glm::vec4 item_size;
        glm::vec4 flash_glow_color;
        glm::vec4 hellscape_params;

        glm::vec4 layer_0_speed;
        glm::vec4 layer_1_speed;
        glm::vec4 layer_2_speed;
        glm::vec4 layer_3_speed;
    };

    PsCbData psCbCpu_;
    CComPtr<ID3D11PixelShader> ps_;
};