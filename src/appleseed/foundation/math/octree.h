

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

#pragma once

// appleseed.foundation headers.
#include "foundation/math/scalar.h"
#include "foundation/math/vector.h"
#include "foundation/platform/compiler.h"


namespace foundation
{

struct SparseOctreeLocation
{
    const Vector3f& get_local_coords() const;
    uint16_t get_node_id() const;

    SparseOctreeLocation(const Vector3f& local_coords, uint16_t node_id);

    Vector3f    m_local_coords;
    uint16_t    m_node_id;
};


template <typename ValueType, uint16_t MaxSize>
class SparseOctree
{
public:
    SparseOctree();

    static const uint8_t Null = 0xFFu;

    void split(const uint16_t node_id);

    uint16_t get_root_id() const;
    const uint16_t get_child_id(const uint16_t node_id, const uint8_t child) const;
    const uint16_t get_parent_id(const uint16_t node_id) const;

    bool find_next_intersection(
        const Vector3f&                 direction,
        const Vector3f&                 direction_signs,
        const SparseOctreeLocation&     prev_intersection,
        SparseOctreeLocation&           intersection) const;

    const ValueType& operator[](const uint16_t node_id) const;
    ValueType& operator[](const uint16_t node_id);

    SparseOctreeLocation calculate_location(const Vector3f& coords) const;
    Vector3f calculate_coords(const SparseOctreeLocation& location) const;

    uint16_t get_size() const;
    static constexpr uint16_t get_max_size();

private:
    struct Node
    {
        Node()
            : m_value()
            , m_children_id(Null)
            , m_parent_id(Null)
        {
        }

        ValueType   m_value;
        uint16_t    m_children_id;
        uint16_t    m_parent_id;
    };

    void descend_till_leaf(SparseOctreeLocation& location) const;
    void ascend_till_has_neighbor(
        SparseOctreeLocation&   location,
        const uint8_t           face,
        const bool              side) const;

    bool has_neighbor(
        const SparseOctreeLocation& location,
        const uint8_t               face,
        const bool                  side) const;
    uint32_t get_neighbor(
        const SparseOctreeLocation& location,
        const uint8_t               face,
        const bool                  side) const;

    void ascend(SparseOctreeLocation& location) const;
    void descend(SparseOctreeLocation& location) const;

