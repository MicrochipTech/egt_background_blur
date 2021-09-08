/*
 * Copyright (C) 2021 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <egt/ui>
#include <cairo/cairo.h>
#include <iostream>
#include "sideboard2.h"

#define KERNEL_SIZE 17
#define KERNEL_HALF_SIZE 8

static std::shared_ptr<egt::ImageLabel> blur_background(egt::Application& app)
{
	auto surface = egt::shared_cairo_surface_t(
						cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
												   app.screen()->size().width(),
												   app.screen()->size().height()),
						cairo_surface_destroy);
	auto cr = egt::shared_cairo_t(cairo_create(surface.get()), cairo_destroy);

	// render the screen to the new surface
	egt::Painter painter(cr);
	for (auto& w : app.windows()) {
		if (!w->visible())
			continue;

		// draw top level frames and plane frames
		if (w->top_level() || w->plane_window())
			w->paint(painter);
	}

	// we now have a copy of the top level screen to perform the blur on
	cairo_surface_t* tmpSurface;
	int width, height;
	int src_stride, dst_stride;
	int x, y, z, w;
	uint8_t *src, *dst;
	uint32_t *s, *d, p;
	int i, j, k;
	// use a pre-computed Gaussian kernel
	const uint32_t a = 0x2EC;
	const uint8_t kernel[17] = { 9, 15, 24, 34, 46, 59, 70, 77, 80, 77, 70, 59, 46, 34, 24, 15, 9 };

	width = app.screen()->size().width();
	height = app.screen()->size().height();

	// screen surface will be ARGB32
    tmpSurface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

    src = cairo_image_surface_get_data (surface.get());
    src_stride = cairo_image_surface_get_stride (surface.get());

    dst = cairo_image_surface_get_data (tmpSurface);
    dst_stride = cairo_image_surface_get_stride (tmpSurface);

    /* Horizontally blur from surface -> tmp */
    for (i = 0; i < height; i++) {
		s = (uint32_t *) (src + i * src_stride);
		d = (uint32_t *) (dst + i * dst_stride);

		for (j = 0; j < width; j++) {
			x = y = z = w = 0;
			for (k = 0; k < KERNEL_SIZE; k++) {
				if ((j - KERNEL_HALF_SIZE + k < 0) || (j - KERNEL_HALF_SIZE + k >= width))
					continue;

				p = s[j - KERNEL_HALF_SIZE + k];

				x += ((p >> 24) & 0xff) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >>  8) & 0xff) * kernel[k];
				w += ((p >>  0) & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
    }

    /* Then vertically blur from tmp -> surface */
    for (i = 0; i < height; i++) {
		s = (uint32_t *) (dst + i * dst_stride);
		d = (uint32_t *) (src + i * src_stride);

		for (j = 0; j < width; j++) {
			x = y = z = w = 0;
			for (k = 0; k < KERNEL_SIZE; k++) {
				if ((i - KERNEL_HALF_SIZE + k < 0) || (i - KERNEL_HALF_SIZE + k >= height))
					continue;

				s = (uint32_t *) (dst + (i - KERNEL_HALF_SIZE + k) * dst_stride);
				p = s[j];

				x += ((p >> 24) & 0xff) * kernel[k];
				y += ((p >> 16) & 0xff) * kernel[k];
				z += ((p >>  8) & 0xff) * kernel[k];
				w += ((p >>  0) & 0xff) * kernel[k];
			}
			d[j] = (x / a << 24) | (y / a << 16) | (z / a << 8) | w / a;
		}
    }

    // destroy the temporary surface and tell Cairo we changed the image
    cairo_surface_destroy (tmpSurface);
    cairo_surface_mark_dirty (surface.get());

	// create an ImageLabel object
	egt::Image image(surface);
	auto imageLabel = std::make_shared<egt::ImageLabel>(image);
	return imageLabel;
}

int main(int argc, char** argv)
{
    egt::Application app(argc, argv);

    egt::TopWindow win;

    auto create_label = [](const std::string & text)
    {
        auto label = std::make_shared<egt::Label>(text);
        label->font(egt::Font(30));
        label->align(egt::AlignFlag::center);
        return label;
    };

    auto label = std::make_shared<egt::ImageLabel>(
                     egt::Image("icon:egt_logo_black.png;128"),
                     "SideBoard Widget");
    label->font(egt::Font(28));
    label->fill_flags().clear();
    label->align(egt::AlignFlag::center);
    label->image_align(egt::AlignFlag::top);
    win.add(label);

    // a label on the main screen
    auto mainLabel = std::make_shared<egt::Label>(win, "0, 0", egt::Rect(360, 300, 80, 40));
    mainLabel->align(egt::AlignFlag::center_horizontal);

    // a button on the main screen
    auto mainButton = std::make_shared<egt::Button>(win, "Main Button", egt::Rect(360, 360, 80, 40));
    mainButton->align(egt::AlignFlag::center_horizontal);


    egt::SideBoard2 board0(egt::SideBoard2::PositionFlag::left, egt::Size(140, 0),
                          egt::WindowHint::software);
    board0.color(egt::Palette::ColorId::bg, egt::Palette::antiquewhite);
    board0.add(create_label("LEFT"));
    win.add(board0);
    board0.show();

    board0.on_event([&app, &mainLabel, &board0, &win] (egt::Event& event) {
    	static std::shared_ptr<egt::ImageLabel> blurredImage = nullptr;

    	switch (event.id())
    	{
    		case egt::EventId::pointer_click:
    	    	mainLabel->text("board0: " + egt::detail::to_string(event.pointer().point));

    	    	if (!board0.is_open()) {
    	    		// sideboard is opening
    	    		mainLabel->text("Open");
    	    		blurredImage = blur_background(app);
    	    		win.add(blurredImage);
    	    		board0.zorder_top();
    	    	} else {
    	    		// sideboard is closing
    	    		win.remove(blurredImage.get());
    	    		mainLabel->text("Close");
    	    		win.damage();
    	    	}

    			break;
    		default:
    			break;
    	}
    });

    // add a button to the sideboard
    egt::Button buttonLeft(board0, "Button1", egt::Rect(20, 100, 80, 40));

    win.on_event([mainLabel] (egt::Event& event) {
    	switch (event.id())
    	{
    		case egt::EventId::pointer_click:
    			mainLabel->text("win: " + egt::detail::to_string(event.pointer().point));
    			break;
    		default:
    			break;
    	}
    });

    egt::SideBoard2 board1(egt::SideBoard2::PositionFlag::bottom, egt::Size(0, 200));
    board1.color(egt::Palette::ColorId::bg, egt::Palette::blue);
    board1.add(create_label("BOTTOM"));
    win.add(board1);
    board1.show();

    egt::SideBoard2 board2(egt::SideBoard2::PositionFlag::right, egt::Size(200, 0));
    board2.color(egt::Palette::ColorId::bg, egt::Palette::green);
    board2.add(create_label("RIGHT"));
    win.add(board2);
    board2.show();

    egt::SideBoard2 board3(egt::SideBoard2::PositionFlag::top, egt::Size(0, 200));
    board3.color(egt::Palette::ColorId::bg, egt::Palette::gray);
    board3.add(create_label("TOP"));
    win.add(board3);
    board3.show();

    win.show();

    return app.run();
}
