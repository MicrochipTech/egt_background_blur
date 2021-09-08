/*
 * Copyright (C) 2021 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "egt/app.h"
#include "egt/detail/enum.h"
#include "egt/serialize.h"
#include "sideboard2.h"

namespace egt
{
inline namespace v1
{

static Size calculate_size(SideBoard2::PositionFlag position, Size requestedSize)
{
	Size appSize = Application::instance().screen()->size();
	Size finalSize;

	// calculate size of the sideboard based upon requested size and zero extents
	finalSize.width((requestedSize.width() == 0) ? appSize.width() : requestedSize.width());
	finalSize.height((requestedSize.height() == 0) ? appSize.height() : requestedSize.height());

	switch (position)
	{
		case SideBoard2::PositionFlag::left:
		case SideBoard2::PositionFlag::right:
			finalSize += Size(SideBoard2::HANDLE_WIDTH, 0);
			break;
		case SideBoard2::PositionFlag::top:
		case SideBoard2::PositionFlag::bottom:
			finalSize += Size(0, SideBoard2::HANDLE_WIDTH);
			break;
	}

	return finalSize;
}

SideBoard2::SideBoard2(PositionFlag position,
					 Size size,
                     WindowHint hint)
    : SideBoard2(position,
    			size,
                std::chrono::milliseconds(1000),
                easing_cubic_easeinout,
                std::chrono::milliseconds(1000),
                easing_circular_easeinout,
                hint)
{}

SideBoard2::SideBoard2(PositionFlag position,
					 Size size,
                     std::chrono::milliseconds open_duration,
                     EasingFunc open_func,
                     std::chrono::milliseconds close_duration,
                     EasingFunc close_func,
                     WindowHint hint)
    : Window(calculate_size(position, size), PixelFormat::rgb565, hint),
      m_position(position), m_size(size)
{
    m_oanim.duration(open_duration);
    m_canim.duration(close_duration);
    m_oanim.easing_func(std::move(open_func));
    m_canim.easing_func(std::move(close_func));

    initialize();
}

SideBoard2::SideBoard2(Serializer::Properties& props) noexcept
    : Window(props)
{
    m_oanim.duration(std::chrono::milliseconds(1000));
    m_canim.duration(std::chrono::milliseconds(1000));
    m_oanim.easing_func(std::move(easing_cubic_easeinout));
    m_canim.easing_func(std::move(easing_circular_easeinout));

    initialize();

    deserialize(props);
}

void SideBoard2::initialize()
{
    reset_animations();

    switch (m_position)
    {
    case PositionFlag::left:
    {
        m_oanim.on_change([this](PropertyAnimator::Value value) { x(value); });
        m_canim.on_change([this](PropertyAnimator::Value value) { x(value); });

        move(Point(m_oanim.starting(), 0));
        break;
    }
    case PositionFlag::right:
    {
        m_oanim.on_change([this](PropertyAnimator::Value value) { x(value); });
        m_canim.on_change([this](PropertyAnimator::Value value) { x(value); });

        move(Point(m_oanim.starting(), 0));
        break;
    }
    case PositionFlag::top:
    {
        m_oanim.on_change([this](PropertyAnimator::Value value) { y(value); });
        m_canim.on_change([this](PropertyAnimator::Value value) { y(value); });

        move(Point(0, m_oanim.starting()));
        break;
    }
    case PositionFlag::bottom:
    {
        m_oanim.on_change([this](PropertyAnimator::Value value) { y(value); });
        m_canim.on_change([this](PropertyAnimator::Value value) { y(value); });

        move(Point(0, m_oanim.starting()));
        break;
    }
    }
}

void SideBoard2::position(PositionFlag position)
{
    if (detail::change_if_diff<>(m_position, position))
    {
        m_oanim.stop();
        m_canim.stop();
        resize(calculate_size(position, m_size));
        m_dir = false;
        reset_animations();
        move(Point(m_oanim.starting(), 0));
    }
}

void SideBoard2::reset_animations()
{
	Size appSize = Application::instance().screen()->size();

    switch (m_position)
    {
    case PositionFlag::left:
    {
        m_oanim.starting((m_size.width() == 0) ? -appSize.width() : -m_size.width());
        m_oanim.ending(0);
        m_canim.starting(m_oanim.ending());
        m_canim.ending(m_oanim.starting());
        break;
    }
    case PositionFlag::right:
    {
        m_oanim.starting(appSize.width() - HANDLE_WIDTH);
        m_oanim.ending((m_size.width() == 0) ? -HANDLE_WIDTH : appSize.width() - m_size.width() - HANDLE_WIDTH);
        m_canim.starting(m_oanim.ending());
        m_canim.ending(m_oanim.starting());
        break;
    }
    case PositionFlag::top:
    {
        m_oanim.starting((m_size.height() == 0) ? -appSize.height() : -m_size.height());
        m_oanim.ending(0);
        m_canim.starting(m_oanim.ending());
        m_canim.ending(m_oanim.starting());
        break;
    }
    case PositionFlag::bottom:
    {
        m_oanim.starting(appSize.height() - HANDLE_WIDTH);
        m_oanim.ending((m_size.height() == 0) ? -HANDLE_WIDTH : appSize.height() - m_size.height() - HANDLE_WIDTH);
        m_canim.starting(m_oanim.ending());
        m_canim.ending(m_oanim.starting());
        break;
    }
    }
}

void SideBoard2::handle(Event& event)
{
    Window::handle(event);

    switch (event.id())
    {
    case EventId::pointer_click:
    {
        if (m_dir)
            close();
        else
            open();

        break;
    }
    default:
        break;
    }
}

void SideBoard2::close()
{
    m_canim.stop();
    const auto running = m_oanim.running();
    m_oanim.stop();
    const auto current = m_oanim.current();
    reset_animations();
    if (running)
        m_canim.starting(current);
    m_canim.start();
    m_dir = false;
}

void SideBoard2::open()
{
    m_oanim.stop();
    const auto running = m_canim.running();
    m_canim.stop();
    const auto current = m_canim.current();
    reset_animations();
    if (running)
        m_oanim.starting(current);
    m_oanim.start();
    m_dir = true;
}

template<>
const std::pair<SideBoard2::PositionFlag, char const*> detail::EnumStrings<SideBoard2::PositionFlag>::data[] =
{
    {SideBoard2::PositionFlag::left, "left"},
    {SideBoard2::PositionFlag::right, "right"},
    {SideBoard2::PositionFlag::top, "top"},
    {SideBoard2::PositionFlag::bottom, "bottom"},
};

void SideBoard2::serialize(Serializer& serializer) const
{
    Window::serialize(serializer);

    serializer.add_property("position", detail::enum_to_string(position()));
}

void SideBoard2::deserialize(Serializer::Properties& props)
{
    props.erase(std::remove_if(props.begin(), props.end(), [&](auto & p)
    {
        if (std::get<0>(p) == "position")
        {
            position(detail::enum_from_string<PositionFlag>(std::get<1>(p)));
            return true;
        }
        return false;
    }), props.end());
}

std::ostream& operator<<(std::ostream& os, const SideBoard2::PositionFlag& flag)
{
    return os << detail::enum_to_string(flag);
}

}
}
