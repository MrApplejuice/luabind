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
//#include <luabind/shared_ptr_converter.hpp>


#include <boost/shared_ptr.hpp>

struct A
{
  typedef boost::shared_ptr<A> Ptr;

  int x;
  
  A(int x) : x(x) {}
};

struct B : public A
{
  typedef boost::shared_ptr<B> Ptr;

  void BFunction() {}
  B(int x) : A(x) {}
};

struct User {
  typedef A::Ptr Type;

  Type getSomeA() const { return A::Ptr(new A(0)); }
  void setA(Type t) { std::cout << __FUNCTION__ << " " << t->x << std::endl; }
  void setConstRefA(const Type& t) { std::cout << __FUNCTION__ << " " << t->x << std::endl; }
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
            .def(constructor<int>())
    ];
       
    module(L)
    [
        class_<B, A, B::Ptr>("B")
            .def(constructor<int>())
            .def("BFunc", &B::BFunction),
        class_<User>("User")
            .def("setA", &User::setA)
            .def("setConstRefA", &User::setConstRefA)
            .property("a_prop_constRef", &User::getSomeA, &User::setConstRefA)
            .property("a_prop", &User::getSomeA, &User::setA),
        def("accessX", &accessX),
        def("accessXConstRef", &accessXConstRef)
    ];
    
    globals(L)["a_"] = object(L, A::Ptr(new A(11337)));
    globals(L)["b_"] = object(L, B::Ptr(new B(21337)));

    DOSTRING(L, "a = A(1337)");
    DOSTRING(L, "assert(accessX(a) == 1337)");
    DOSTRING(L, "assert(accessXConstRef(a) == 1337)");

    DOSTRING(L, "b = B(2337)");
    DOSTRING(L, "assert(accessX(b) == 2337)");
    DOSTRING(L, "assert(accessXConstRef(b) == 2337)");

    std::cout << "A id: " << luabind::detail::registered_class<A>::id << std::endl;
    std::cout << "B id: " << luabind::detail::registered_class<B>::id << std::endl;

    std::cout << "A::Ptr id: " << luabind::detail::registered_class<A::Ptr>::id << std::endl;
    std::cout << "B::Ptr id: " << luabind::detail::registered_class<B::Ptr>::id << std::endl;


    globals(L)["user"] = object(L, User());
    DOSTRING(L, "user:setA(a)");
    DOSTRING(L, "user:setA(b)");
    DOSTRING(L, "user:setA(a_)");
    DOSTRING(L, "user:setA(b_)");
    DOSTRING(L, "user:setConstRefA(a)");
    DOSTRING(L, "user:setConstRefA(b)");
    DOSTRING(L, "user:setConstRefA(a_)");
    DOSTRING(L, "user:setConstRefA(b_)");
    DOSTRING(L, "user.a_prop = a");
    DOSTRING(L, "user.a_prop = b");
    DOSTRING(L, "user.a_prop = a_");
    DOSTRING(L, "user.a_prop = b_");
    DOSTRING(L, "user.a_prop_constRef = a");
    DOSTRING(L, "user.a_prop_constRef = b");
    DOSTRING(L, "user.a_prop_constRef = a_");
    DOSTRING(L, "user.a_prop_constRef = b_");

    DOSTRING(L, "b:BFunc()");
    DOSTRING(L, "b_:BFunc()");
}

