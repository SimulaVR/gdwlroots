#include <stdint.h>
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/rasterizer_storage_gles3.h"
#include "gles3_renderer.h"
#include<string>
#include <iostream>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "core/os/os.h"

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

// Apparently needed for GLES2/gl2ext.h import
#ifndef GL_APIENTRY
#define GL_APIENTRY
#endif
#include <GLES2/gl2ext.h>

#include <libdrm/drm_fourcc.h>

extern "C" {
// wlroots headers use C99 stuff that will cause C++ compiler to complain unless we wrap in #static
#define static
#include <wayland-server.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/interface.h>
#include <wlr/util/log.h>
#undef static
}

// Launch with SIMULA_DMA_DEBUG=1 to get dma buffer log statements/verification that dma buffers are being used
static bool dma_debug_enabled() {
	static int cached = -1;
	if (cached == -1) {
		const char *value = getenv("SIMULA_DMA_DEBUG");
		bool enabled = value != NULL && *value != '\0' &&
				strcmp(value, "0") != 0 &&
				strcasecmp(value, "false") != 0 &&
				strcasecmp(value, "off") != 0;
		cached = enabled ? 1 : 0;
	}
	return cached == 1;
}

void log_debug_dma(const char *fmt, ...) {
	if (!dma_debug_enabled()) {
		return;
	}

	FILE *f = fopen("/tmp/simula_dma_log.txt", "a");
	if (f) {
		va_list args;
		va_start(args, fmt);
		vfprintf(f, fmt, args);
		va_end(args);
		fclose(f);
	}
}

static bool egl_has_extension(const char *exts, const char *name) {
	if (!exts || !name || !*name) {
		return false;
	}
	const char *start = exts;
	while ((start = strstr(start, name)) != nullptr) {
		const char *end = start + strlen(name);
		if ((start == exts || start[-1] == ' ') && (*end == ' ' || *end == '\0')) {
			return true;
		}
		start = end;
	}
	return false;
}

extern "C" {

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

static void configure_imported_texture_as_raw_rgba(
		RasterizerStorageGLES3::Texture *texture,
		int width, int height, GLenum gl_type) {
	texture->width = width;
	texture->height = height;
	texture->alloc_width = width;
	texture->alloc_height = height;
	texture->depth = 0;
	texture->alloc_depth = 0;
	texture->format = Image::FORMAT_RGBA8;
	texture->type = VS::TEXTURE_TYPE_2D;
	texture->target = GL_TEXTURE_2D;
	texture->gl_format_cache = GL_RGBA;
	texture->gl_internal_format_cache = GL_RGBA8;
	texture->gl_type_cache = gl_type;
	texture->compressed = false;
	texture->srgb = false;
	texture->using_srgb = false;
	texture->active = true;
}

static const enum wl_shm_format *renderer_formats(
		struct wlr_renderer *renderer, size_t *len) {
	log_debug_dma("DEBUG: renderer_formats called\n");
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

	log_debug_dma("DEBUG: texture_from_pixels called (%dx%d)\n", width, height);

	struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)_renderer;
	WlrGLES3Renderer *renderer = state->godot_renderer;
	renderer->try_bind_dmabuf_egl();
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

	const struct gles3_pixel_format *fmt = get_gles3_format_from_wl(wl_fmt);
	if (fmt == NULL) {
		wlr_log(WLR_ERROR, "Unsupported pixel format %" PRIu32, wl_fmt);
		return NULL;
	}

	configure_imported_texture_as_raw_rgba(texture, width, height, fmt->gl_type);
	texture->data_size = width * height * 4; //for savePng

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, fmt->gl_type, data);
	gles3_flush_errors("glTexImage2D");


	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	generateMipmaps(texture, width, height, 0);

	WlrGLES3Texture *wlr_texture = memnew(WlrGLES3Texture(storage, rid, width, height, fmt));
	wlr_texture->reference();
	return wlr_texture->get_wlr_texture();
}

