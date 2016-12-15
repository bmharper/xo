#include "pch.h"
#include "RenderBase.h"
#include "../Text/GlyphCache.h"

namespace xo {

RenderBase::RenderBase() {
	TexIDOffset = 0;
}

RenderBase::~RenderBase() {
}

void RenderBase::Ortho(Mat4f& imat, double left, double right, double bottom, double top, double znear, double zfar) {
	Mat4f m;
	m.Zero();
	double A  = 2 / (right - left);
	double B  = 2 / (top - bottom);
	double C  = -2 / (zfar - znear);
	double tx = -(right + left) / (right - left);
	double ty = -(top + bottom) / (top - bottom);
	double tz = -(zfar + znear) / (zfar - znear);
	m.m(0, 0) = (float) A;
	m.m(1, 1) = (float) B;
	m.m(2, 2) = (float) C;
	m.m(3, 3) = 1;
	m.m(0, 3) = (float) tx;
	m.m(1, 3) = (float) ty;
	m.m(2, 3) = (float) tz;
	imat      = imat * m;
}

void RenderBase::SetupToScreen(Mat4f mvproj) {
	MVProj                     = mvproj;
	ShaderPerFrame.MVProj      = mvproj.Transposed(); // DirectX needs this transposed
	ShaderPerFrame.VPort_HSize = Vec2f(FBWidth / 2.0f, FBHeight / 2.0f);
}

Vec2f RenderBase::ToScreen(Vec2f v) {
	Vec4f r = MVProj * Vec4f(v.x, v.y, 0, 1);
	r.x     = (r.x + 1) * ShaderPerFrame.VPort_HSize.x;
	r.y     = (-r.y + 1) * ShaderPerFrame.VPort_HSize.y;
	return r.vec2;
}

void RenderBase::SurfaceLost_ForgetTextures() {
	TexIDOffset++;
	if (TexIDOffset >= Global()->MaxTextureID)
		TexIDOffset = 0;
	TexIDToNative.clear();
}

bool RenderBase::IsTextureValid(TextureID texID) const {
	TextureID relativeID = texID - TEX_OFFSET_ONE - TexIDOffset;
	return relativeID < (TextureID) TexIDToNative.size();
}

TextureID RenderBase::RegisterTexture(uintptr_t deviceTexID) {
	TextureID maxTexID = Global()->MaxTextureID;
	TextureID id       = TexIDOffset + (TextureID) TexIDToNative.size();
	if (id > maxTexID)
		id -= maxTexID;

	TexIDToNative += deviceTexID;
	return id + TEX_OFFSET_ONE;
}

uintptr_t RenderBase::GetTextureDeviceHandle(TextureID texID) const {
	TextureID absolute = texID - TEX_OFFSET_ONE - TexIDOffset;
	if (absolute >= (TextureID) TexIDToNative.size()) {
		XO_DIE_MSG("RenderBase::GetTextureDeviceHandle: Invalid texture ID. Use IsTextureValid() to check if a texture is valid.");
		return NULL;
	}
	return TexIDToNative[absolute];
}

void RenderBase::EnsureTextureProperlyDefined(Texture* tex, int texUnit) {
	XO_ASSERT(tex->TexWidth != 0 && tex->TexHeight != 0);
	XO_ASSERT(tex->TexFormat != TexFormatInvalid);
	XO_ASSERT(texUnit < MaxTextureUnits);
}

std::string RenderBase::CommonShaderDefines() {
	std::string s;
	s.append(xo::fmt("#define XO_GLYPH_ATLAS_SIZE %d.0\n", GlyphAtlasSize).Z);
	return s;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RenderDummy::InitializeDevice(SysWnd& wnd) {
	return false;
}
void RenderDummy::DestroyDevice(SysWnd& wnd) {
}
void RenderDummy::SurfaceLost() {
}

bool RenderDummy::BeginRender(SysWnd& wnd) {
	return false;
}
void RenderDummy::EndRender(SysWnd& wnd, uint32_t endRenderFlags) {
}

void RenderDummy::PreRender() {
}
void RenderDummy::PostRenderCleanup() {
}

ProgBase* RenderDummy::GetShader(Shaders shader) {
	return NULL;
}
void RenderDummy::ActivateShader(Shaders shader) {
}

void RenderDummy::Draw(GPUPrimitiveTypes type, int nvertex, const void* v) {
}

bool RenderDummy::LoadTexture(Texture* tex, int texUnit) {
	return true;
}
bool RenderDummy::ReadBackbuffer(Image& image) {
	return false;
}
}
