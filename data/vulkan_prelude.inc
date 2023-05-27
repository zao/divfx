#define CONCAT_HELPER( x, y ) x##y
#define CONCAT( x, y ) CONCAT_HELPER( x, y )
#define VULKAN
#define PIXEL_RETURN_SEMANTIC SV_TARGET
#define PIXEL_RETURN_SEMANTIC1 SV_TARGET1
#define PIXEL_RETURN_SEMANTIC2 SV_TARGET2
#define PIXEL_RETURN_SEMANTIC3 SV_TARGET3
#define CBUFFER_BEGIN( name ) cbuffer name##_cb {
#define CBUFFER_END }
#define SAMPLER_DECL( name ) SamplerState name
#define SAMPLER_CMPDECL( name ) SamplerComparisonState name
#define TEXTURE2D_DECL( name ) Texture2D name
#define TEXTURE3D_DECL( name ) Texture3D name
#define TEXTURECUBE_DECL( name ) TextureCube name
#define TEXTURE2DMS_DECL( name, count ) Texture2DMS<float4, count> name
#define SAMPLE_TEX_BIAS( tex, sampler, uv, mip_offset) tex.SampleBias(sampler, uv, mip_offset)
#ifndef NO_MIPMAPS
	#define SAMPLE_TEX2D( tex, sampler, uv ) tex.Sample(sampler, uv)
#else
	#define SAMPLE_TEX2D( tex, sampler, uv ) tex.SampleLevel(sampler, uv, -0.5f )
#endif
#define SAMPLE_TEX2DLOD( tex, sampler, uvw ) tex.SampleLevel(sampler, (uvw).xy, (uvw).w )
#define SAMPLE_TEX3D( tex, sampler, uvw ) tex.Sample(sampler, uvw)
#define SAMPLE_TEX3DLOD( tex, sampler, uvw ) tex.SampleLevel(sampler, uvw.xyz, uvw.w )
#define SAMPLE_TEXCUBE( tex, sampler, uvw ) tex.Sample(sampler, uvw)
#define SAMPLE_TEXCUBELOD( tex, sampler, uvzw ) tex.SampleLevel(sampler, (uvzw).xyz, (uvzw).w )
#define SAMPLE_TEX2DGRAD( tex, sampler, uv, dx, dy ) tex.SampleGrad(sampler, uv, dx, dy )
#define SAMPLE_TEX2DPROJ( tex, sampler, uvw ) tex.Sample(sampler, uvw.xy/uvw.w )
#define SAMPLE_CMPLEVELZERO( tex, sampler, uvzw ) tex.SampleCmpLevelZero(sampler, uvzw.xy / uvzw.w, uvzw.z / uvzw.w)
#define RWTEXTURE2D_DECL( name, type ) RWTexture2D<type> name
#define STRUCTURED_BUFFER_DECL( name, type ) StructuredBuffer< type > name
#define RWSTRUCTURED_BUFFER_DECL( name, type ) RWStructuredBuffer< type > name
#define TEXEL_BUFFER_DECL( name, type ) Buffer< type > name
#define RWTEXEL_BUFFER_DECL( name, type ) RWBuffer< type > name
#define BYTE_ADDRESS_BUFFER_DECL( name ) ByteAddressBuffer name
#define RWBYTE_ADDRESS_BUFFER_DECL( name ) RWByteAddressBuffer name
#define COMPUTE_DECL_NUM_THREADS( X, Y, Z ) [numthreads(X, Y, Z)]
#define ATOMIC_ADD( rsc, offset, value ) rsc.InterlockedAdd(offset, value)
#define ATOMIC_FETCH_ADD( rsc, offset, value, oldValue ) rsc.InterlockedAdd(offset, value, oldValue)
#define ATOMIC_SUB( rsc, offset, value ) rsc.InterlockedAdd(offset, -value)
#define ATOMIC_FETCH_SUB( rsc, offset, value, oldValue ) rsc.InterlockedAdd(offset, -value, oldValue)
#define ATOMIC_AND( rsc, offset, value ) rsc.InterlockedAnd(offset, value)
#define ATOMIC_FETCH_AND( rsc, offset, value, oldValue ) rsc.InterlockedAnd(offset, value, oldValue)
#define ATOMIC_OR( rsc, offset, value ) rsc.InterlockedOr(offset, value)
#define ATOMIC_FETCH_OR( rsc, offset, value, oldValue ) rsc.InterlockedOr(offset, value, oldValue)
#define ATOMIC_XOR( rsc, offset, value ) rsc.InterlockedXor(offset, value)
#define ATOMIC_FETCH_XOR( rsc, offset, value, oldValue ) rsc.InterlockedXor(offset, value, oldValue)
#define ATOMIC_MIN( rsc, offset, value ) rsc.InterlockedMin(offset, value)
#define ATOMIC_FETCH_MIN( rsc, offset, value, oldValue ) rsc.InterlockedMin(offset, value, oldValue)
#define ATOMIC_MAX( rsc, offset, value ) rsc.InterlockedMax(offset, value)
#define ATOMIC_FETCH_MAX( rsc, offset, value, oldValue ) rsc.InterlockedMax(offset, value, oldValue)
#define ATOMIC_EXCHANGE( rsc, offset, value, oldValue ) rsc.InterlockedExchange(offset, value, oldValue)
#define ATOMIC_CMP_STORE( rsc, offset, cmp, value ) rsc.InterlockedCompareStore(offset, cmp, value)
#define ATOMIC_CMP_EXCHANGE( rsc, offset, cmp, value, oldValue ) rsc.InterlockedCompareExchange(offset, cmp, value, oldValue)
#include "Shaders/SamplerDeclarations.inc"
#include "Shaders/CommonDeclarations.inc"
