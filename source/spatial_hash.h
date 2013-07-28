#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "common.h"

template<typename T>
class SpatialHash {
public:

  SpatialHash(y::size bucket_size);

  // Add or update an object with given bounding box.
  void update(T t, const y::wvec2& min, const y::wvec2& max);

  // Remove the object.
  void remove(T t);

  // Clear all objects.
  void clear();

  // Find all objects which might overlap the given rectangle.
  // TODO: have a search iterator. Once that is done, definitely replace
  // OrderedGeometry with this.
  void search(y::vector<T>& output,
              const y::wvec2& min, const y::wvec2& max) const;

private:

  y::int32 _bucket_size;

  struct entry {
    y::wvec2 min;
    y::wvec2 max;
  };
  typedef y::map<T, entry> bucket;

  // Spatial buckets.
  y::map<y::ivec2, bucket> _buckets;

  // Bucket for objects which are too large to fit in a bucket and must always
  // be checked.
  bucket _fallback_bucket;

};

template<typename T>
SpatialHash<T>::SpatialHash(y::size bucket_size)
  : _bucket_size(bucket_size)
{
}

template<typename T>
void SpatialHash<T>::update(T t, const y::wvec2& min, const y::wvec2& max)
{
  // We store objects in a bucket based on their centre, check for objects
  // in adjacent buckets, and keep a separate bucket for objects which
  // could overlap more than the neighbouring buckets.
  // An alternative strategy would be to do away with the fallback bucket
  // and associate a list of all overlapping buckets with each object, but
  // this is faster if we choose an appropriate bucket size such that using
  // the fallback bucket is relatively rare.
  remove(t);
  y::wvec2 half_size = y::abs(max - min) / 2;
  y::wvec2 origin = (min + max) / 2;

  if (half_size[xx] >= _bucket_size || half_size[yy] >= _bucket_size) {
    _fallback_bucket.insert(y::make_pair(t, entry{min, max}));
    return;
  }

  y::ivec2 coord{y::int32(origin[xx]) / _bucket_size,
                 y::int32(origin[yy]) / _bucket_size};
  _buckets[coord].insert(y::make_pair(t, entry{min, max}));
}

template<typename T>
void SpatialHash<T>::remove(T t)
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

template<typename T>
void SpatialHash<T>::clear()
{
  _buckets.clear();
  _fallback_bucket.clear();
}

template<typename T>
void SpatialHash<T>::search(y::vector<T>& output,
                            const y::wvec2& min, const y::wvec2& max) const
{
  y::ivec2 min_coord{y::int32(min[xx]) / _bucket_size,
                     y::int32(min[yy]) / _bucket_size};
  y::ivec2 max_coord{y::int32(max[xx]) / _bucket_size,
                     y::int32(max[yy]) / _bucket_size};
  // We have to extend the search by one in each direction because of overlaps.
  min_coord -= y::ivec2{1, 1};
  max_coord += y::ivec2{2, 2};

  for (auto it = y::cartesian(min_coord, max_coord); it; ++it) {
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

#endif
