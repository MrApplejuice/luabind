// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include <luabind/luabind.hpp>
#include <boost/shared_ptr.hpp>
//#include <luabind/shared_ptr_converter.hpp>

class ExistenceTester {
  public:
    static int number_of_initialized_objects;
  
    typedef boost::shared_ptr<ExistenceTester> Ptr;
  
    const char* name;
    bool canDestroy;
    
    ExistenceTester(const char* name) : name(name), canDestroy(false) {
      number_of_initialized_objects++;
    }
    
    virtual ~ExistenceTester() {
      if (!canDestroy) {
        report_failure((std::string("ExistenceTester '") + name + "' destroyed too early").c_str(), __FILE__, __LINE__);
      }
      number_of_initialized_objects--;
    }
};

int ExistenceTester::number_of_initialized_objects = 0;

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
               
  //typename luabind::default_converter<ExistenceTester::Ptr> converter;
  //createdVar = converter.apply(lua, ExistenceTester::Ptr(), -1);
  createdVar = luabind::object_cast<ExistenceTester::Ptr>(luabind::object(luabind::from_stack(lua, -1)));
  
  lua_close(lua);
  
  TEST_CHECK(createdVar);
  
  createdVar->canDestroy = true;
  createdVar.reset();
  
  etVar->canDestroy = true;
  etVar.reset();
  
  TEST_CHECK(ExistenceTester::number_of_initialized_objects == 0);
}


std::vector<void*> created_pointers;
bool in_created_pointers(void* p) {
  for (std::vector<void*>::iterator it = created_pointers.begin(); it != created_pointers.end(); it++) {
    if (p == *it) {
      return true;
    }
  }
  return false;
}

struct X : public luabind::wrap_base
{
    X(int value)
      : value(value) {
      created_pointers.push_back(this);
    }

    int value;
};

int get_value(boost::shared_ptr<X> p)
{
    TEST_CHECK(in_created_pointers(p.get()));
    return p->value;
}

boost::shared_ptr<X> filter(boost::shared_ptr<X> p)
{
    TEST_CHECK(in_created_pointers(p.get()));
    return p;
}

void test_main(lua_State* L)
{
    using namespace luabind;

    test_object_exists_after_state_destruction();
    
    module(L) [
        class_<X, boost::shared_ptr<X> >("X")
            .def(constructor<int>()),
        def("get_value", &get_value),
        def("filter", &filter)
    ];

    DOSTRING(L,
        "x = X(1)\n"
        "assert(get_value(x) == 1)\n"
    );

    DOSTRING(L,
        "assert(x == x)\n"
        "y = x\n"
        "assert(y == x)\n"
    );
    
    DOSTRING(L,
        "assert(x == filter(x))\n"
    );
}
