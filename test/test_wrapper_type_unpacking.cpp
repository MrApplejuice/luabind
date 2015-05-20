// Copyright (c) 2015 Paul Konstantin Gerke

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

#include <boost/shared_ptr.hpp>

struct A
{
  typedef boost::shared_ptr<A> Ptr;

  int x;
  
  A(int x) : x(x) {}
};

struct B : public A
{
  B(int x) : A(x) {}
};

struct User {
  A::Ptr getSomeA() const { return A::Ptr(new A(0)); }
  void setA(A::Ptr) {}
  void setConstRefA(const A::Ptr&) {}
};

int accessX(A::Ptr a) {
  return a->x;
}

int accessXConstRef(const A::Ptr& a) {
  return a->x;
}

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L)
    [
        class_<A, A::Ptr>("A")
            .def(constructor<int>()),
        class_<B, A, A::Ptr>("B")
            .def(constructor<int>()),
        class_<User>("User")
            .def(constructor<>())
            .def("setA", &User::setA)
            .def("setConstRefA", &User::setConstRefA)
            .def("a_prop_constRef", &User::getSomeA, &User::setConstRefA)
            .def("a_prop", &User::getSomeA, &User::setA),
        def("accessX", &accessX),
        def("accessXConstRef", &accessXConstRef)
    ];

    DOSTRING(L, "a = A(1337)");
    DOSTRING(L, "assert(accessX(a) == 1337)");
    DOSTRING(L, "assert(accessXConstRef(a) == 1337)");

    DOSTRING(L, "b = B(1500)");
    DOSTRING(L, "assert(accessX(b) == 1500)");
    DOSTRING(L, "assert(accessXConstRef(b) == 1500)");

    DOSTRING(L, "user = User()");
    DOSTRING(L, "user:setA(a)");
    DOSTRING(L, "user:setA(b)");
    DOSTRING(L, "user:setConstRefA(a)");
    DOSTRING(L, "user:setConstRefA(b)");
    DOSTRING(L, "user.a_prop = a");
    DOSTRING(L, "user.a_prop = b");
    DOSTRING(L, "user.a_prop_constRef = a");
    DOSTRING(L, "user.a_prop_constRef = b");
}

