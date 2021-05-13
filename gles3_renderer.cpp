#include <stdint.h>
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/rasterizer_storage_gles3.h"
#include "gles3_renderer.h"
#include<string>
#include <iostream>
extern "C" {
#define static
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#include <wlr/util/log.h>
#undef static

#define _GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE

static void generateMipmaps(RasterizerStorageGLES3::Texture* texture, int width, int height, int level);

void saveScreenshotToFile(std::string filename, int windowWidth, int windowHeight) {
    const int numberOfPixels = windowWidth * windowHeight * 3;
    unsigned char pixels[numberOfPixels];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    //glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    FILE *outputFile = fopen(filename.c_str(), "w");
    short header[] = {0, 2, 0, 0, 0, 0, (short) windowWidth, (short) windowHeight, 24};

    fwrite(&header, sizeof(header), 1, outputFile);
    fwrite(pixels, numberOfPixels, 1, outputFile);
    fclose(outputFile);

    printf("Finish writing to file.\n");
}

static const enum wl_shm_format wl_formats[] = {
	WL_SHM_FORMAT_ARGB8888,
	WL_SHM_FORMAT_XRGB8888,
	WL_SHM_FORMAT_ABGR8888,
	WL_SHM_FORMAT_XBGR8888,
};

static const struct gles3_pixel_format formats[] = {
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

const struct gles3_pixel_format *get_gles3_format_from_wl(
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
	return get_gles3_format_from_wl(fmt) != NULL;
}

const char *gles3_strerror(GLenum err) {
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

static bool gles3_flush_errors(const char *context) {
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
					err, gles3_strerror(err));
		}
	}
	return failure;
}

struct wlr_texture *WlrGLES3Renderer::texture_from_pixels(
		struct wlr_renderer *_renderer, enum wl_shm_format wl_fmt,
		uint32_t stride, uint32_t width, uint32_t height, const void *data) {

	struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)_renderer;
	WlrGLES3Renderer *renderer = state->godot_renderer;
	auto storage =
		(RasterizerStorageGLES3 *)renderer->rasterizer->get_storage();
	gles3_flush_errors(NULL);

	RID rid = storage->texture_create();
	gles3_flush_errors("texture_create");
	RasterizerStorageGLES3::Texture *texture =
		storage->texture_owner.getornull(rid);

	storage->texture_allocate(rid, width, height, 0,
			Image::FORMAT_RGBA8, VS::TEXTURE_TYPE_2D, 0);
	gles3_flush_errors("texture_allocate");

  texture->data_size = width * height * 4; //for savePng

	const struct gles3_pixel_format *fmt = get_gles3_format_from_wl(wl_fmt);
	if (fmt == NULL) {
		wlr_log(WLR_ERROR, "Unsupported pixel format %" PRIu32, wl_fmt);
		return NULL;
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gles3_flush_errors("glTexParameterf");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gles3_flush_errors("glPixelStorei");

	if (fmt->swizzle && fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	} else if (fmt->swizzle && !fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ONE}; //force the alpha values to 1 (or else they'll be garbage & cause weird transparency effects)
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / (fmt->bpp / 8));


	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, fmt->gl_type, data);
	gles3_flush_errors("glTexImage2D");


	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	generateMipmaps(texture, width, height, 0);

	WlrGLES3Texture *wlr_texture = new WlrGLES3Texture(storage, rid, width, height, fmt);
	wlr_texture->reference();
	return wlr_texture->get_wlr_texture();
}

static void generateMipmaps(RasterizerStorageGLES3::Texture* texture, int width, int height, int level) {
	return;
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
	.texture_from_pixels = WlrGLES3Renderer::texture_from_pixels,
	.init_wl_display = renderer_init_wl_display,
};

}

struct wlr_renderer *WlrGLES3Renderer::get_wlr_renderer() {
	return &renderer_state.wlr_renderer;
}

