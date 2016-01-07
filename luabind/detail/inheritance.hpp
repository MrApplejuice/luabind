// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_INHERITANCE_090217_HPP
# define LUABIND_INHERITANCE_090217_HPP

# include <cstdio>
# include <cassert>
# include <limits>
# include <map>
# include <memory>
# include <vector>
# include <luabind/typeid.hpp>

# include <boost/shared_ptr.hpp>
# include <boost/scoped_ptr.hpp>
# include <boost/type_traits.hpp>
# include <boost/utility.hpp>

namespace luabind { namespace detail {

class CastRefContainer
{
public:
    class PointerManagerCallerBase {
        public:
            virtual PointerManagerCallerBase* clone() const = 0;
            virtual void* get() = 0;
            virtual ~PointerManagerCallerBase() {}
    };

    template <typename T>
    class SpecificPointerManager : public PointerManagerCallerBase {
        public:
            SpecificPointerManager(T* ptr) : ptr(ptr) {
                assert(ptr);
                printf("!! %lld created pointer %lld -- %s\n", (long long) this, (long long) ptr, typeid(T).name());
            }

            virtual PointerManagerCallerBase* clone() const {
                T* nptr = new T;
                *nptr = *ptr;
                return new SpecificPointerManager<T>(nptr);
            }

            virtual void* get() { 
                printf("!! %lld retrieving pointer %lld -- %s\n", (long long) this, (long long) ptr, typeid(T).name());
                return ptr;
            }
            
            virtual ~SpecificPointerManager() {
                printf("!! %lld deleting pointer %lld -- %s\n", (long long) this, (long long) ptr, typeid(T).name());
                if (ptr) {
                    delete ptr;
                    ptr = NULL;
                }
            }
        private:
            T* ptr;
    };
    
/*    template <typename P>
    class SpecificPointerManager< std::auto_ptr<P> > : public PointerManagerCallerBase {
        public:
            SpecificPointerManager(std::auto_ptr<P>* ptr) {
                abort();
            }
            virtual PointerManagerCallerBase* clone() const { return NULL; }
            virtual void* get() { return NULL; }
    };

    template <typename P>
    class SpecificPointerManager< const std::auto_ptr<P> > : public SpecificPointerManager< std::auto_ptr<P> > {
        public:
            SpecificPointerManager(const std::auto_ptr<P>* ptr) : SpecificPointerManager< std::auto_ptr<P> >(NULL) {}
    };*/

    CastRefContainer() : pointerManager(NULL) {}
    CastRefContainer(CastRefContainer& other) : pointerManager(NULL) {
        *this = other;
    }
    CastRefContainer(const CastRefContainer& other) : pointerManager(NULL) {
        *this = other;
    }
    template <typename T>
    CastRefContainer(const std::auto_ptr<T>& ref) : pointerManager(NULL) {
        abort(); // Should never be called... this is invalid use
    }
/*    template <typename T>
    CastRefContainer(T*& ptr) : pointerManager(NULL) {
        printf("----------------- PTR thing %s\n", typeid(T*).name());
        typedef T* TPtr;
        
        if (ptr != NULL) {
            TPtr* dest_ptr = new TPtr;
            *dest_ptr = ptr;
            pointerManager = new SpecificPointerManager<TPtr>(dest_ptr);
        }
        printf("created pointer: %lld for %lld\n", (long long) pointerManager, (long long) this);
    }*/
    template <typename T>
    CastRefContainer(const T& ref) : pointerManager(NULL) {
        printf("----------------- REF thing %s\n", typeid(T*).name());
        if (&ref != NULL) {
            T* dest_ptr = new T;
            *dest_ptr = ref;
            pointerManager = new SpecificPointerManager<T>(dest_ptr);
        }
        printf("created pointer: %lld for %lld\n", (long long) pointerManager, (long long) this);
    }

    virtual ~CastRefContainer() {
        freeData();
    }
    
/*    CastRefContainer& operator=(CastRefContainer& other) {
        if (this == &other) {
            return *this;
        }
        
        freeData();
        pointerManager = other.pointerManager;
        other.pointerManager = NULL;
        printf("moved pointer: %lld into %lld from %lld\n", (long long) pointerManager, (long long) this, (long long) &other);
        return *this;
    }
*/    
    CastRefContainer& operator=(const CastRefContainer& other) {
        if (this == &other) {
            return *this;
        }
        
        freeData();
        pointerManager = other.pointerManager ? other.pointerManager->clone() : NULL;
        printf("cloned pointer: %lld to %lld for %lld\n", (long long) other.pointerManager, (long long) pointerManager, (long long) this);
        return *this;
    }
    
    virtual void* get() const {
        printf("getting pointer: %lld\n", (long long) (pointerManager ? pointerManager->get() : NULL));
        return pointerManager ? pointerManager->get() : NULL;
    }
    
    operator bool() const {
        return pointerManager != NULL;
    }
private:
    void freeData() {
        if (pointerManager) {
            printf("destroying %lld with pointer %lld\n", (long long) this, (long long) pointerManager);
            delete pointerManager;
            pointerManager = NULL;
        }
    }

