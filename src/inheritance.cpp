// Copyright Daniel Wallin 2009. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define LUABIND_BUILDING

#include <limits>
#include <map>
#include <vector>
#include <queue>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <luabind/typeid.hpp>
#include <luabind/detail/inheritance.hpp>

namespace luabind { namespace detail {

class_id const class_id_map::local_id_base =
    std::numeric_limits<class_id>::max() / 2;

namespace
{

  struct edge
  {
      edge(class_id target, cast_function cast)
        : target(target)
        , cast(cast)
      {}

      class_id target;
      cast_function cast;
  };

  bool operator<(edge const& x, edge const& y)
  {
      return x.target < y.target;
  }

  struct vertex
  {
      vertex(class_id id)
        : id(id)
      {}

      class_id id;
      std::vector<edge> edges;
  };

  typedef std::pair<std::ptrdiff_t, int> cache_entry;

  class cache
  {
  public:
      static std::ptrdiff_t const unknown;
      static std::ptrdiff_t const invalid;

      cache_entry get(
          class_id src, class_id target, class_id dynamic_id
        , std::ptrdiff_t object_offset) const;

      void put(
          class_id src, class_id target, class_id dynamic_id
        , std::ptrdiff_t object_offset
        , std::ptrdiff_t offset, int distance);

      void invalidate();

  private:
      typedef boost::tuple<
          class_id, class_id, class_id, std::ptrdiff_t> key_type;
      typedef std::map<key_type, cache_entry> map_type;
      map_type m_cache;
  };

  std::ptrdiff_t const cache::unknown =
      std::numeric_limits<std::ptrdiff_t>::max();
  std::ptrdiff_t const cache::invalid = cache::unknown - 1;

  cache_entry cache::get(
      class_id src, class_id target, class_id dynamic_id
    , std::ptrdiff_t object_offset) const
  {
      map_type::const_iterator i = m_cache.find(
          key_type(src, target, dynamic_id, object_offset));
      return i != m_cache.end() ? i->second : cache_entry(unknown, -1);
  }

  void cache::put(
      class_id src, class_id target, class_id dynamic_id
    , std::ptrdiff_t object_offset, std::ptrdiff_t offset, int distance)
  {
      m_cache.insert(std::make_pair(
          key_type(src, target, dynamic_id, object_offset)
        , cache_entry(offset, distance)
      ));
  }

  void cache::invalidate()
  {
      m_cache.clear();
  }

} // namespace unnamed

class cast_graph::impl
{
public:
    std::pair<void*, int> cast(
        void* p, class_id src, class_id target
      , class_id dynamic_id, void const* dynamic_ptr) const;
    void insert(class_id src, class_id target, cast_function cast);

private:
    std::vector<vertex> m_vertices;
    mutable cache m_cache;
};

namespace
{

  struct queue_entry
  {
      queue_entry(void* p, class_id vertex_id, int distance)
        : p(p)
        , vertex_id(vertex_id)
        , distance(distance)
      {}

