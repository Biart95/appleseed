
//
// This source file is part of appleseed.
// Visit https://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2019 Artem Bishev, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "foundation/math/octree.h"
#include "foundation/utility/test.h"

using namespace foundation;

TEST_SUITE(Foundation_Math_Octree)
{
    TEST_CASE(SplitNode)
    {
        SparseOctree<float, 64> octree;

        const uint16_t root = octree.get_root_id();
        ASSERT_EQ(1, octree.get_size());

        octree.split(root);
        ASSERT_EQ(9, octree.get_size());
        for (uint8_t i = 0; i < 8; ++i)
        {
            const uint16_t child = octree.get_child_id(root, i);
            EXPECT_NEQ(octree.Null, child);
            EXPECT_LT(octree.get_size(), child);
            EXPECT_EQ(root, octree.get_parent_id(child));
            EXPECT_EQ(octree.Null, octree.get_child_id(child, i));
        }

        const uint16_t child = octree.get_child_id(root, 7);
        for (uint8_t i = 0; i < 7; ++i)
        {
            EXPECT_EQ(octree.Null, octree.get_child_id(child, i));
        }

        octree.split(child);
        EXPECT_EQ(17, octree.get_size());
        for (uint8_t i = 0; i < 8; ++i)
        {
            const uint16_t grandchild = octree.get_child_id(child, i);
            EXPECT_NEQ(octree.Null, grandchild);
            EXPECT_LT(octree.get_size(), grandchild);
            EXPECT_EQ(child, octree.get_parent_id(grandchild));
            EXPECT_EQ(root, octree.get_parent_id(octree.get_parent_id(grandchild)));
        }
    }

    TEST_CASE(CalculateLocation)
    {
        SparseOctree<float, 1+8+64> octree;
        Vector3f a = Vector3f(0.7f, 0.9f, 0.1f);
        Vector3f b = Vector3f(0.1f, 0.1f, 0.1f);

        const uint16_t root = octree.get_root_id();
        for (Vector3f point : { a, b })
        {
            auto loc = octree.calculate_location(point);
            EXPECT_EQ(root, loc.get_node_id());
            EXPECT_FEQ(point.x, loc.get_local_coords().x);
            EXPECT_FEQ(point.y, loc.get_local_coords().y);
            EXPECT_FEQ(point.z, loc.get_local_coords().z);
        }

        octree.split(root);
        {
            auto loc = octree.calculate_location(a);
            EXPECT_EQ(octree.get_child_id(root, 1|2|0), loc.get_node_id());
            EXPECT_FEQ(0.4f, loc.get_local_coords().x);
            EXPECT_FEQ(0.8f, loc.get_local_coords().y);
            EXPECT_FEQ(0.2f, loc.get_local_coords().z);
        }
        {
            auto loc = octree.calculate_location(b);
            EXPECT_EQ(octree.get_child_id(root, 0|0|0), loc.get_node_id());
            EXPECT_FEQ(0.2f, loc.get_local_coords().x);
            EXPECT_FEQ(0.2f, loc.get_local_coords().y);
            EXPECT_FEQ(0.2f, loc.get_local_coords().z);
        }

        const auto child = octree.get_child_id(root, 1|2|0);
        octree.split(child);
        {
            auto loc = octree.calculate_location(a);
            EXPECT_EQ(octree.get_child_id(child, 0|2|0), loc.get_node_id());
            EXPECT_FEQ(0.8f, loc.get_local_coords().x);
            EXPECT_FEQ(0.6f, loc.get_local_coords().y);
            EXPECT_FEQ(0.4f, loc.get_local_coords().z);
        }
        {
            auto loc = octree.calculate_location(b);
            EXPECT_EQ(octree.get_child_id(root, 0|0|0), loc.get_node_id());
            EXPECT_FEQ(0.2f, loc.get_local_coords().x);
            EXPECT_FEQ(0.2f, loc.get_local_coords().y);
            EXPECT_FEQ(0.2f, loc.get_local_coords().z);
        }

        for (int i = 0; i < 8; ++i)
        {
            octree.split(octree.get_child_id(root, i));
        }
        for (int i = 0; i < 8; ++i)
        {
            Vector3f point(
                static_cast<float>((i >> 0) & 1),
                static_cast<float>((i >> 1) & 1),
                static_cast<float>((i >> 2) & 1));
            auto loc = octree.calculate_location(point);
            EXPECT_EQ(octree.get_child_id(octree.get_child_id(root, i), i), loc.get_node_id());
            EXPECT_FEQ(point.x, loc.get_local_coords().x);
            EXPECT_FEQ(point.y, loc.get_local_coords().y);
            EXPECT_FEQ(point.z, loc.get_local_coords().z);
        }
    }

    TEST_CASE(Trace)
    {
        SparseOctree<float, 64> octree;
        Vector3f origin = Vector3f(1.0f, 0.0f, 0.0f);
        Vector3f direction = normalize(Vector3f(-15.0f, 10.0f, 8.0f));
        Vector3f direction_signs = Vector3f(0.0f, 1.0f, 1.0f);

        //
        // +-------+-+-+---O
        // |       +-+-+   +
        // |       +-+-+---+
        // |       |   |   |
        // +-------+---+---+
        // |       |       |
        // |       |       |
        // |       |       |
        // +-------+---+---+
        //
        const uint16_t root = octree.get_root_id();
        octree.split(root);
        octree.split(octree.get_child_id(root, 1|0|0));
        octree.split(octree.get_child_id(octree.get_child_id(root, 1|0|0), 0|0|0));
        octree.split(octree.get_child_id(root, 0|2|0));

        auto intersection = octree.calculate_location(origin);
        bool left_octree;

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        EXPECT_EQ(octree.get_child_id(octree.get_child_id(octree.get_child_id(root, 1|0|0), 0|0|0), 1|2|4), intersection.get_node_id());

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        {
            const auto coords = octree.calculate_coords(intersection);
            EXPECT_FEQ(5.0f / 8.0f, coords.x); EXPECT_FEQ(2.0f / 8.0f, coords.y); EXPECT_FEQ(1.6f / 8.0f, coords.z);
        }

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        EXPECT_EQ(octree.get_child_id(octree.get_child_id(root, 1|0|0), 0|2|4), intersection.get_node_id());

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        EXPECT_EQ(octree.get_child_id(root, 0|0|0), intersection.get_node_id());

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        {
            const auto coords = octree.calculate_coords(intersection);
            EXPECT_FEQ(0.25f, coords.x); EXPECT_FEQ(0.5f, coords.y); EXPECT_FEQ(0.4f, coords.z);
        }

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        ASSERT_FALSE(left_octree);
        EXPECT_EQ(octree.get_child_id(root, 0|2|4), intersection.get_node_id());

        left_octree = octree.find_next_intersection(direction, direction_signs, intersection, intersection);
        EXPECT_TRUE(left_octree);
        {
            const auto coords = octree.calculate_coords(intersection);
            EXPECT_FEQ(0.0f, coords.x);
            EXPECT_FEQ(-direction.y, coords.y * direction.x);
            EXPECT_FEQ(-direction.z, coords.z * direction.x);
        }
    }
}
