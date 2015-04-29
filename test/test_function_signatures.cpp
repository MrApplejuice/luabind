// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#include "test.hpp"
#include <luabind/luabind.hpp>

// I do not test "pure" references (int&) here, because they
// are not supported in lua

void ia(int x) {}
void ib(const int x) {}
void ic(const int& x) {}

void fa(float x) {}
void fb(const float x) {}
void fc(const float& x) {}

void sa(std::string x) {}
void sb(const std::string x) {}
void sc(const std::string& x) {}

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L)
    [
        def("ia", &ia),
        def("ib", &ib),
        def("ic", &ic),

        def("fa", &fa),
        def("fb", &fb),
        def("fc", &fc),

        def("sa", &sa),
        def("sb", &sb),
        def("sc", &sc)
    ];

    DOSTRING(L, "ia(1)");
    DOSTRING(L, "ib(1)");
    DOSTRING(L, "ic(1)");


    DOSTRING(L, "fa(1)");
    DOSTRING(L, "fa(1.0)");

    DOSTRING(L, "fb(1)");
    DOSTRING(L, "fb(1.0)");

    DOSTRING(L, "fc(1)");
    DOSTRING(L, "fc(1.0)");

    DOSTRING(L, "sa('1')");
    DOSTRING(L, "sb('1')");
    DOSTRING(L, "sc('1')");
}