int WlrGLES3Renderer::get_dmabuf_formats(
		struct wlr_renderer *renderer, int **formats) {
  	log_debug_dma("DEBUG: get_dmabuf_formats called\n");

		// Get EGLDisplay
    struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)renderer;
    WlrGLES3Renderer *godot_renderer = state->godot_renderer;
    godot_renderer->try_bind_dmabuf_egl();
    EGLDisplay display = godot_renderer->egl_display;

		if (display == EGL_NO_DISPLAY) {
			log_debug_dma("DEBUG: get_dmabuf_formats: EGL display not ready\n");
			return -1;
		}

		// Attempt to get supported formats (or else fall back to a conservative list if they're unavailable)
    PFNEGLQUERYDMABUFFORMATSEXTPROC eglQueryDmaBufFormatsEXT = (PFNEGLQUERYDMABUFFORMATSEXTPROC)eglGetProcAddress("eglQueryDmaBufFormatsEXT");

    if (!eglQueryDmaBufFormatsEXT) {
        log_debug_dma("DEBUG: eglQueryDmaBufFormatsEXT not supported, using fallbacks\n");
        static const int supported_formats[] = {
            DRM_FORMAT_ARGB8888,
            DRM_FORMAT_XRGB8888,
            DRM_FORMAT_ABGR8888,
            DRM_FORMAT_XBGR8888,
        };
        *formats = (int *)calloc(sizeof(supported_formats) / sizeof(supported_formats[0]), sizeof(int));
        for (size_t i = 0; i < sizeof(supported_formats) / sizeof(supported_formats[0]); ++i) {
            (*formats)[i] = supported_formats[i];
        }
        return sizeof(supported_formats) / sizeof(supported_formats[0]);
    }

    EGLint num;
    if (!eglQueryDmaBufFormatsEXT(display, 0, NULL, &num)) {
        return -1;
    }

    *formats = (int *)calloc(num, sizeof(int));
    if (!eglQueryDmaBufFormatsEXT(display, num, *formats, &num)) {
        free(*formats);
        return -1;
    }

    return num;
}

// Probe EGL/our driver for supported modifiers (s.t. "modifier" is EGL speak for how pixels are laid out in memory)
int WlrGLES3Renderer::get_dmabuf_modifiers(
		struct wlr_renderer *renderer, int format, uint64_t **modifiers) {
	log_debug_dma("DEBUG: get_dmabuf_modifiers called for format %d\n", format);

	  //Get state
    struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)renderer;
    WlrGLES3Renderer *godot_renderer = state->godot_renderer;
    godot_renderer->try_bind_dmabuf_egl();
    EGLDisplay display = godot_renderer->egl_display;

		if (display == EGL_NO_DISPLAY) {
			log_debug_dma("DEBUG: get_dmabuf_modifiers: EGL display not ready\n");
			return -1;
		}

		// Probe for modifiers (falling back to linear if querying isn't supported)
    PFNEGLQUERYDMABUFMODIFIERSEXTPROC eglQueryDmaBufModifiersEXT = (PFNEGLQUERYDMABUFMODIFIERSEXTPROC)eglGetProcAddress("eglQueryDmaBufModifiersEXT");

    if (!eglQueryDmaBufModifiersEXT) {
        log_debug_dma("DEBUG: eglQueryDmaBufModifiersEXT not supported, using linear only\n");
        *modifiers = (uint64_t *)calloc(1, sizeof(uint64_t));
        (*modifiers)[0] = DRM_FORMAT_MOD_LINEAR;
        return 1;
    }

    EGLint num;
    if (!eglQueryDmaBufModifiersEXT(display, format, 0, NULL, NULL, &num)) {
        return -1;
    }

    *modifiers = (uint64_t *)calloc(num, sizeof(uint64_t));
    if (!eglQueryDmaBufModifiersEXT(display, format, num, *modifiers, NULL, &num)) {
        free(*modifiers);
        return -1;
    }

    return num;
}

