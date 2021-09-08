/*
 * Copyright (C) 2021 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EGT_SIDEBOARD2_H
#define EGT_SIDEBOARD2_H

/**
 * @file
 * @brief Sideboard widget.
 */

#include <chrono>
#include <egt/animation.h>
#include <egt/detail/meta.h>
#include <egt/easing.h>
#include <egt/window.h>
#include <iosfwd>

namespace egt
{
inline namespace v1
{

/**
 * SideBoard Window for a sliding board that slides on and off the screen.
 *
 * This is a widget that manages a Window that slides on and off the screen,
 * with only a small portion of it, the "handle", shown so that sliding it out
 * can be initiated by default.
 */
class EGT_API SideBoard2 : public Window
{
public:

    /// Default width shown when the SideBoard is closed.
    constexpr static const int HANDLE_WIDTH = 50;

    /// Position flag of the SideBoard.
    enum class PositionFlag : uint32_t
    {
        /// Left side.
        left,
        /// Right side.
        right,
        /// Top side.
        top,
        /// Bottom side.
        bottom,
    };

    /**
     * @param[in] position Position of the SideBoard.
     * @param[in] size Size of the sideboard (0,0 = use full size of screen)
     * @param[in] open_duration The duration of the open animation.
     * @param[in] open_func Open easing function.
     * @param[in] close_duration Duration of the close animation.
     * @param[in] close_func Close easing function.
     * @param[in] hint Requested Window type. This only applies if this Window
     *            will be responsible for creating a backing screen.  This is
     *            only a hint.
     */
    explicit SideBoard2(PositionFlag position = PositionFlag::left,
    				   Size size = Size(0, 0),
                       std::chrono::milliseconds open_duration = std::chrono::milliseconds(1000),
                       EasingFunc open_func = easing_cubic_easeinout,
                       std::chrono::milliseconds close_duration = std::chrono::milliseconds(1000),
                       EasingFunc close_func = easing_circular_easeinout,
                       WindowHint hint = WindowHint::automatic);

    /**
     * @param[in] position Position of the SideBoard.
     * @param[in] hint Requested Window type. This only applies if this Window
     *            will be responsible for creating a backing screen.  This is
     *            only a hint.
     */
    SideBoard2(PositionFlag position, Size size, WindowHint hint);

    /**
     * @param[in] props list of widget argument and its properties.
     */
    explicit SideBoard2(Serializer::Properties& props) noexcept;

    void handle(Event& event) override;

    /**
     * Set the position of the SideBoard.
     *
     * @param[in] position Position of the SideBoard.
     */
    void position(PositionFlag position);

    /// Get the position of the SideBoard.
    EGT_NODISCARD PositionFlag position() const { return m_position; }

    /// Move to a closed state.
    void close();

    /// Move to an open state.
    void open();

    // report opened state
    bool is_open() { return m_dir; }

    void serialize(Serializer& serializer) const override;

protected:

    /// Reset animation start/end values.
    void reset_animations();

    /// SideBoard flags.
    PositionFlag m_position{PositionFlag::left};

    // size of the board, 0 on an extent implies use the screen dimension for that extent
    Size m_size;

    /// Open animation.
    PropertyAnimator m_oanim;

    /// Close animation.
    PropertyAnimator m_canim;

    /// State of the current direction.
    bool m_dir{false};

private:
    void initialize();

    void deserialize(Serializer::Properties& props) override;
};

/// Overloaded std::ostream insertion operator
EGT_API std::ostream& operator<<(std::ostream& os, const SideBoard2::PositionFlag& flag);

}
}

#endif
