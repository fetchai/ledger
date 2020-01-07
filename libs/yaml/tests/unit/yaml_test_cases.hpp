#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include <string>

struct TestCase
{
  char const *input_text;
  char const *output_text;
  bool        expect_output;
  bool        expect_throw;
};

static const TestCase TEST_CASES[] = {
    // ====================================================================
    // Easy parsing
    // ====================================================================
    // basic one-line sequences
    {R"([one, two,three])", R"(["one", "two", "three"])", true, false},
    {R"([true, false])", "[true, false]", true, false},
    {R"([1, 2, 3])", "[1, 2, 3]", true, false},
    {R"(one: two)", R"({"one": "two"})", true, false},
    // numeric data types
    // ====================================================================
    // Problems
    // ====================================================================
    // sequence in mapping
    {R"(sequence: [one, two])", R"({"sequence": ["one", "two"]})", true, false},
    {R"(sequence:
- one
- two)",
     R"({"sequence": ["one", "two"]})", true, false},
    // multiline compact sequence
    {R"(- key: value
- key: another value)",
     R"([{"key": "value"}, {"key": "another value"}])", true,
     false},                                     // Check for compact mappings in multiline sequence
    {R"(one, two)", "", false, true},            // invalid single-line sequence
    {R"(sequence: one, two)", "", false, true},  // invalid single-line sequence as a value
    // ====================================================================
    // Spec examples
    // ====================================================================
    {R"(- Mark McGwire
- Sammy Sosa
- Ken Griffey)",
     R"(["Mark McGwire", "Sammy Sosa", "Ken Griffey"])", true, false},  // Example 2.1
    {R"(- [name        , hr, avg  ]
- [Mark McGwire, 65, 0.278]
- [Sammy Sosa  , 63, 0.288])",
     R"([["name", "hr", "avg"], ["Mark McGwire", 65, 0.278], ["Sammy Sosa", 63, 0.288]])", true,
     false},  // Example 2.5
    {R"(# ASCII Art
--- |
  \//||\/||
  // ||  ||__)",
     "\"\\\\//||\\\\/||\n// ||  ||__\"", true,
     false},  // Example 2.13.  In literals, newlines are preserved
    {R"(--- >
  Mark McGwire's
  year was crippled
  by a knee injury.)",
     "\"Mark McGwire's year was crippled by a knee injury.\"", true,
     false},  // Example 2.14.  In the folded scalars, newlines become spaces
    {R"(# Ordered maps are represented as
# A sequence of mappings, with
# each mapping having one key
--- !!omap
- Mark McGwire: 65
- Sammy Sosa: 63
- Ken Griffy: 58)",
     R"([{"Mark McGwire": 65}, {"Sammy Sosa": 63}, {"Ken Griffy": 58}])", true,
     false},                                       // Example 2.26. Ordered Mappings
    {R"(commercial-at: @text)", "", false, true},  // 5.10. Invalid use of reserved indicators
    {R"(grave-accent: `text)", "", false, true},   // 5.10. Invalid use of reserved indicators
};