struct wlr_texture *WlrGLES3Renderer::texture_from_dmabuf(
		struct wlr_renderer *_renderer, struct wlr_dmabuf_attributes *attribs) {

	log_debug_dma("DEBUG: texture_from_dmabuf called (fmt=%u %ux%u planes=%d)\n",
		attribs->format, attribs->width, attribs->height, attribs->n_planes);

	struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)_renderer;
	WlrGLES3Renderer *renderer = state->godot_renderer;
	renderer->try_bind_dmabuf_egl();
	auto storage =
		(RasterizerStorageGLES3 *)renderer->rasterizer->get_storage();
	if (renderer->egl_display == EGL_NO_DISPLAY) {
		log_debug_dma("DEBUG: texture_from_dmabuf: EGL display not ready\n");
		return NULL;
	}

	//eglCreateImageKHR basically (i) labels the graphical data as shared and (ii) creates a handle which other things can use to access it
	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
	//glEGLImageTargetTexture2DOES is used to tell the OpenGL state machine that the shared image is the currently bound/active texture
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	if (!eglCreateImageKHR || !glEGLImageTargetTexture2DOES) {
		wlr_log(WLR_ERROR, "Missing EGL/GL extensions for dmabuf");
		return NULL;
	}

	EGLDisplay display = renderer->egl_display;
	const char *exts = eglQueryString(display, EGL_EXTENSIONS);
	bool has_modifiers_ext = egl_has_extension(exts, "EGL_EXT_image_dma_buf_import_modifiers");
	bool has_modifier = false;
	if (attribs->modifier != DRM_FORMAT_MOD_INVALID &&
			attribs->modifier != DRM_FORMAT_MOD_LINEAR) {
		if (!has_modifiers_ext) {
			wlr_log(WLR_ERROR, "dmabuf modifiers extension not present");
			return NULL;
		}
		has_modifier = true;
	}

	//attrib_list encodes the layout of the dma buffers which we feed to eglCreateImageKHR
	EGLint attrib_list[50];
	int i = 0;
	attrib_list[i++] = EGL_WIDTH;
	attrib_list[i++] = attribs->width;
	attrib_list[i++] = EGL_HEIGHT;
	attrib_list[i++] = attribs->height;
	attrib_list[i++] = EGL_LINUX_DRM_FOURCC_EXT;
	attrib_list[i++] = attribs->format;

	//DMA buffers have 4 planes, each with this data conforming to this struct
	struct {
		int fd;
		int offset_idx;
		int pitch_idx;
		int mod_lo;
		int mod_hi;
	} planes[] = {
		{ EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT },
		{ EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT },
		{ EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT },
		{ EGL_DMA_BUF_PLANE3_FD_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT },
	};

	for (int plane = 0; plane < attribs->n_planes; ++plane) {
		attrib_list[i++] = planes[plane].fd;
		attrib_list[i++] = attribs->fd[plane];
		attrib_list[i++] = planes[plane].offset_idx;
		attrib_list[i++] = attribs->offset[plane];
		attrib_list[i++] = planes[plane].pitch_idx;
		attrib_list[i++] = attribs->stride[plane];
		if (has_modifier) {
			attrib_list[i++] = planes[plane].mod_lo;
			attrib_list[i++] = attribs->modifier & 0xFFFFFFFF;
			attrib_list[i++] = planes[plane].mod_hi;
			attrib_list[i++] = attribs->modifier >> 32;
		}
	}
	attrib_list[i++] = EGL_NONE;

	// Creates the shared EGL image handle of the dma buffer
	EGLImageKHR image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attrib_list);
	if (image == EGL_NO_IMAGE_KHR) {
		wlr_log(WLR_ERROR, "Failed to create EGLImage (err=0x%x, fmt=%u %ux%u planes=%d)",
				eglGetError(), attribs->format, attribs->width, attribs->height, attribs->n_planes);
		return NULL;
	}

	// Not sure if this managed right. Be weary of potential leaks
	RID rid = storage->texture_create();
	RasterizerStorageGLES3::Texture *texture =
		storage->texture_owner.getornull(rid);

	// Create an OpenGL texture object, make it active in the state machine, and have it point towards the dma buffer
	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

	// Make the texture have tehse paramaters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Treat imported client buffers as raw RGBA bytes and decode once in the
	// final quad shader instead of relying on Godot's sRGB texture metadata.
	texture->tex_id = tex_id;
	configure_imported_texture_as_raw_rgba(
			texture, attribs->width, attribs->height, GL_UNSIGNED_BYTE);

	// We use dummy wl_shm_format data here since we're using dma buffers (which carry their own format meta-data)
	static const struct gles3_pixel_format dummy_fmt = {
		.wl_format = (wl_shm_format)attribs->format,
		.gl_format = GL_RGBA,
		.gl_type = GL_UNSIGNED_BYTE,
		.depth = 24,
		.bpp = 32,
		.has_alpha = true,
		.swizzle = false
	};

	// Return a new heap allocated texture; actually not super sure this gets cleaned up so be on lookout
	WlrGLES3Texture *wlr_texture = memnew(WlrGLES3Texture(storage, rid, attribs->width, attribs->height, &dummy_fmt));
	wlr_texture->egl_image = image;
	wlr_texture->egl_display = display;
	wlr_texture->reference();
	log_debug_dma("DEBUG: texture_from_dmabuf success\n");

	return wlr_texture->get_wlr_texture();
}

static void generateMipmaps(RasterizerStorageGLES3::Texture* texture, int width, int height, int level) {
	return;
}

