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
#include <wlr/util/log.h>
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
	bool swizzle;
};

static const struct gles2_pixel_format formats[] = {
	{
		.wl_format = WL_SHM_FORMAT_ARGB8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 32,
		.bpp = 32,
		.has_alpha = true,
		.swizzle = true,
	},
	{
		.wl_format = WL_SHM_FORMAT_XRGB8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 24,
		.bpp = 32,
		.has_alpha = false,
		.swizzle = true,
	},
	{
		.wl_format = WL_SHM_FORMAT_XBGR8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 24,
		.bpp = 32,
		.has_alpha = false,
		.swizzle = false,
	},
	{
		.wl_format = WL_SHM_FORMAT_ABGR8888,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 32,
		.bpp = 32,
		.has_alpha = true,
		.swizzle = false,
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

const char *gles2_strerror(GLenum err) {
	switch (err) {
	case GL_INVALID_ENUM:
		return "Invalid enum";
	case GL_INVALID_VALUE:
		return "Invalid value";
	case GL_INVALID_OPERATION:
		return "Invalid operation";
	case GL_OUT_OF_MEMORY:
		return "Out of memory";
	case GL_INVALID_FRAMEBUFFER_OPERATION:
		return "Invalid framebuffer operation";
	default:
		return "Unknown error";
	}
}

static bool gles2_flush_errors(const char *context) {
	GLenum err;
	bool failure = false;
	while ((err = glGetError()) != GL_NO_ERROR) {
		failure = true;
		if (!context) {
			continue;
		}
		if (err == GL_OUT_OF_MEMORY) {
			// The OpenGL context is now undefined
			wlr_log(WLR_ERROR, "%s: Fatal GL error: out of memory", context);
			exit(1);
		} else {
			wlr_log(WLR_ERROR, "%s: GL error %d %s", context,
					err, gles2_strerror(err));
		}
	}
	return failure;
}

struct wlr_texture *WlrGLES2Renderer::texture_from_pixels(
		struct wlr_renderer *_renderer, enum wl_shm_format wl_fmt,
		uint32_t stride, uint32_t width, uint32_t height, const void *data) {
	struct WlrGLES2Renderer::renderer_state *state =
		(struct WlrGLES2Renderer::renderer_state *)_renderer;
	WlrGLES2Renderer *renderer = state->godot_renderer;
	auto storage =
		(RasterizerStorageGLES2 *)renderer->rasterizer->get_storage();
	gles2_flush_errors(NULL);

	RID rid = storage->texture_create();
	gles2_flush_errors("texture_create");
	RasterizerStorageGLES2::Texture *texture =
		storage->texture_owner.getornull(rid);

	storage->texture_allocate(rid, width, height, 0,
			Image::FORMAT_RGBA8, VS::TEXTURE_TYPE_2D, 0);
	gles2_flush_errors("texture_allocate");

	const struct gles2_pixel_format *fmt = get_gles2_format_from_wl(wl_fmt);
	if (fmt == NULL) {
		wlr_log(WLR_ERROR, "Unsupported pixel format %" PRIu32, wl_fmt);
		return NULL;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, stride / (fmt->bpp / 8));
	gles2_flush_errors("glPixelStorei");

	if (fmt->swizzle) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, fmt->gl_type, data);
	gles2_flush_errors("glTexImage2D");

	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0);
	gles2_flush_errors("glPixelStorei (2)");

	WlrGLES2Texture *wlr_texture = new WlrGLES2Texture(rid, width, height);
	wlr_texture->reference();
	return wlr_texture->get_wlr_texture();
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
	.texture_from_pixels = WlrGLES2Renderer::texture_from_pixels,
	.init_wl_display = renderer_init_wl_display,
};

}

struct wlr_renderer *WlrGLES2Renderer::get_wlr_renderer() {
	return &renderer_state.wlr_renderer;
}

WlrGLES2Renderer::WlrGLES2Renderer(RasterizerGLES2 *p_rasterizer) {
	rasterizer = p_rasterizer;
	wlr_renderer_init(&renderer_state.wlr_renderer, &renderer_impl);
	renderer_state.godot_renderer = this;
	WlrRenderer::singleton = this;
}

extern "C" {

void WlrGLES2Texture::wlr_texture_get_size(
		struct wlr_texture *_texture, int *width, int *height) {
	WlrGLES2Texture *texture = (WlrGLES2Texture *)_texture;
	*width = texture->w;
	*height = texture->h;
}

bool WlrGLES2Texture::wlr_texture_write_pixels(
		struct wlr_texture *_texture, uint32_t stride,
		uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		const void *data) {
	wlr_log(WLR_DEBUG, "TODO: write_pixels");
	return false;
}

void WlrGLES2Texture::wlr_texture_destroy(struct wlr_texture *_texture) {
	WlrGLES2Texture *texture = WlrGLES2Texture::texture_from_wlr(
		_texture);
	texture->unreference();
}

static const struct wlr_texture_impl texture_impl = {
	.get_size = WlrGLES2Texture::wlr_texture_get_size,
	.write_pixels = WlrGLES2Texture::wlr_texture_write_pixels,
	.destroy = WlrGLES2Texture::wlr_texture_destroy,
};

}

WlrGLES2Renderer::~WlrGLES2Renderer() {
	wlr_renderer_destroy(&renderer_state.wlr_renderer);
}

Texture *WlrGLES2Renderer::texture_from_wlr(struct wlr_texture *texture) {
	return WlrGLES2Texture::texture_from_wlr(texture);
}

WlrGLES2Texture::WlrGLES2Texture(RID p_texture, int width, int height) {
	wlr_texture_init(&state.wlr_texture, &texture_impl);
	state.godot_texture = this;
	texture = p_texture;
	w = width;
	h = height;
}

int WlrGLES2Texture::get_width() const {
	return w;
}

int WlrGLES2Texture::get_height() const {
	return h;
}

RID WlrGLES2Texture::get_rid() const {
	return texture;
}

bool WlrGLES2Texture::has_alpha() const {
	// TODO
	return false;
}

void WlrGLES2Texture::set_flags(uint32_t p_flags) {
	// We manage our own flags, bugger off
}

uint32_t WlrGLES2Texture::get_flags() const {
	return flags;
}

struct wlr_texture *WlrGLES2Texture::get_wlr_texture() {
	return &state.wlr_texture;
}

WlrGLES2Texture *WlrGLES2Texture::texture_from_wlr(
		struct wlr_texture *texture) {
	if (texture == NULL) {
		return NULL;
	}
	struct WlrGLES2Texture::texture_state *state =
		(struct WlrGLES2Texture::texture_state *)texture;
	return state->godot_texture;
}
