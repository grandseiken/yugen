#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "vector.h"
#include <boost/iterator/iterator_facade.hpp>

// Container for fast lookup of objects in regions of space. T is the stored
// object type, V is the type of the coordinate line, and V the dimensions; that
// is, the coordinate system is defined in terms of y::vec<V, N>.
template<typename T, typename V, y::size N>
class SpatialHash {
public:

  typedef y::vec<V, N> coord;

  SpatialHash(y::size bucket_size);

  // Add or update an object with given bounding box.
  void update(const T& t, const coord& min, const coord& max);
  // Remove the object.
  void remove(const T& t);
  // Clear all objects.
  void clear();

  class iterator : public boost::iterator_facade<
      iterator, const T&, boost::forward_traversal_tag> {
  public:

    explicit operator bool() const;

  private:

    struct entry {
      coord min;
      coord max;
    };
    typedef y::map<T, entry> bucket;

    iterator(const SpatialHash& hash,
             const coord& min, const coord& max);

    friend class SpatialHash<T, V, N>;
    friend class boost::iterator_core_access;

    void seek_to_next(bool inner);
    void increment();
    bool equal(const iterator& arg) const;
    const T& dereference() const;

    const SpatialHash<T, V, N>& _hash;
    coord _min;
    coord _max;
    y::vec_iterator<y::int32, N, true> _i;
    typename bucket::const_iterator _j;

  };

  // Find all objects which overlap the given bounding box.
  void search(y::vector<T>& output,
              const coord& min, const coord& max) const;
  iterator search(const coord& min, const coord& max) const;

private:


  typedef y::vec<y::int32, N> bucket_coord;
  y::int32 _bucket_size;

  typedef typename iterator::entry entry;
  typedef typename iterator::bucket bucket;

  // Spatial buckets.
  y::map<bucket_coord, bucket> _buckets;

  // Bucket for objects which are too large to fit in a bucket and must always
  // be checked.
  bucket _fallback_bucket;

};

template<typename T, typename V, y::size N>
SpatialHash<T, V, N>::SpatialHash(y::size bucket_size)
  : _bucket_size(bucket_size)
{
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::update(const T& t, const coord& min, const coord& max)
{
  // We store objects in a bucket based on their centre, check for objects
  // in adjacent buckets, and keep a separate bucket for objects which
  // could overlap more than the neighbouring buckets.
  // An alternative strategy would be to do away with the fallback bucket
  // and associate a list of all overlapping buckets with each object, but
  // this is faster if we choose an appropriate bucket size such that using
  // the fallback bucket is relatively rare.
  remove(t);
  coord half_size = y::abs(max - min) / 2;
  coord origin = (min + max) / 2;

  if (half_size[xx] >= _bucket_size || half_size[yy] >= _bucket_size) {
    _fallback_bucket.insert(y::make_pair(t, entry{min, max}));
    return;
  }

  bucket_coord bucket = bucket_coord(origin) / _bucket_size;
  _buckets[bucket].insert(y::make_pair(t, entry{min, max}));
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::remove(const T& t)
{
  for (auto it = _buckets.begin(); it != _buckets.end();) {
    // Also clean up empty buckets while we're at it.
    if (it->second.erase(t) && it->second.empty()) {
      it = _buckets.erase(it);
    }
    else {
      ++it;
    }
  }
  _fallback_bucket.erase(t);
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::clear()
{
  _buckets.clear();
  _fallback_bucket.clear();
}

template<typename T, typename V, y::size N>
SpatialHash<T, V, N>::iterator::operator bool() const
{
  return _j != _hash._fallback_bucket.end();
}

template<typename T, typename V, y::size N>
SpatialHash<T, V, N>::iterator::iterator(const SpatialHash& hash,
                                         const coord& min, const coord& max)
  : _hash(hash)
  , _min(min)
  , _max(max)
{
  bucket_coord min_bucket = bucket_coord(min) / _hash._bucket_size;
  bucket_coord max_bucket = bucket_coord(max) / _hash._bucket_size;
  for (y::size i = 0; i < N; ++i) {
    min_bucket[i] -= 1;
    max_bucket[i] += 2;
  }

  _i = y::cartesian(min_bucket, max_bucket);
  seek_to_next(false);
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::iterator::seek_to_next(bool inner)
{
  // Replicates the search functions below in iterator form. Iterators have the
  // weirdest code structure. I think any sequence of nested loops with
  // arbitrary break/continue conditions can be transformed to an iterator via
  // a seek function of the below form. It's a very unintuitive kind of logic,
  // though.
  if (inner) {
    goto inner;
  }

  outer:
  if (_i && _hash._buckets.find(*_i) == _hash._buckets.end()) {
    ++_i;
    goto outer;
  }
  _j = _i ? _hash._buckets.find(*_i)->second.begin() :
            _hash._fallback_bucket.begin();

  inner:
  if (_i && _j == _hash._buckets.find(*_i)->second.end()) {
    ++_i;
    goto outer;
  }
  if (_j != _hash._fallback_bucket.end() &&
      !(_j->second.max > _min && _j->second.min < _max)) {
    ++_j;
    goto inner;
  }
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::iterator::increment()
{
  ++_j;
  seek_to_next(true);
}

template<typename T, typename V, y::size N>
bool SpatialHash<T, V, N>::iterator::equal(const iterator& arg) const
{
  return _j == arg._j;
}

template<typename T, typename V, y::size N>
const T& SpatialHash<T, V, N>::iterator::dereference() const
{
  return _j->first;
}

template<typename T, typename V, y::size N>
void SpatialHash<T, V, N>::search(y::vector<T>& output,
                                  const coord& min, const coord& max) const
{
  bucket_coord min_bucket = bucket_coord(min) / _bucket_size;
  bucket_coord max_bucket = bucket_coord(max) / _bucket_size;
  // We have to extend the search by one in each direction because of overlaps.
  for (y::size i = 0; i < N; ++i) {
    min_bucket[i] -= 1;
    max_bucket[i] += 2;
  }

  for (auto it = y::cartesian(min_bucket, max_bucket); it; ++it) {
    auto jt = _buckets.find(*it);
    if (jt != _buckets.end()) {
      for (const auto& p : jt->second) {
        if (p.second.max > min && p.second.min < max) {
          output.emplace_back(p.first);
        }
      }
    }
  }

  for (const auto& p : _fallback_bucket) {
    if (p.second.max > min && p.second.min < max) {
      output.emplace_back(p.first);
    }
  }
}

template<typename T, typename V, y::size N>
typename SpatialHash<T, V, N>::iterator SpatialHash<T, V, N>::search(
    const coord& min, const coord& max) const
{
  return iterator(*this, min, max);
}

#endif
