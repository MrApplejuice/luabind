// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_SHARED_PTR_CONVERTER_090211_HPP
# define LUABIND_SHARED_PTR_CONVERTER_090211_HPP

# include <luabind/detail/policy.hpp>    // for default_converter, etc
# include <luabind/get_main_thread.hpp>  // for get_main_thread
# include <luabind/object.hpp>           // for object
# include <luabind/scope.hpp>            // for module
# include <luabind/class.hpp>            // for class_
# include <luabind/detail/decorate_type.hpp>  // for LUABIND_DECORATE_TYPE

# include <boost/mpl/bool.hpp>           // for bool_, false_
# include <boost/smart_ptr/shared_ptr.hpp>  // for shared_ptr, get_deleter

namespace luabind {

namespace detail
{

  struct shared_ptr_manager {
    public:
      struct shared_ptr_container {
        virtual ~shared_ptr_container() {}
      };
      
      template <typename T>
      struct boost_shared_ptr_container : shared_ptr_container {
        boost::shared_ptr<T> ptr;

        boost_shared_ptr_container(const boost::shared_ptr<T>& ptr) : shared_ptr_container(), ptr(ptr) {}
        virtual ~boost_shared_ptr_container() {}
      };
    private:
      lua_State* lua;
      luabind::object map;
      
      void _register() {
        map = luabind::registry(lua)["__boost_shared_ptr_management_table"];
        if (luabind::type(map) == LUA_TNIL) {
          // Push a new weak metatable an register required bindings
          map = luabind::newtable(lua);
          luabind::object metatable = luabind::newtable(lua);
          metatable["__mode"] = "k";
          luabind::setmetatable(map, metatable);
          luabind::registry(lua)["__boost_shared_ptr_management_table"] = map;
          
          luabind::module(lua) [
            luabind::class_<shared_ptr_container>("__boost_shared_ptr_container")
          ];
        }
      }
    public:
      shared_ptr_manager(lua_State* lua) : lua(lua) {
        _register();
      }
      
      template <typename T>
      boost::shared_ptr<T> get_shared_ptr(int index) {
        typedef boost::shared_ptr<T> ConversionType;
        
        // Try a direct-extraction of the boost::shared_ptr (in case that is the internally used holder pointer, already)
        luabind::detail::value_converter directPointerConverter;
        int directPtrMatchResult = directPointerConverter.match(lua, luabind::detail::by_value< ConversionType >(), -1);
        if (directPtrMatchResult == 0) {
          return directPointerConverter.apply(lua, luabind::detail::by_value< ConversionType >(), -1);
        }
        
        luabind::object object(luabind::from_stack(lua, index));
        luabind::object ptrObject = map[object];
        if (luabind::type(ptrObject) != LUA_TNIL) {
          shared_ptr_container* ptrContainer = luabind::object_cast<shared_ptr_container*>(ptrObject);
          if (ptrContainer) {
            return dynamic_cast<boost_shared_ptr_container<T>*>(ptrContainer)->ptr;
          }
        }
        return boost::shared_ptr<T>();
      }
      
      template <typename T>
      void push_shared_ptr(const boost::shared_ptr<T>& ptr) {
        default_converter<T*>().apply(lua, ptr.get());
        luabind::object object(luabind::from_stack(lua, -1));
        map[object] = dynamic_cast<shared_ptr_container*>(new boost_shared_ptr_container<T>(ptr));
      }
  };

} // namespace detail

template <class T>
struct default_converter<boost::shared_ptr<T> >
  : default_converter<T*>
{
    typedef boost::mpl::false_ is_native;

    template <class U>
    int match(lua_State* L, U, int index)
    {
        return default_converter<T*>::match(
            L, LUABIND_DECORATE_TYPE(T*), index);
    }

    template <class U>
    boost::shared_ptr<T> apply(lua_State* L, U, int index)
    {
        return detail::shared_ptr_manager(L).get_shared_ptr<T>(index);
    }
  
    void apply(lua_State* L, boost::shared_ptr<T> const& p)
    {
        detail::shared_ptr_manager(L).push_shared_ptr<T>(p);
    }

    template <class U>
    void converter_postcall(lua_State*, U const&, int)
    {}
};

template <class T>
struct default_converter<boost::shared_ptr<T> const&>
  : default_converter<boost::shared_ptr<T> >
{};

} // namespace luabind

#endif // LUABIND_SHARED_PTR_CONVERTER_090211_HPP
