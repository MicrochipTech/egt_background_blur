/*
 * Copyright (C) 2021 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <egt/ui>
#include <cairo/cairo.h>
#include <iostream>
#include <sys/time.h>
#include "sideboard2.h"

#define KERNEL_SIZE 17
#define KERNEL_HALF_SIZE 8

static std::shared_ptr<egt::ImageLabel> blur_background_gaussian(egt::Application& app)
{
    int timediff = 0;
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

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

    gettimeofday(&time2, NULL);
    timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    std::cout << "Repaint screen to surface: " << timediff << "us" << std::endl;
    gettimeofday(&time1, NULL);

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

    gettimeofday(&time2, NULL);
    timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    std::cout << "Gaussian blur: " << timediff << "us" << std::endl;

	// create an ImageLabel object
	egt::Image image(surface);
	auto imageLabel = std::make_shared<egt::ImageLabel>(image);
	return imageLabel;
}

static std::shared_ptr<egt::ImageLabel> blur_background_box(egt::Application& app)
{
    int timediff = 0;
    struct timeval time1, time2;
    gettimeofday(&time1, NULL);

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

    gettimeofday(&time2, NULL);
    timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    std::cout << "Repaint screen to surface: " << timediff << "us" << std::endl;
    gettimeofday(&time1, NULL);

    // perform a box blur
    const int MAX_ITERATIONS = 3;
    int iteration;
	int width, height;
	uint8_t *src, *dst;
	int src_stride, dst_stride;
	int i, j;
	uint32_t *d, p;

	width = app.screen()->size().width();
	height = app.screen()->size().height();

	// we now have a copy of the top level screen to perform the blur on
	cairo_surface_t* tmpSurface;
	tmpSurface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

    src = cairo_image_surface_get_data (surface.get());
    src_stride = cairo_image_surface_get_stride (surface.get());
    dst = cairo_image_surface_get_data (tmpSurface);
    dst_stride = cairo_image_surface_get_stride (tmpSurface);

    // the ARGB components, order does not matter
    int tx, ty, tz, tw;
    uint32_t *s0, *s1, *s2; // these are the 3 pointers for the sliding window

    for(iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
    	// horizontally blur from src->tmp
    	for(i = 0; i < height; i++) {
    		d = (uint32_t*) (dst + i * dst_stride);

    		// s0 is off screen
    		s1 = (uint32_t*) (src + i * src_stride);
    		s2 = s1 + 1;

    		// calculate d(0) = s(n-1) + s(n) + s(n+1)
    		// s(n-1) = 0 for first entry
    		// preset accumulators and set d(0)
    		p = *s1;
			tx = (p >> 24) & 0xff;
			ty = (p >> 16) & 0xff;
			tz = (p >>  8) & 0xff;
			tw = (p >>  0) & 0xff;

			p = *s2;
			tx += (p >> 24) & 0xff;
			ty += (p >> 16) & 0xff;
			tz += (p >>  8) & 0xff;
			tw += (p >>  0) & 0xff;
			*d++ = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;

			// increment pointers and set first s(n-1)
			s0 = (uint32_t*) (src + i * src_stride);
			s2++;

			// calculate d(1)
			// subtract s(n-1) not required as it is zero
			p = *s2;
			tx += (p >> 24) & 0xff;
			ty += (p >> 16) & 0xff;
			tz += (p >>  8) & 0xff;
			tw += (p >>  0) & 0xff;
			*d++ = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;

    		for(j = 2; j < (width - 1); j++) {
    			// subtract value for s(n-1)
    			p = *s0;
				tx -= (p >> 24) & 0xff;
				ty -= (p >> 16) & 0xff;
				tz -= (p >>  8) & 0xff;
				tw -= (p >>  0) & 0xff;

    			// update pointers
    			s0++;
    			s2++;

    			// add the channel values for s(n+1)
    			p = *s2;
				tx += (p >> 24) & 0xff;
				ty += (p >> 16) & 0xff;
				tz += (p >>  8) & 0xff;
				tw += (p >>  0) & 0xff;

    			*d++ = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;
    		}

    		// calculate last pixel in row
    		p = *s0;
			tx -= (p >> 24) & 0xff;
			ty -= (p >> 16) & 0xff;
			tz -= (p >>  8) & 0xff;
			tw -= (p >>  0) & 0xff;
			*d = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;
    	}

    	// vertically blur from tmp->src
    	for(j = 0; j < width; j++) {
    		// preset accumulators and set d(0) and d(1)
    		s1 = (uint32_t*) (dst); // remember we are copying back from the temporary buffer
    		s1 += j; // adjust pointer according to current width position
    		s2 = s1 + (dst_stride / 4);
    		s0 = s1; // used later, just pre-init for now

    		// do not add in s(n-1) as it is off-screen
			p = *s1;
			tx = (p >> 24) & 0xff;
			ty = (p >> 16) & 0xff;
			tz = (p >>  8) & 0xff;
			tw = (p >>  0) & 0xff;

			p = *s2;
			tx += (p >> 24) & 0xff;
			ty += (p >> 16) & 0xff;
			tz += (p >>  8) & 0xff;
			tw += (p >>  0) & 0xff;

			d = (uint32_t*) (src); // copying back into original surface
			d += j; // adjust for horizontal position
			*d = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;

			// increment pointers
			// subtract s(n-1) not required as it was zero
			s2 += dst_stride / 4; // advance by screen width, allowing for pixel density
			p = *s2;
			tx += (p >> 24) & 0xff;
			ty += (p >> 16) & 0xff;
			tz += (p >>  8) & 0xff;
			tw += (p >>  0) & 0xff;
			d += src_stride / 4;
			*d = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;

    		for(i = 2; i < (height - 1); i++) {
    			// subtract value for s(n-2)
    			p = *s0;
				tx -= (p >> 24) & 0xff;
				ty -= (p >> 16) & 0xff;
				tz -= (p >>  8) & 0xff;
				tw -= (p >>  0) & 0xff;

				// update pointers
				s0 += dst_stride / 4;
				s2 += dst_stride / 4;

    			// add the channel values for s(n+1)
    			p = *s2;
				tx += (p >> 24) & 0xff;
				ty += (p >> 16) & 0xff;
				tz += (p >>  8) & 0xff;
				tw += (p >>  0) & 0xff;

				d += src_stride / 4;
    			*d = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;
    		}

    		// calculate last pixel in column
			p = *s0;
			tx -= (p >> 24) & 0xff;
			ty -= (p >> 16) & 0xff;
			tz -= (p >>  8) & 0xff;
			tw -= (p >>  0) & 0xff;
			d += src_stride / 4;
			*d = (tx / 3 << 24) | (ty / 3 << 16) | (tz / 3 << 8) | tw / 3;
    	}
    }

    // destroy the temporary surface and tell Cairo we changed the image
    cairo_surface_destroy (tmpSurface);
    cairo_surface_mark_dirty (surface.get());

    gettimeofday(&time2, NULL);
    timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    std::cout << "Box blur: " << timediff << "us" << std::endl;

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
    	    		blurredImage = blur_background_box(app);
//    	    		blurredImage = blur_background_gaussian(app);
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