      void* p;
      class_id vertex_id;
      int distance;
  };

} // namespace unnamed

std::pair<void*, int> cast_graph::impl::cast(
    void* const p, class_id src, class_id target
  , class_id dynamic_id, void const* dynamic_ptr) const
{
    if (src == target)
        return std::make_pair(p, 0);

    if (src >= m_vertices.size() || target >= m_vertices.size())
        return std::pair<void*, int>((void*)0, -1);

    std::ptrdiff_t const object_offset =
        (char const*)dynamic_ptr - (char const*)p;

    cache_entry cached = m_cache.get(src, target, dynamic_id, object_offset);

    if (cached.first != cache::unknown)
    {
        if (cached.first == cache::invalid)
            return std::pair<void*, int>((void*)0, -1);
        return std::make_pair((char*)p + cached.first, cached.second);
    }

    std::queue<queue_entry> q;
    q.push(queue_entry(p, src, 0));

    boost::dynamic_bitset<> visited(m_vertices.size());

    while (!q.empty())
    {
        queue_entry const qe = q.front();
        q.pop();

        visited[qe.vertex_id] = true;
        vertex const& v = m_vertices[qe.vertex_id];

        if (v.id == target)
        {
            m_cache.put(
                src, target, dynamic_id, object_offset
              , (char*)qe.p - (char*)p, qe.distance
            );

            return std::make_pair(qe.p, qe.distance);
        }

        BOOST_FOREACH(edge const& e, v.edges)
        {
            if (visited[e.target])
                continue;
            if (void* casted = e.cast(qe.p))
                q.push(queue_entry(casted, e.target, qe.distance + 1));
        }
    }

    m_cache.put(src, target, dynamic_id, object_offset, cache::invalid, -1);

    return std::pair<void*, int>((void*)0, -1);
}

void cast_graph::impl::insert(
    class_id src, class_id target, cast_function cast)
{
    class_id const max_id = std::max(src, target);

    if (max_id >= m_vertices.size())
    {
        m_vertices.reserve(max_id + 1);
        for (class_id i = m_vertices.size(); i < max_id + 1; ++i)
            m_vertices.push_back(vertex(i));
    }

    std::vector<edge>& edges = m_vertices[src].edges;

    std::vector<edge>::iterator i = std::lower_bound(
        edges.begin(), edges.end(), edge(target, 0)
    );

    if (i == edges.end() || i->target != target)
    {
        edges.insert(i, edge(target, cast));
        m_cache.invalidate();
    }
}

std::pair<void*, int> cast_graph::cast(
    void* p, class_id src, class_id target
  , class_id dynamic_id, void const* dynamic_ptr) const
{
    return m_impl->cast(p, src, target, dynamic_id, dynamic_ptr);
}

void cast_graph::insert(class_id src, class_id target, cast_function cast)
{
    m_impl->insert(src, target, cast);
}

cast_graph::cast_graph()
  : m_impl(new impl)
{}

cast_graph::~cast_graph()
{}

LUABIND_API class_id allocate_class_id(type_id const& cls)
{
    typedef std::map<type_id, class_id> map_type;

    static map_type registered;
    static class_id id = 0;

    std::pair<map_type::iterator, bool> inserted = registered.insert(
        std::make_pair(cls, id));

    if (inserted.second)
        ++id;

    return inserted.first->second;
}

struct WeakReentrantLock {
public:
    boost::mutex mutex;
    boost::condition_variable release_notification;
    
    /** 
     * If -1, the reentrant lock is locked hard (no other entries are allowed). 
     * If positive, this counts the number of weak reentrant locks on the lock.
     */
    int weakLockCount; 
    
    class Lock {
    public:
        typedef boost::unique_lock<boost::mutex> LockType;
        
        Lock& operator=(Lock& other) {
          relockRef = other.relockRef;
          wasHard = other.wasHard;
          lock = other.lock;
          other.lock = NULL;
          
          return *this;
        }
        
        Lock(Lock& other) : relockRef(other.relockRef), lock(other.lock), wasHard(other.wasHard) {
          other.lock = NULL;  
        }
        
        Lock(WeakReentrantLock& reLock, bool lockHard) : relockRef(&reLock), lock(new LockType(reLock.mutex)), wasHard(lockHard) {
          if (lockHard) {
            while (relockRef->weakLockCount != 0) {
              relockRef->release_notification.wait(*lock);
            }
            relockRef->weakLockCount = -1;
          } else {
            while (relockRef->weakLockCount < 0) {
              relockRef->release_notification.wait(*lock);
            }
            relockRef->weakLockCount += 1;
            lock->unlock();
          }
        }
        
        virtual ~Lock() {
          if (lock) {
            if (wasHard) {
              relockRef->weakLockCount = 0;
            } else {
              lock->lock();
              relockRef->weakLockCount -= 1;
            }
            relockRef->release_notification.notify_all();
            
            delete lock;
          }
        }
    private:
        bool dead;
    
        WeakReentrantLock* relockRef;
        LockType* lock;
        
        bool wasHard; //! Saves if this was a hard or weak lock
    };

    WeakReentrantLock() : weakLockCount() {}
};

typedef std::map<class_id, std::vector<PointerDescriptor> > RegisteredClassPointerRelationType;
static RegisteredClassPointerRelationType registered_class_pointer_relations;
static WeakReentrantLock registered_class_pointer_relations_lock;

bool get_pointed_types(class_id pointer, std::vector<PointerDescriptor>& target) {
  WeakReentrantLock::Lock lock(registered_class_pointer_relations_lock, false);
  
  RegisteredClassPointerRelationType::iterator found = registered_class_pointer_relations.find(pointer);
  if (found == registered_class_pointer_relations.end()) {
    return false;
  }
  target = found->second;
  return true;
}

void register_registered_class_pointer_relation(class_id pointer, ComplexPointerTypes pointer_type, class_id target) {
  WeakReentrantLock::Lock lock(registered_class_pointer_relations_lock, true);
  registered_class_pointer_relations[pointer].push_back(PointerDescriptor(pointer_type, target));
}

}} // namespace luabind::detail
