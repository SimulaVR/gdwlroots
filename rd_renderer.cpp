#include <stdint.h>
#include "rd_renderer.h"
#include <string>
#include <iostream>
#include "servers/rendering/rendering_server_globals.h"
namespace wlr {
extern "C" {
#define static
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#include <wlr/util/log.h>
#undef static



void saveScreenshotToFile(std::string filename, int windowWidth, int windowHeight) {
}

static const enum wl_shm_format wl_formats[] = {
	WL_SHM_FORMAT_ARGB8888,
	WL_SHM_FORMAT_XRGB8888,
	WL_SHM_FORMAT_ABGR8888,
	WL_SHM_FORMAT_XBGR8888,
};

static const struct rd_pixel_format formats[] = {
	{
		.wl_format = WL_SHM_FORMAT_ARGB8888,
		.img_format = Image::FORMAT_BGRA8,
		.size = 4,
		.has_alpha = true,
		.rd_format = RD::DATA_FORMAT_R8G8B8A8_UNORM,
		.rd_swizzle_r = RD::TEXTURE_SWIZZLE_B,
		.rd_swizzle_g = RD::TEXTURE_SWIZZLE_G,
		.rd_swizzle_b = RD::TEXTURE_SWIZZLE_R,
		.rd_swizzle_a = RD::TEXTURE_SWIZZLE_A,
	},
	{
		.wl_format = WL_SHM_FORMAT_XRGB8888,
		.img_format = Image::FORMAT_BGRA8,
		.size = 4,
		.has_alpha = false,
                .rd_format = RD::DATA_FORMAT_R8G8B8A8_UNORM,
                .rd_swizzle_r = RD::TEXTURE_SWIZZLE_R,
                .rd_swizzle_g = RD::TEXTURE_SWIZZLE_G,
                .rd_swizzle_b = RD::TEXTURE_SWIZZLE_R,
                .rd_swizzle_a = RD::TEXTURE_SWIZZLE_A,

	},
	{
		.wl_format = WL_SHM_FORMAT_XBGR8888,
		.img_format = Image::FORMAT_RGBA8,
		.size = 4,
		.has_alpha = true,
                .rd_format = RD::DATA_FORMAT_R8G8B8A8_UNORM,
                .rd_swizzle_r = RD::TEXTURE_SWIZZLE_R,
                .rd_swizzle_g = RD::TEXTURE_SWIZZLE_G,
                .rd_swizzle_b = RD::TEXTURE_SWIZZLE_B,
                .rd_swizzle_a = RD::TEXTURE_SWIZZLE_A,

	},
	{
		.wl_format = WL_SHM_FORMAT_ABGR8888,
		.img_format = Image::FORMAT_RGBA8,
		.size = 4,
		.has_alpha = false,
                .rd_format = RD::DATA_FORMAT_R8G8B8A8_UNORM,
                .rd_swizzle_r = RD::TEXTURE_SWIZZLE_R,
                .rd_swizzle_g = RD::TEXTURE_SWIZZLE_G,
                .rd_swizzle_b = RD::TEXTURE_SWIZZLE_B,
                .rd_swizzle_a = RD::TEXTURE_SWIZZLE_A,

	},
};

const struct rd_pixel_format *get_rd_format_from_wl(
		enum wl_shm_format fmt) {
	for (size_t i = 0; i < sizeof(formats) / sizeof(*formats); ++i) {
		if (formats[i].wl_format == fmt) {
			return &formats[i];
		}
	}
	return NULL;
}

static const enum wl_shm_format *renderer_formats(
		struct wlr_renderer *renderer, size_t *len) {
	*len = sizeof(wl_formats) / sizeof(wl_formats[0]);
	return wl_formats;
}

static bool renderer_format_supported(
		struct wlr_renderer *renderer, enum wl_shm_format fmt) {
  //std::cout << "renderer_format_supported: " << get_gles3_format_from_wl(fmt) << std::endl;
	return get_rd_format_from_wl(fmt) != NULL;
}

static void renderer_init_wl_display(struct wlr_renderer *renderer,
		struct wl_display *wl_display) {
	// TODO: bind EGL
}

static void renderer_begin(struct wlr_renderer *renderer,
		uint32_t width, uint32_t height) {
	/* This space deliberately left blank */
}

static void renderer_end(struct wlr_renderer *renderer) {
	/* This space deliberately left blank */
}

void renderer_clear(struct wlr_renderer *renderer, const float color[4]) {
	/* This space deliberately left blank */
}

void renderer_scissor(struct wlr_renderer *renderer, struct wlr_box *box) {
	/* This space deliberately left blank */
}

bool renderer_render_texture_with_matrix(struct wlr_renderer *renderer,
		struct wlr_texture *texture, const float matrix[9], float alpha) {
	/* This space deliberately left blank */
	return false;
}

void renderer_render_quad_with_matrix(struct wlr_renderer *renderer,
		const float color[4], const float matrix[9]) {
	/* This space deliberately left blank */
}

void renderer_render_ellipse_with_matrix(struct wlr_renderer *renderer,
		const float color[4], const float matrix[9]) {
	/* This space deliberately left blank */
}

static const struct wlr_renderer_impl renderer_impl = {
	/* We need to implement these, but we don't use them */
	/* TODO wlroots: we should consider separating the "allocate textures"
	 * interface from the "abstract drawing library" interface, at least */
	.begin = renderer_begin,
	.end = renderer_end,
	.clear = renderer_clear,
	.scissor = renderer_scissor,
	.render_texture_with_matrix = renderer_render_texture_with_matrix,
	.render_quad_with_matrix = renderer_render_quad_with_matrix,
	.render_ellipse_with_matrix = renderer_render_ellipse_with_matrix,
	/* We use these */
	.formats = renderer_formats,
	.format_supported = renderer_format_supported,
	.texture_from_pixels = WlrRDRenderer::texture_from_pixels,
	.init_wl_display = renderer_init_wl_display,
};

}

}

