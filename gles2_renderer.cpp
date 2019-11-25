#include <stdint.h>
#include "drivers/gles2/rasterizer_gles2.h"
#include "drivers/gles2/rasterizer_storage_gles2.h"
#include "gles2_renderer.h"
#include<string>
#include <iostream>
extern "C" {
#define static
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#include <wlr/util/log.h>
#undef static

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
  //std::cout << "renderer_format_supported: " << get_gles2_format_from_wl(fmt) << std::endl;
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

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gles2_flush_errors("glTexParameterf");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gles2_flush_errors("glPixelStorei");

	if (fmt->swizzle && fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	} else if (fmt->swizzle && !fmt->has_alpha) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ONE}; //force the alpha values to 1 (or else they'll be garbage & cause weird transparency effects)
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / (fmt->bpp / 8));

  //glGenerateMipmap(texture->target); //Doesn't seem to cause mip maps to be generated
  //glGenerateTextureMipmap(texture->tex_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_RGBA, fmt->gl_type, data);
	gles2_flush_errors("glTexImage2D");

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	WlrGLES2Texture *wlr_texture = new WlrGLES2Texture(
			storage, rid, width, height, fmt);
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

void WlrGLES2Texture::wlr_texture_get_size(struct wlr_texture *_texture, int *width, int *height) {
  WlrGLES2Texture *texture = (WlrGLES2Texture *)_texture;
  if(texture) {
    *width = texture->w; //gles2_renderer.cpp:275
    *height = texture->h;
  } else {
    *width = 0; //gles2_renderer.cpp:275
    *height = 0;
  }
}

bool WlrGLES2Texture::wlr_texture_write_pixels(
		struct wlr_texture *_texture, uint32_t stride,
		uint32_t width, uint32_t height,
		uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
		const void *data) {
	WlrGLES2Texture *gles2_texture = WlrGLES2Texture::texture_from_wlr(
		_texture);
	gles2_flush_errors(NULL);

	RasterizerStorageGLES2::Texture *texture =
		gles2_texture->storage->texture_owner.getornull(gles2_texture->texture);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	gles2_flush_errors("glTexParameterf");
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	gles2_flush_errors("glPixelStorei");

	const struct gles2_pixel_format *fmt = gles2_texture->pixel_format;
	if (fmt->swizzle) {
		GLint swizzleMask[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
		glTexParameteriv(texture->target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	}

	glPixelStorei(GL_UNPACK_ROW_LENGTH, stride / (fmt->bpp / 8));
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, src_x);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, src_y);

	glTexSubImage2D(GL_TEXTURE_2D, 0, dst_x, dst_y, width, height,
		fmt->gl_format, fmt->gl_type, data);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	return true;
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

WlrGLES2Texture::WlrGLES2Texture(RasterizerStorageGLES2 *p_storage,
		RID p_texture, int width, int height,
		const struct gles2_pixel_format *fmt) {
	wlr_texture_init(&state.wlr_texture, &texture_impl);
	state.godot_texture = this;
	storage = p_storage;
	texture = p_texture;
	pixel_format = fmt;
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
	return pixel_format->has_alpha;
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
