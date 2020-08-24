/*
 *     Copyright (c) 2020 NetEase Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * Project: curve
 * Date: Wed Sep  9 11:28:54 CST 2020
 * Author: wuhanqing
 */

#include <gtest/gtest.h>
#include <memory>
#include "nbd/src/ImageInstance.h"

namespace curve {
namespace nbd {

TEST(ImageInstanceTest, FileNameTest) {
    ImagePtr image = std::make_shared<NebdImageInstance>("/1");
    ASSERT_EQ(image->GetFileName(), "cbd:pool//1");

    image = std::make_shared<NebdImageInstance>("cbd:pool//1");
    ASSERT_EQ(image->GetFileName(), "cbd:pool//1");
}

}  // namespace nbd
}  // namespace curve