using namespace wlr;

struct wlr_texture *WlrRDRenderer::texture_from_pixels(
                struct wlr_renderer *_renderer, enum wl_shm_format wl_fmt,
                uint32_t stride, uint32_t width, uint32_t height, const void *data) {

        auto fmt = get_rd_format_from_wl(wl_fmt);
        if (!fmt) {
                wlr_log(WLR_ERROR, "Unsupported pixel format %" PRIu32, wl_fmt);
                return NULL;
        }


        Vector<uint8_t> vector_data;
        vector_data.resize(height*stride*fmt->size);
        copymem(vector_data.ptrw(), data, vector_data.size());

        auto img = memnew(Image(width, height, false, fmt->img_format, vector_data));

        auto tex_rid = RSG::storage->texture_allocate();
        RSG::storage->texture_2d_initialize(tex_rid, img);

        auto wlr_texture = new WlrRDTexture(tex_rid, width, height, fmt);
        wlr_texture->reference();
        return wlr_texture->get_wlr_texture();
}


struct wlr_renderer *WlrRDRenderer::get_wlr_renderer() {
	return &renderer_state.wlr_renderer;
}

WlrRDRenderer::WlrRDRenderer() {
	wlr_renderer_init(&renderer_state.wlr_renderer, &renderer_impl);
	renderer_state.godot_renderer = this;
	WlrRenderer::singleton = this;
}

extern "C" {

void WlrRDTexture::wlr_texture_get_size(struct wlr_texture *_texture, int *width, int *height) {
  WlrRDTexture *texture = WlrRDTexture::texture_from_wlr(_texture);
  if(texture) {
    *width = texture->w;
    *height = texture->h;
  }
}

bool WlrRDTexture::wlr_texture_write_pixels(
		struct wlr_texture *_texture, uint32_t stride,
		uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		const void *data) {
	auto wlr_rd_texture = WlrRDTexture::texture_from_wlr(_texture);

	// TODO hack
        Vector<uint8_t> vector_data;
        vector_data.resize(height*stride * wlr_rd_texture->pixel_format->size);
        copymem(vector_data.ptrw(), data, vector_data.size());

        auto img = memnew(Image(width, height, false, wlr_rd_texture->pixel_format->img_format, vector_data));
	

	RSG::storage->texture_2d_update(wlr_rd_texture->get_rid(), img, 0);

	return true;
}

void WlrRDTexture::wlr_texture_destroy(struct wlr_texture *_texture) {
	WlrRDTexture *texture = WlrRDTexture::texture_from_wlr(
		_texture);
	texture->unreference();
}

static const struct wlr_texture_impl texture_impl = {
	.get_size = WlrRDTexture::wlr_texture_get_size,
	.write_pixels = WlrRDTexture::wlr_texture_write_pixels,
	.destroy = WlrRDTexture::wlr_texture_destroy,
};

}

WlrRDRenderer::~WlrRDRenderer() {
	wlr_renderer_destroy(&renderer_state.wlr_renderer);
}

Texture *WlrRDRenderer::texture_from_wlr(struct wlr_texture *texture) {
	return WlrRDTexture::texture_from_wlr(texture);
}

WlrRDTexture::WlrRDTexture(RID p_texture, int width, int height,
		const struct rd_pixel_format *fmt) {
	wlr_texture_init(&state.wlr_texture, &texture_impl);
	state.godot_texture = this;
	texture = p_texture;
	pixel_format = fmt;
	w = width;
	h = height;
}

int WlrRDTexture::get_width() const {
	return w;
}

int WlrRDTexture::get_height() const {
	return h;
}

RID WlrRDTexture::get_rid() const {
	return texture;
}

bool WlrRDTexture::has_alpha() const {
	return pixel_format->has_alpha;
}

void WlrRDTexture::set_flags(uint32_t p_flags) {
	// We manage our own flags, bugger off
}

uint32_t WlrRDTexture::get_flags() const {
	return flags;
}

struct wlr_texture *WlrRDTexture::get_wlr_texture() {
	return &state.wlr_texture;
}

WlrRDTexture *WlrRDTexture::texture_from_wlr(
		struct wlr_texture *texture) {
	if (texture == NULL) {
		return NULL;
	}
	struct WlrRDTexture::texture_state *state =
		(struct WlrRDTexture::texture_state *)texture;
	return state->godot_texture;
}
