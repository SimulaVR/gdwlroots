#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdint.h>
#include "drivers/gles2/rasterizer_gles2.h"
#include "drivers/gles2/rasterizer_storage_gles2.h"
#include "gles2_renderer.h"
extern "C" {
#define static
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#undef static

static const enum wl_shm_format wl_formats[] = {
	WL_SHM_FORMAT_ARGB8888,
	WL_SHM_FORMAT_XRGB8888,
	WL_SHM_FORMAT_ABGR8888,
	WL_SHM_FORMAT_XBGR8888,
};

struct gles2_pixel_format {
	enum wl_shm_format wl_format;
	GLint gl_format, gl_type;
	int depth, bpp;
	bool has_alpha;
};

static const struct gles2_pixel_format formats[] = {
	{
		.wl_format = WL_SHM_FORMAT_ARGB8888,
		.gl_format = GL_BGRA_EXT,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 32,
		.bpp = 32,
		.has_alpha = true,
	},
	{
		.wl_format = WL_SHM_FORMAT_XRGB8888,
		.gl_format = GL_BGRA_EXT,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 24,
		.bpp = 32,
		.has_alpha = false,
	},
	{
		.wl_format = WL_SHM_FORMAT_XBGR8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 24,
		.bpp = 32,
		.has_alpha = false,
	},
	{
		.wl_format = WL_SHM_FORMAT_ABGR8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 32,
		.bpp = 32,
		.has_alpha = true,
	},
};

const struct gles2_pixel_format *get_gles2_format_from_wl(
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
	return get_gles2_format_from_wl(fmt) != NULL;
}

static struct wlr_texture *renderer_texture_from_pixels(
		struct wlr_renderer *_renderer, enum wl_shm_format fmt,
		uint32_t stride, uint32_t width, uint32_t height, const void *data) {
	WlrGLES2Renderer *renderer = (WlrGLES2Renderer *)_renderer;
	return NULL;
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
	.texture_from_pixels = renderer_texture_from_pixels,
	.init_wl_display = renderer_init_wl_display,
};

}

struct wlr_renderer *WlrGLES2Renderer::get_wlr_renderer() {
	return &wlr_renderer;
}

WlrGLES2Renderer::WlrGLES2Renderer(RasterizerGLES2 *p_rasterizer) {
	rasterizer = p_rasterizer;
	storage = dynamic_cast<RasterizerStorageGLES2 *>(rasterizer->get_storage());
	wlr_renderer_init(&wlr_renderer, &renderer_impl);
}

WlrGLES2Renderer::~WlrGLES2Renderer() {
	wlr_renderer_destroy(&wlr_renderer);
}
