#!/usr/bin/env python3
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Independent synthetic coverage for the M98w row-union oracle."""

from __future__ import annotations

import unittest

import zundamon_dirty_union as union


class M98wUnionTests(unittest.TestCase):
    def test_empty_row(self) -> None:
        result = union.row_union([], 0)
        self.assertEqual(result["candidates"], [])
        self.assertEqual(result["merged"], [])

    def test_one_rectangle_matches_word_rounding(self) -> None:
        result = union.row_union([(3, 10, 5, 2)], 10)
        self.assertEqual([(item.x0, item.x1, item.instance_id)
                          for item in result["merged"]], [(2, 8, 0)])
        self.assertEqual(union.row_commands([(3, 10, 5, 2)], 10, 0x220000),
                         [(0x220000 + 10 * 320 + 2, 3)])

    def test_all_parities_and_boundaries(self) -> None:
        expected = ((0, 1, (0, 2)), (0, 2, (0, 2)),
                    (1, 1, (0, 2)), (1, 2, (0, 4)))
        for x, width, interval in expected:
            with self.subTest(x=x, width=width):
                self.assertEqual(union.rounded_interval(x, width), interval)
        self.assertEqual(union.rounded_interval(319, 1), (318, 320))
        self.assertEqual(union.rounded_interval(0, 320), (0, 320))

    def test_overlap_adjacency_and_transitive_merge(self) -> None:
        rectangles = [(0, 0, 5, 1), (4, 0, 5, 1), (8, 0, 4, 1)]
        result = union.row_union(rectangles, 0)
        self.assertEqual([(item.x0, item.x1) for item in result["merged"]],
                         [(0, 12)])
        self.assertEqual(result["overlap_merges"], 2)
        self.assertEqual(result["containment_merges"], 0)

    def test_rounding_can_make_adjacent_or_overlapping(self) -> None:
        adjacent = union.row_union([(1, 0, 1, 1), (2, 0, 1, 1)], 0)
        self.assertEqual([(item.x0, item.x1) for item in adjacent["merged"]],
                         [(0, 4)])
        self.assertEqual(adjacent["adjacency_merges"], 1)
        separate = union.row_union([(0, 0, 1, 1), (4, 0, 1, 1)], 0)
        self.assertEqual(len(separate["merged"]), 2)

    def test_containment_duplicate_and_equal_end(self) -> None:
        result = union.row_union([(2, 0, 10, 1), (4, 0, 6, 1),
                                  (6, 0, 2, 1)], 0)
        self.assertEqual([(item.x0, item.x1) for item in result["merged"]],
                         [(2, 12)])
        self.assertEqual(result["overlap_merges"], 0)
        self.assertEqual(result["containment_merges"], 2)

    def test_sort_key_is_explicit_and_reverse_input_is_safe(self) -> None:
        result = union.row_union([(20, 0, 2, 1), (0, 0, 2, 1),
                                  (10, 0, 2, 1)], 0)
        self.assertEqual([(item.x0, item.instance_id)
                          for item in result["sorted"]],
                         [(0, 1), (10, 2), (20, 0)])

    def test_all_sixteen_candidates_are_bounded(self) -> None:
        rectangles = [(index * 2, 0, 1, 1) for index in range(16)]
        result = union.row_union(rectangles, 0)
        self.assertEqual(len(result["candidates"]), 16)
        self.assertEqual(len(result["merged"]), 1)

    def test_invalid_geometry_has_stable_codes(self) -> None:
        cases = (((0, 0, 0, 1), "M98W_RECT_BOUNDS"),
                 ((-1, 0, 1, 1), "M98W_RECT_BOUNDS"),
                 ((320, 0, 1, 1), "M98W_RECT_BOUNDS"),
                 ((0, 200, 1, 1), "M98W_RECT_BOUNDS"),
                 ((319, 0, 2, 1), "M98W_RECT_BOUNDS"))
        for rectangle, code in cases:
            with self.subTest(rectangle=rectangle):
                with self.assertRaises(union.UnionError) as caught:
                    union.row_union([rectangle], 0)
                self.assertEqual(caught.exception.code, code)

    def test_capacity_and_row_boundaries(self) -> None:
        rectangles = [(index, 0, 1, 1) for index in range(17)]
        with self.assertRaises(union.UnionError) as caught:
            union.row_union(rectangles, 0)
        self.assertEqual(caught.exception.code, "M98W_INTERVAL_CAPACITY")
        self.assertEqual(union.row_union([(0, 199, 1, 1)], 198)["merged"], [])
        self.assertEqual(len(union.row_union([(0, 199, 1, 1)], 199)["merged"]), 1)

    def test_page_address_and_even_cls_count(self) -> None:
        commands = union.row_commands([(319, 199, 1, 1)], 199, 0x22FA00)
        address, words = commands[0]
        self.assertEqual(address, 0x22FA00 + 199 * 320 + 318)
        self.assertEqual(words, 1)
        self.assertEqual(words * 2 % 2, 0)

    def test_page_and_row_overflow_are_rejected(self) -> None:
        with self.assertRaises(union.UnionError) as caught:
            union.row_commands([], 200, 0x220000)
        self.assertEqual(caught.exception.code, "M98W_ROW_BOUNDS")
        with self.assertRaises(union.UnionError) as caught:
            union.row_commands([(0, 0, 1, 1)], 0, 0xffffffff)
        self.assertEqual(caught.exception.code, "M98W_PAGE_BOUNDS")

    def test_all_rows_are_row_major(self) -> None:
        commands = union.all_rows([(2, 1, 4, 2), (20, 3, 2, 1)], 0x220000)
        self.assertEqual(commands, [(0x220000 + 1 * 320 + 2, 2),
                                    (0x220000 + 2 * 320 + 2, 2),
                                    (0x220000 + 3 * 320 + 20, 1)])


if __name__ == "__main__":
    unittest.main()
