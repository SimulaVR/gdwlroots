#ifndef GDWLR_GLES3_RENDERER_H
#define GDWLR_GLES3_RENDERER_H
#include "drivers/gles3/rasterizer_gles3.h"
#include "scene/resources/texture.h"
#include "renderer.h"
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#undef static
}

struct gles3_pixel_format {
	enum wl_shm_format wl_format;
	GLint gl_format, gl_type;
	int depth, bpp;
	bool has_alpha;
	bool swizzle;
};

class WlrGLES3Texture : public Texture {
	GDCLASS(WlrGLES3Texture, Texture);
	RES_BASE_EXTENSION("wlrtex");

	struct texture_state {
		struct wlr_texture wlr_texture;
		struct WlrGLES3Texture *godot_texture;
	};
	struct texture_state state;

	const struct gles3_pixel_format *pixel_format;
	RasterizerStorageGLES3 *storage;
	RID texture;
	int w, h;
	uint32_t flags;

public:
	static WlrGLES3Texture *texture_from_wlr(struct wlr_texture *texture);

	struct wlr_texture *get_wlr_texture();

	static void wlr_texture_get_size(
			struct wlr_texture *_texture, int *width, int *height);
	static bool wlr_texture_write_pixels(struct wlr_texture *_texture,
			uint32_t stride, uint32_t width, uint32_t height,
			uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
			const void *data);
	static void wlr_texture_destroy(struct wlr_texture *texture);

	int get_width() const;
	int get_height() const;
	RID get_rid() const;
	bool has_alpha() const;
	uint32_t get_flags() const;
	void set_flags(uint32_t p_flags);

	WlrGLES3Texture(RasterizerStorageGLES3 *storage,
			RID texture, int width, int height,
			const struct gles3_pixel_format *pixel_format);
};

class WlrGLES3Renderer : public WlrRenderer {
	RasterizerGLES3 *rasterizer;

	/* Hack necessary for moving pointers between wlroots and godot */
	struct renderer_state {
		struct wlr_renderer wlr_renderer;
		WlrGLES3Renderer *godot_renderer;
	};

	struct renderer_state renderer_state;

public:
	static struct wlr_texture *texture_from_pixels(
			struct wlr_renderer *_renderer, enum wl_shm_format fmt,
			uint32_t stride, uint32_t width, uint32_t height, const void *data);

	virtual struct wlr_renderer *get_wlr_renderer();
	virtual Texture *texture_from_wlr(struct wlr_texture *texture);

	WlrGLES3Renderer(RasterizerGLES3 *rasterizer);
	~WlrGLES3Renderer();
};

#endif
