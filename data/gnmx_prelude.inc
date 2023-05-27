#define CONCAT_HELPER( x, y ) x##y
#define CONCAT( x, y ) CONCAT_HELPER( x, y )
#define GNMX
#define PIXEL_RETURN_SEMANTIC S_TARGET_OUTPUT
#define PIXEL_RETURN_SEMANTIC1 S_TARGET_OUTPUT1
#define PIXEL_RETURN_SEMANTIC2 S_TARGET_OUTPUT2
#define PIXEL_RETURN_SEMANTIC3 S_TARGET_OUTPUT3
#define SV_POSITION S_POSITION
#define SV_DEPTH S_DEPTH_OUTPUT
#define SV_VertexID S_VERTEX_ID
#define SV_InstanceID S_INSTANCE_ID
#define SV_GroupThreadID S_GROUP_THREAD_ID
#define SV_GroupID S_GROUP_ID
#define SV_GroupIndex S_GROUP_INDEX
#define SV_DispatchThreadID S_DISPATCH_THREAD_ID
#define CBUFFER_BEGIN( name ) ConstantBuffer name##{
#define CBUFFER_END }
#define SAMPLER_DECL( name ) SamplerState name
#define SAMPLER_CMPDECL( name ) SamplerComparisonState name
#define TEXTURE2D_DECL( name ) Texture2D name
#define TEXTURE3D_DECL( name ) Texture3D name
#define TEXTURECUBE_DECL( name ) TextureCube name
#define TEXTURE2DMS_DECL( name, count ) MS_Texture2D<float4> name
#define SAMPLE_TEX_BIAS( tex, sampler, uv, mip_offset) tex.SampleBias(sampler, uv, mip_offset)
#ifndef NO_MIPMAPS
	#define SAMPLE_TEX2D( tex, sampler, uv ) tex.Sample(sampler, uv)
#else
	#define SAMPLE_TEX2D( tex, sampler, uv ) tex.SampleLOD(sampler, uv, -0.5f )
#endif
#define SAMPLE_TEX2DLOD( tex, sampler, uvw ) tex.SampleLOD(sampler, (uvw).xy, (uvw).w )
#define SAMPLE_TEX3D( tex, sampler, uvw ) tex.Sample(sampler, uvw)
#define SAMPLE_TEX3DLOD( tex, sampler, uvw ) tex.SampleLOD(sampler, uvw.xyz, uvw.w )
#define SAMPLE_TEXCUBE( tex, sampler, uvw ) tex.Sample(sampler, uvw)
#define SAMPLE_TEXCUBELOD( tex, sampler, uvzw ) tex.SampleLOD(sampler, (uvzw).xyz, (uvzw).w )
#define SAMPLE_TEX2DGRAD( tex, sampler, uv, dx, dy ) tex.SampleGradient(sampler, uv, dx, dy )
#define SAMPLE_TEX2DPROJ( tex, sampler, uvw ) tex.Sample(sampler, uvw.xy/uvw.w )
#define SAMPLE_CMPLEVELZERO( tex, sampler, uvzw ) tex.SampleCmpLOD0(sampler, uvzw.xy / uvzw.w, uvzw.z / uvzw.w)
#define RWTEXTURE2D_DECL( name, type ) RW_Texture2D<type> name
#define STRUCTURED_BUFFER_DECL( name, type ) RegularBuffer< type > name
#define RWSTRUCTURED_BUFFER_DECL( name, type ) RW_RegularBuffer< type > name
#define TEXEL_BUFFER_DECL( name, type ) DataBuffer< type > name
#define RWTEXEL_BUFFER_DECL( name, type ) RW_DataBuffer< type > name
#define BYTE_ADDRESS_BUFFER_DECL( name ) ByteBuffer name
#define RWBYTE_ADDRESS_BUFFER_DECL( name ) RW_ByteBuffer name
#define COMPUTE_DECL_NUM_THREADS( X, Y, Z ) [NUM_THREADS(X, Y, Z)]
#define ATOMIC_ADD( rsc, offset, value ) rsc.AtomicAdd(offset, value)
#define ATOMIC_FETCH_ADD( rsc, offset, value, oldValue ) rsc.AtomicAdd(offset, value, oldValue)
#define ATOMIC_SUB( rsc, offset, value ) rsc.AtomicAdd(offset, -value)
#define ATOMIC_FETCH_SUB( rsc, offset, value, oldValue ) rsc.AtomicAdd(offset, -value, oldValue)
#define ATOMIC_AND( rsc, offset, value ) rsc.AtomicAnd(offset, value)
#define ATOMIC_FETCH_AND( rsc, offset, value, oldValue ) rsc.AtomicAnd(offset, value, oldValue)
#define ATOMIC_OR( rsc, offset, value ) rsc.AtomicOr(offset, value)
#define ATOMIC_FETCH_OR( rsc, offset, value, oldValue ) rsc.AtomicOr(offset, value, oldValue)
#define ATOMIC_XOR( rsc, offset, value ) rsc.AtomicXor(offset, value)
#define ATOMIC_FETCH_XOR( rsc, offset, value, oldValue ) rsc.AtomicXor(offset, value, oldValue)
#define ATOMIC_MIN( rsc, offset, value ) rsc.AtomicMin(offset, value)
#define ATOMIC_FETCH_MIN( rsc, offset, value, oldValue ) rsc.AtomicMin(offset, value, oldValue)
#define ATOMIC_MAX( rsc, offset, value ) rsc.AtomicMax(offset, value)
#define ATOMIC_FETCH_MAX( rsc, offset, value, oldValue ) rsc.AtomicMax(offset, value, oldValue)
#define ATOMIC_EXCHANGE( rsc, offset, value, oldValue ) rsc.AtomicExchange(offset, value, oldValue)
#define ATOMIC_CMP_STORE( rsc, offset, cmp, value ) rsc.AtomicCmpStore(offset, cmp, value)
#define ATOMIC_CMP_EXCHANGE( rsc, offset, cmp, value, oldValue ) rsc.AtomicCmpExchange(offset, cmp, value, oldValue)
#include "Shaders/SamplerDeclarations.inc"
#include "Shaders/CommonDeclarations.inc"
