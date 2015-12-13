// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <luabind/shared_ptr_converter.hpp>

struct X
{
    X(int value)
      : value(value)
    {}

    int value;
};

int get_value(boost::shared_ptr<X> const& p)
{
    return p->value;
}

boost::shared_ptr<X> filter(boost::shared_ptr<X> const& p)
{
    return p;
}


class ExistenceTester {
  public:
    typedef boost::shared_ptr<ExistenceTester> Ptr;
  
    const char* name;
    bool canDestroy;
    
    ExistenceTester(const char* name) : name(name), canDestroy(false) {}
    virtual ~ExistenceTester() {
      if (!canDestroy) {
        report_failure((std::string("ExistenceTester '") + name + "' destroyed too early").c_str(), __FILE__, __LINE__);
      }
    }
};

#include <typeinfo>
void test_object_exists_after_state_destruction() {
  ExistenceTester::Ptr etVar(new ExistenceTester("assigned variable"));
  ExistenceTester::Ptr createdVar;

  lua_State* lua = luaL_newstate();
  luabind::open(lua);
  luabind::module(lua) [
    luabind::class_< ExistenceTester, ExistenceTester::Ptr >("ExistenceTester")
      .def(luabind::constructor<const char*>())
  ];
  
  luabind::globals(lua)["var"] = etVar;
  
  DOSTRING(lua, "created = ExistenceTester('created variable')\n");
      
  luabind::globals(lua)["created"].push(lua);
               
  typename luabind::default_converter<ExistenceTester::Ptr> converter;
  for (int i = 1; i <= lua_gettop(lua); i++) {
    std::cout << "Stack " << i << ":" <<  lua_typename(lua, lua_type(lua, i)) << std::endl;
    lua_pushvalue(lua, i);
    std::cout << "   " << lua_tostring(lua, -1) << std::endl;
    lua_pop(lua, 1);
  }
  std::cout << "match result = " << converter.match(lua, ExistenceTester::Ptr(), -1) << std::endl;
  std::cout << typeid(converter).name() << std::endl;
  createdVar = converter.apply(lua, ExistenceTester::Ptr(), -1);
  
  std::cout << createdVar->name << std::endl;
  
  lua_close(lua);
  
  std::cout << "Segfault imminent" << std::endl;
  
  assert(createdVar);
  
  createdVar->canDestroy = true;
  createdVar.reset();
  
  etVar->canDestroy = true;
  etVar.reset();
}

void test_main(lua_State* L)
{
    using namespace luabind;

    module(L) [
        class_<X>("X")
            .def(constructor<int>()),
        def("get_value", &get_value),
        def("filter", &filter)
    ];

    DOSTRING(L,
        "x = X(1)\n"
        "assert(get_value(x) == 1)\n"
    );

    DOSTRING(L,
        "assert(x == filter(x))\n"
    );
    
    test_object_exists_after_state_destruction();
}