    PointerManagerCallerBase* pointerManager;
};

typedef CastRefContainer(*cast_function)(CastRefContainer);
typedef std::size_t class_id;

class_id const unknown_class = (std::numeric_limits<class_id>::max)();

class class_rep;

class LUABIND_API cast_graph
{
public:
    cast_graph();
    ~cast_graph();

    // `src` and `p` here describe the *most derived* object. This means that
    // for a polymorphic type, the pointer must be cast with
    // dynamic_cast<void*> before being passed in here, and `src` has to
    // match typeid(*p).
    std::pair<CastRefContainer, int> cast(
        void* p, class_id src, class_id target
      , class_id dynamic_id, void const* dynamic_ptr) const;
    void insert(class_id src, class_id target, cast_function cast);

private:
    class impl;
    boost::scoped_ptr<impl> m_impl;
};

// Maps a type_id to a class_id. Note that this actually partitions the
// id-space into two, using one half for "local" ids; ids that are used only as
// keys into the conversion cache. This is needed because we need a unique key
// even for types that hasn't been registered explicitly.
class LUABIND_API class_id_map
{
public:
    class_id_map();

    class_id get(type_id const& type) const;
    class_id get_local(type_id const& type);
    void put(class_id id, type_id const& type);

private:
    typedef std::map<type_id, class_id> map_type;
    map_type m_classes;
    class_id m_local_id;

    static class_id const local_id_base;
};

inline class_id_map::class_id_map()
  : m_local_id(local_id_base)
{}

inline class_id class_id_map::get(type_id const& type) const
{
    map_type::const_iterator i = m_classes.find(type);
    if (i == m_classes.end() || i->second >= local_id_base)
        return unknown_class;
    return i->second;
}

inline class_id class_id_map::get_local(type_id const& type)
{
    std::pair<map_type::iterator, bool> result = m_classes.insert(
        std::make_pair(type, 0));

    if (result.second)
        result.first->second = m_local_id++;

    assert(m_local_id >= local_id_base);

    return result.first->second;
}

inline void class_id_map::put(class_id id, type_id const& type)
{
    assert(id < local_id_base);

    std::pair<map_type::iterator, bool> result = m_classes.insert(
        std::make_pair(type, 0));

    assert(
        result.second
        || result.first->second == id
        || result.first->second >= local_id_base
    );

    result.first->second = id;
}

class class_map
{
public:
    class_rep* get(class_id id) const;
    void put(class_id id, class_rep* cls);

private:
    std::vector<class_rep*> m_classes;
};

inline class_rep* class_map::get(class_id id) const
{
    if (id >= m_classes.size())
        return 0;
    return m_classes[id];
}

inline void class_map::put(class_id id, class_rep* cls)
{
    if (id >= m_classes.size())
        m_classes.resize(id + 1);
    m_classes[id] = cls;
}

template <class S, class T>
struct static_cast_
{
    static CastRefContainer execute(CastRefContainer p)
    {
        return CastRefContainer(static_cast<T*>(static_cast<S*>(p.get())));
    }
};

template <class S, class T>
struct dynamic_cast_
{
    static CastRefContainer execute(CastRefContainer p)
    {
        return CastRefContainer(dynamic_cast<T*>(static_cast<S*>(p.get())));
    }
};

// Thread safe class_id allocation.
LUABIND_API class_id allocate_class_id(type_id const& cls);

template <class T>
struct registered_class
{
    static class_id const id;
};

template <class T>
class_id const registered_class<T>::id = allocate_class_id(typeid(T));

template <class T>
struct registered_class<T const>
  : registered_class<T>
{};


enum ComplexPointerTypes {
    PT_UNKNOWN,
    PT_AUTO_PTR,
    PT_BOOST_SHARED_PTR
};

template <typename T>
struct pointer_type {
    static const ComplexPointerTypes type_id = PT_UNKNOWN;
};

template <typename T>
struct pointer_type< std::auto_ptr<T> > {
    static const ComplexPointerTypes type_id = PT_AUTO_PTR;
};

template <typename T>
struct pointer_type< boost::shared_ptr<T> > {
    static const ComplexPointerTypes type_id = PT_BOOST_SHARED_PTR;
};

struct PointerDescriptor {
    ComplexPointerTypes pointer_type;
    class_id target_id;
    
    PointerDescriptor(const PointerDescriptor& other) {
        *this = other;
    }
    PointerDescriptor(ComplexPointerTypes pointer_type, class_id target_id) :
      pointer_type(pointer_type),
      target_id(target_id) {}
};

/**
 * Retrieves the target class_id that pointer points at. If pointer is not a pointer
 * to another class_id, this function returns false.
 */
bool get_pointed_types(class_id pointer, std::vector<PointerDescriptor>& target);

/**
 * Registeres a new class relation between a pointer class_id and a target class_id.
 */
void register_registered_class_pointer_relation(class_id pointer, ComplexPointerTypes pointer_type, class_id target);

/**
 * Registeres a new class relation between a pointer class_id and a target class_id, inferring all parameters
 * from the template arguments.
 */
template <typename T>
void register_registered_class_pointer_relation(class_id target) {
    register_registered_class_pointer_relation(registered_class<T>::id, pointer_type<T>::type_id, target);
}


}} // namespace luabind::detail

#endif // LUABIND_INHERITANCE_090217_HPP
