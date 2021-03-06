#ifndef GDWLR_RD_RENDERER_H
#define GDWLR_RD_RENDERER_H
#include "servers/rendering/rendering_device.h"
#include "scene/resources/texture.h"
#include "renderer.h"
#include "core/io/image.h"

namespace wlr {
extern "C" {
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#undef static
}
}

struct rd_pixel_format {
	enum wlr::wl_shm_format wl_format;
	Image::Format img_format;
	uint32_t size;
	bool has_alpha;
	RD::DataFormat rd_format;
	RD::TextureSwizzle rd_swizzle_r;
        RD::TextureSwizzle rd_swizzle_g;
        RD::TextureSwizzle rd_swizzle_b;
        RD::TextureSwizzle rd_swizzle_a;

};

class WlrRDTexture : public Texture {
	GDCLASS(WlrRDTexture, Texture);
	RES_BASE_EXTENSION("wlrtex");

	struct texture_state {
		struct wlr::wlr_texture wlr_texture;
		struct WlrRDTexture *godot_texture;
	};
	struct texture_state state;

	const struct rd_pixel_format *pixel_format;
	RID texture;
	int w, h;
	uint32_t flags;

public:
	static WlrRDTexture *texture_from_wlr(struct wlr::wlr_texture *texture);

	struct wlr::wlr_texture *get_wlr_texture();

	static void wlr_texture_get_size(
			struct wlr::wlr_texture *_texture, int *width, int *height);
	static bool wlr_texture_write_pixels(struct wlr::wlr_texture *_texture,
			uint32_t stride, uint32_t width, uint32_t height,
			uint32_t src_x, uint32_t src_y, uint32_t dst_x, uint32_t dst_y,
			const void *data);
	static void wlr_texture_destroy(struct wlr::wlr_texture *texture);

	int get_width() const;
	int get_height() const;
	RID get_rid() const;
	bool has_alpha() const;
	uint32_t get_flags() const;
	void set_flags(uint32_t p_flags);

	WlrRDTexture(RID texture, int width, int height,
			const struct rd_pixel_format *pixel_format);
};

class WlrRDRenderer : public WlrRenderer {
	/* Hack necessary for moving pointers between wlroots and godot */
	struct renderer_state {
		struct wlr::wlr_renderer wlr_renderer;
		WlrRDRenderer *godot_renderer;
	};

	struct renderer_state renderer_state;

public:
	static struct wlr::wlr_texture *texture_from_pixels(
			struct wlr::wlr_renderer *_renderer, enum wlr::wl_shm_format fmt,
			uint32_t stride, uint32_t width, uint32_t height, const void *data);

	virtual struct wlr::wlr_renderer *get_wlr_renderer();
	virtual Texture *texture_from_wlr(struct wlr::wlr_texture *texture);

	WlrRDRenderer();
	~WlrRDRenderer();
};

#endif