static void renderer_init_wl_display(struct wlr_renderer *renderer,
		struct wl_display *wl_display) {
	log_debug_dma("DEBUG: renderer_init_wl_display called (EGL current display)\n");

	struct WlrGLES3Renderer::renderer_state *state =
		(struct WlrGLES3Renderer::renderer_state *)renderer;
	WlrGLES3Renderer *godot_renderer = state->godot_renderer;
	godot_renderer->pending_wl_display = wl_display;
	godot_renderer->try_bind_dmabuf_egl();
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
	.get_dmabuf_formats = WlrGLES3Renderer::get_dmabuf_formats,
	.get_dmabuf_modifiers = WlrGLES3Renderer::get_dmabuf_modifiers,
	.texture_from_pixels = WlrGLES3Renderer::texture_from_pixels,
	.texture_from_dmabuf = WlrGLES3Renderer::texture_from_dmabuf,
	.init_wl_display = renderer_init_wl_display,
};

}

struct wlr_renderer *WlrGLES3Renderer::get_wlr_renderer() {
	return &renderer_state.wlr_renderer;
}

WlrGLES3Renderer::WlrGLES3Renderer(RasterizerGLES3 *p_rasterizer) {
	log_debug_dma("DEBUG: WlrGLES3Renderer constructor called\n");
	rasterizer = p_rasterizer;
	wlr_renderer_init(&renderer_state.wlr_renderer, &renderer_impl);
	renderer_state.godot_renderer = this;
	WlrRenderer::singleton = this;
	egl_display = EGL_NO_DISPLAY;
	pending_wl_display = NULL;
	dmabuf_bound = false;
	dmabuf_disabled = false;
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
	log_debug_dma("DEBUG: wlr_texture_write_pixels called (%dx%d)\n", width, height);
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
	if (texture == NULL) {
		return;
	}
	texture->release_render_resources();
	if (texture->unreference()) {
		memdelete(texture);
	}
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

void WlrGLES3Renderer::try_bind_dmabuf_egl() {
	if (dmabuf_bound || dmabuf_disabled || pending_wl_display == NULL) {
		return;
	}

	// Attempt to get EGL display on current thread
	EGLDisplay display = eglGetCurrentDisplay();

	// If this fails, try a bunch of other ways to get an EGL display
	if (display == EGL_NO_DISPLAY) {
		void *native_display = OS::get_singleton()->get_native_handle(OS::DISPLAY_HANDLE);
		log_debug_dma("DEBUG: eglGetCurrentDisplay = EGL_NO_DISPLAY; native_display=%p\n", native_display);

		PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =
				(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
		if (!eglGetPlatformDisplayEXT) {
			log_debug_dma("DEBUG: eglGetPlatformDisplayEXT not available\n");
		} else if (native_display) {
			bool tried_platform = false;
// An LLM suggested we bifurcate on these extension guards, though honestly I don't have a good grip of whether we need them both.
// Looks like our Godot EGL backend uses EGL_PLATFORM_X11_EXT, while wlroots uses EGL_PLATFORM_X11_KHR.
#ifdef EGL_PLATFORM_X11_EXT
			tried_platform = true;
			eglGetError();
			display = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT, native_display, nullptr);
			if (display == EGL_NO_DISPLAY) {
				EGLint err = eglGetError();
				log_debug_dma("DEBUG: eglGetPlatformDisplayEXT(X11_EXT) failed (err=0x%x)\n", err);
			}
#endif
#ifdef EGL_PLATFORM_X11_KHR
			if (display == EGL_NO_DISPLAY) {
				tried_platform = true;
				eglGetError();
				display = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_KHR, native_display, nullptr);
				if (display == EGL_NO_DISPLAY) {
					EGLint err = eglGetError();
					log_debug_dma("DEBUG: eglGetPlatformDisplayEXT(X11_KHR) failed (err=0x%x)\n", err);
				}
			}
#endif
			if (!tried_platform) {
				log_debug_dma("DEBUG: EGL_PLATFORM_X11_EXT/KHR not defined by EGL headers\n");
			}
			if (display != EGL_NO_DISPLAY) {
				log_debug_dma("DEBUG: eglGetPlatformDisplayEXT returned display=%p\n", (void *)display);
			}
		}

		if (display == EGL_NO_DISPLAY && native_display) {
			eglGetError();
			display = eglGetDisplay((EGLNativeDisplayType)native_display);
			if (display == EGL_NO_DISPLAY) {
				EGLint err = eglGetError();
				log_debug_dma("DEBUG: eglGetDisplay(native) failed (err=0x%x)\n", err);
			}
		}
		if (display == EGL_NO_DISPLAY) {
			eglGetError();
			display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
			if (display == EGL_NO_DISPLAY) {
				EGLint err = eglGetError();
				log_debug_dma("DEBUG: eglGetDisplay(default) failed (err=0x%x)\n", err);
			}
		}
		if (display == EGL_NO_DISPLAY) {
			log_debug_dma("DEBUG: eglGetDisplay failed; deferring dmabuf EGL bind\n");
			return;
		}
	}

	EGLint major, minor;
	if (!eglInitialize(display, &major, &minor)) {
		log_debug_dma("DEBUG: eglInitialize failed (0x%x) on display=%p\n", eglGetError(), (void *)display);
		return;
	}
	log_debug_dma("DEBUG: EGL Initialized %d.%d (current display)\n", major, minor);

	// Use GLES
	EGLenum egl_api = EGL_OPENGL_ES_API;
#ifdef GLES_OVER_GL
	egl_api = EGL_OPENGL_API;
#endif
	if (!eglBindAPI(egl_api)) {
		log_debug_dma("DEBUG: eglBindAPI failed (0x%x)\n", eglGetError());
	}

	egl_display = display;

	// Get supported EGL extensions for a debug print
	const char *exts = eglQueryString(display, EGL_EXTENSIONS);
	log_debug_dma("DEBUG: EGL Extensions: %s\n", exts ? exts : "NULL");

	if (!egl_has_extension(exts, "EGL_WL_bind_wayland_display")) {
		log_debug_dma("DEBUG: EGL_WL_bind_wayland_display not advertised; disabling dmabuf support\n");
		egl_display = EGL_NO_DISPLAY;
		dmabuf_disabled = true;
		return;
	}

	// Bind EGL to Wayland display to support wl_drm/dmabuf.
	PFNEGLBINDWAYLANDDISPLAYWL eglBindWaylandDisplayWL =
		(PFNEGLBINDWAYLANDDISPLAYWL)eglGetProcAddress("eglBindWaylandDisplayWL");
	if (eglBindWaylandDisplayWL) {
		if (eglBindWaylandDisplayWL(display, pending_wl_display)) {
			log_debug_dma("DEBUG: eglBindWaylandDisplayWL succeeded\n");
			dmabuf_bound = true;
		} else {
			log_debug_dma("DEBUG: eglBindWaylandDisplayWL failed; disabling dmabuf support\n");
			egl_display = EGL_NO_DISPLAY;
			dmabuf_disabled = true;
		}
	} else {
		log_debug_dma("DEBUG: eglBindWaylandDisplayWL symbol unavailable; disabling dmabuf support\n");
		egl_display = EGL_NO_DISPLAY;
		dmabuf_disabled = true;
	}
}