    uint16_t    m_size;
    Node        m_nodes[MaxSize];
};


const Vector3f& SparseOctreeLocation::get_local_coords() const
{
    return m_local_coords;
}

uint16_t SparseOctreeLocation::get_node_id() const
{
    return m_node_id;
}

SparseOctreeLocation::SparseOctreeLocation(const Vector3f& local_coords, uint16_t node_id)
  : m_local_coords(local_coords)
  , m_node_id(node_id)
{
}


template <typename ValueType, uint16_t MaxSize>
SparseOctree<ValueType, MaxSize>::SparseOctree()
  : m_size(1u)
{
}

template <typename ValueType, uint16_t MaxSize>
void SparseOctree<ValueType, MaxSize>::split(const uint16_t node_id)
{
    if (m_nodes[node_id].m_children_id != Null)
        return;
    assert(m_size + 8 <= MaxSize);
    m_nodes[node_id].m_children_id = m_size;
    for (uint8_t i = 0; i < 8; ++i)
        m_nodes[m_size + i].m_parent_id = node_id;
    m_size += 8;
}

template <typename ValueType, uint16_t MaxSize>
uint16_t SparseOctree<ValueType, MaxSize>::get_root_id() const
{
    return 0;
}

template <typename ValueType, uint16_t MaxSize>
const uint16_t SparseOctree<ValueType, MaxSize>::get_child_id(const uint16_t node_id, const uint8_t child) const
{
    assert(node_id < m_size);
    assert(child < 8);
    return (m_nodes[node_id].m_children_id == Null) ? Null : (m_nodes[node_id].m_children_id + child);
}

template <typename ValueType, uint16_t MaxSize>
const uint16_t SparseOctree<ValueType, MaxSize>::get_parent_id(const uint16_t node_id) const
{
    assert(node_id < m_size);
    return m_nodes[node_id].m_parent_id;
}

template <typename ValueType, uint16_t MaxSize>
bool SparseOctree<ValueType, MaxSize>::find_next_intersection(
    const Vector3f&             direction,
    const Vector3f&             direction_signs,
    const SparseOctreeLocation& prev_intersection,
    SparseOctreeLocation&       intersection) const
{
    while (true)
    {
        const Vector3f t = (direction_signs - prev_intersection.m_local_coords) / direction;
        const uint8_t face = static_cast<uint8_t>(min_index(t));
        const bool side = (direction_signs[face] > 0.5f);

        intersection = SparseOctreeLocation(
            prev_intersection.m_local_coords + t[face] * direction,
            prev_intersection.m_node_id);
        ascend_till_has_neighbor(intersection, face, side);
        if (intersection.m_node_id == 0)
            return true;
        intersection.m_node_id = get_neighbor(intersection, face, side);
        intersection.m_local_coords[face] = 1.0f - direction_signs[face];
        descend_till_leaf(intersection);
        if (t[face] >= 1e-06f)
            return false;
    }
    APPLESEED_UNREACHABLE;
}

template <typename ValueType, uint16_t MaxSize>
const ValueType& SparseOctree<ValueType, MaxSize>::operator[](const uint16_t node_id) const
{
    return m_nodes[node_id].m_value;
}

template <typename ValueType, uint16_t MaxSize>
ValueType& SparseOctree<ValueType, MaxSize>::operator[](const uint16_t node_id)
{
    return m_nodes[node_id].m_value;
}

template <typename ValueType, uint16_t MaxSize>
SparseOctreeLocation SparseOctree<ValueType, MaxSize>::calculate_location(
    const Vector3f&     coords) const
{
    SparseOctreeLocation result(coords, 0);
    descend_till_leaf(result);
    return result;
}

template <typename ValueType, uint16_t MaxSize>
Vector3f SparseOctree<ValueType, MaxSize>::calculate_coords(
    const SparseOctreeLocation&     location) const
{
    SparseOctreeLocation result = location;
    while (result.get_node_id() != 0) ascend(result);
    return result.get_local_coords();
}

template <typename ValueType, uint16_t MaxSize>
uint16_t SparseOctree<ValueType, MaxSize>::get_size() const
{
    return m_size;
}

template <typename ValueType, uint16_t MaxSize>
static constexpr uint16_t SparseOctree<ValueType, MaxSize>::get_max_size()
{
    return MaxSize;
}

template <typename ValueType, uint16_t MaxSize>
void SparseOctree<ValueType, MaxSize>::descend_till_leaf(
    SparseOctreeLocation&   location) const
{
    while (m_nodes[location.m_node_id].m_children_id != Null) descend(location);
}

template <typename ValueType, uint16_t MaxSize>
void SparseOctree<ValueType, MaxSize>::ascend_till_has_neighbor(
    SparseOctreeLocation&   location,
    const uint8_t           face,
    const bool              side) const
{
    while (location.m_node_id != 0 && !has_neighbor(location, face, side)) ascend(location);
}

template <typename ValueType, uint16_t MaxSize>
bool SparseOctree<ValueType, MaxSize>::has_neighbor(
    const SparseOctreeLocation& location,
    const uint8_t               face,
    const bool                  side) const
{
    if (location.m_node_id == 0) return false;
    const uint32_t parent_id = m_nodes[location.m_node_id].m_parent_id;
    assert(parent_id < MaxSize);
    const uint32_t child = location.m_node_id - m_nodes[parent_id].m_children_id;
    const bool child_side = (child & (1 << face)) != 0;
    return child_side != side;
}

template <typename ValueType, uint16_t MaxSize>
uint32_t SparseOctree<ValueType, MaxSize>::get_neighbor(
    const SparseOctreeLocation& location,
    const uint8_t               face,
    const bool                  side) const
{
    assert(has_neighbor(location, face, side));
    const uint32_t parent_id = m_nodes[location.m_node_id].m_parent_id;
    assert(parent_id < MaxSize);
    uint32_t child = location.m_node_id - m_nodes[parent_id].m_children_id;
    assert(child < 8);
    child ^= (1 << face);
    assert(child < 8);

    return m_nodes[parent_id].m_children_id + child;
}

template <typename ValueType, uint16_t MaxSize>
void SparseOctree<ValueType, MaxSize>::ascend(SparseOctreeLocation& location) const
{
    assert(is_saturated(location.m_local_coords));
    assert(location.m_node_id < MaxSize);

    const uint32_t parent_id = m_nodes[location.m_node_id].m_parent_id;
    assert(parent_id < MaxSize);

    const uint32_t child = location.m_node_id - m_nodes[parent_id].m_children_id;
    assert(child < 8);

    for (uint8_t i = 0; i < 3; ++i)
    {
        if ((child & (1 << i)) != 0)
        {
            location.m_local_coords[i] += 1.0f;
        }
    }
    location.m_local_coords *= 0.5f;
    location.m_node_id = parent_id;
}

template <typename ValueType, uint16_t MaxSize>
void SparseOctree<ValueType, MaxSize>::descend(SparseOctreeLocation& location) const
{
    assert(location.m_node_id < MaxSize && m_nodes[location.m_node_id].m_children_id < MaxSize);
    assert(is_saturated(location.m_local_coords));

    uint16_t child = 0;
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (location.m_local_coords[i] > 0.5f)
        {
            child |= (1 << i);
            location.m_local_coords[i] -= 0.5f;
        }
    }
    location.m_local_coords *= 2.0f;
    location.m_node_id = m_nodes[location.m_node_id].m_children_id + child;
    assert(child < 8 && location.m_node_id < MaxSize);
}

};