WlrGLES3Renderer::WlrGLES3Renderer(RasterizerGLES3 *p_rasterizer) {
	rasterizer = p_rasterizer;
	wlr_renderer_init(&renderer_state.wlr_renderer, &renderer_impl);
	renderer_state.godot_renderer = this;
	WlrRenderer::singleton = this;
}

extern "C" {

void WlrGLES3Texture::wlr_texture_get_size(struct wlr_texture *_texture, int *width, int *height) {
  WlrGLES3Texture *texture = WlrGLES3Texture::texture_from_wlr(_texture);
  if(texture) {
    *width = texture->w;
    *height = texture->h;
  }
}

bool WlrGLES3Texture::wlr_texture_write_pixels(
		struct wlr_texture *_texture, uint32_t stride,
		uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		const void *data) {
	WlrGLES3Texture *gles3_texture = WlrGLES3Texture::texture_from_wlr(
		_texture);
	gles3_flush_errors(NULL);

	RasterizerStorageGLES3::Texture *texture =
		gles3_texture->storage->texture_owner.getornull(gles3_texture->texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	gles3_flush_errors("glTexParameterf");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gles3_flush_errors("glPixelStorei");

	const struct gles3_pixel_format *fmt = gles3_texture->pixel_format;
	if (fmt->swizzle && fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	} else if (fmt->swizzle && !fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ONE}; //force the alpha values to 1 (or else they'll be garbage & cause weird transparency effects)
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
  }


	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / (fmt->bpp / 8));
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, src_x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, src_y);

	glTexSubImage2D(GL_TEXTURE_2D, 0, dst_x, dst_y, width, height, fmt->gl_format, fmt->gl_type, data);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

	generateMipmaps(texture, width, height, 0);

	return true;
}

void WlrGLES3Texture::wlr_texture_destroy(struct wlr_texture *_texture) {
	WlrGLES3Texture *texture = WlrGLES3Texture::texture_from_wlr(
		_texture);
	texture->unreference();
}

static const struct wlr_texture_impl texture_impl = {
	.get_size = WlrGLES3Texture::wlr_texture_get_size,
	.write_pixels = WlrGLES3Texture::wlr_texture_write_pixels,
	.destroy = WlrGLES3Texture::wlr_texture_destroy,
};

}

WlrGLES3Renderer::~WlrGLES3Renderer() {
	wlr_renderer_destroy(&renderer_state.wlr_renderer);
}

Texture *WlrGLES3Renderer::texture_from_wlr(struct wlr_texture *texture) {
	return WlrGLES3Texture::texture_from_wlr(texture);
}

WlrGLES3Texture::WlrGLES3Texture(RasterizerStorageGLES3 *p_storage,
		RID p_texture, int width, int height,
		const struct gles3_pixel_format *fmt) {
	wlr_texture_init(&state.wlr_texture, &texture_impl);
	state.godot_texture = this;
	storage = p_storage;
	texture = p_texture;
	pixel_format = fmt;
	w = width;
	h = height;
}

int WlrGLES3Texture::get_width() const {
	return w;
}

int WlrGLES3Texture::get_height() const {
	return h;
}

RID WlrGLES3Texture::get_rid() const {
	return texture;
}

bool WlrGLES3Texture::has_alpha() const {
	return pixel_format->has_alpha;
}

void WlrGLES3Texture::set_flags(uint32_t p_flags) {
	// We manage our own flags, bugger off
}

uint32_t WlrGLES3Texture::get_flags() const {
	return flags;
}

struct wlr_texture *WlrGLES3Texture::get_wlr_texture() {
	return &state.wlr_texture;
}

WlrGLES3Texture *WlrGLES3Texture::texture_from_wlr(
		struct wlr_texture *texture) {
	if (texture == NULL) {
		return NULL;
	}
	struct WlrGLES3Texture::texture_state *state =
		(struct WlrGLES3Texture::texture_state *)texture;
	return state->godot_texture;
}