Texture *WlrGLES3Renderer::texture_from_wlr(struct wlr_texture *texture) {
	return WlrGLES3Texture::texture_from_wlr(texture);
}

bool WlrGLES3Renderer::is_dmabuf_available() const {
	return dmabuf_bound && !dmabuf_disabled && egl_display != EGL_NO_DISPLAY;
}

WlrGLES3Texture::WlrGLES3Texture(RasterizerStorageGLES3 *p_storage,
		RID p_texture, int width, int height,
		const struct gles3_pixel_format *fmt) {
	log_debug_dma("DEBUG: WlrGLES3Texture constructor called (%dx%d)\n", width, height);
	wlr_texture_init(&state.wlr_texture, &texture_impl);
	state.godot_texture = this;
	storage = p_storage;
	texture = p_texture;
	pixel_format = fmt;
	w = width;
	h = height;
	resources_released = false;
	egl_image = EGL_NO_IMAGE_KHR;
	egl_display = EGL_NO_DISPLAY;
}

void WlrGLES3Texture::release_render_resources() {
	if (resources_released) {
		return;
	}
	resources_released = true;

	if (egl_image != EGL_NO_IMAGE_KHR && egl_display != EGL_NO_DISPLAY) {
		PFNEGLDESTROYIMAGEKHRPROC destroy_image = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
		if (destroy_image) {
			destroy_image(egl_display, egl_image);
		}
		egl_image = EGL_NO_IMAGE_KHR;
		egl_display = EGL_NO_DISPLAY;
	}

	if (texture.is_valid()) {
		VS *vs = VS::get_singleton();
		if (vs != nullptr) {
			vs->free(texture);
		} else if (storage != nullptr) {
			storage->free(texture);
		}
		texture = RID();
	}
}

WlrGLES3Texture::~WlrGLES3Texture() {
	release_render_resources();
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
