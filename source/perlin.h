#ifndef PERLIN_H
#define PERLIN_H

#include "common.h"
#include "vector.h"
#include <random>

// Default real [0, 1) generator.
template<typename T>
struct DefaultGenerator {
  DefaultGenerator(y::size seed)
    : generator(seed)
    , distribution(T(0), T(1))
  {}

  DefaultGenerator()
    : distribution(T(0), T(1))
  {}

  std::default_random_engine generator;
  std::uniform_real_distribution<T> distribution;

  T operator()()
  {
    return distribution(generator);  
  }
};

// Generalised vector generator.
template<typename T, y::size N>
struct DefaultGenerator<y::vec<T, N>> : DefaultGenerator<T> {
  using DefaultGenerator<T>::DefaultGenerator;

  typedef y::vec<T, N> V;
  y::vec<T, N> operator()()
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = DefaultGenerator<T>::operator()();
    }
    return v;
  }
};

// Arbitrary-dimensional perlin noise over values of type T, using random number
// generator of type G. G must have overload T operator()() which generates
// the next random T (in the desired range).
// TODO: simplex noise.
template<typename T, typename G = DefaultGenerator<T>>
class Perlin {
public:

  Perlin();
  Perlin(y::size seed);
  Perlin(const G& generator);

  typedef y::vector<T> field;

  // Generate a single N-dimensional layer of perlin noise. The resulting noise
  // has grid_count independent points per dimension, scaled by scale, so the
  // output will have (grid_count * scale)^N values.
  template<y::size N>
  void generate_layer(field& output,
                      y::size grid_count, y::size scale);

  // Combine several perlin layers into one. The first layer is as above; each
  // subsequent layer has scale doubled, and they are all averaged.
  template<y::size N>
  void generate_perlin(field& output,
                       y::size grid_count, y::size scale, y::size layers);

private:

  G _generator;

  // Generates a field of given dimension and side length.
  template<y::size N>
  void generate_field(field& output, y::size side_length);

  // Look up the value stored in a field.
  template<y::size N>
  const T& field_lookup(const field& f, y::size side_length,
                        const y::vec<y::size, N>& index) const;

};

template<typename T, typename G>
Perlin<T, G>::Perlin()
{
}

template<typename T, typename G>
Perlin<T, G>::Perlin(y::size seed)
  : _generator(seed)
{
}

template<typename T, typename G>
Perlin<T, G>::Perlin(const G& generator)
  : _generator(generator)
{
}

template<typename T, typename G>
template<y::size N>
void Perlin<T, G>::generate_layer(field& output,
                                  y::size grid_count, y::size scale)
{
  typedef y::vec<y::size, N> sv;
  typedef y::vec<float, N> fv;

  field f;
  generate_field<N>(f, grid_count);

  sv grid_corners;
  sv grid_size;
  sv scale_vec;
  for (y::size i = 0; i < N; ++i) {
    grid_corners[i] = 2;
    grid_size[i] = grid_count;
    scale_vec[i] = scale;
  }

  // Special case: if scale is 1, just output directly; no interpolation is
  // necessary.
  if (scale == 1) {
    generate_field<N>(output, grid_count);
    return;
  }

  // Special case: if grid_count is 1 there is only one value; just output it.
  if (grid_count == 1) {
    const T& value = field_lookup(f, grid_count, sv());
    for (auto it = y::cartesian(scale_vec); it; ++it) {
      output.emplace_back(value);
    }
    return;
  }

  /// Build lookup table for each offset.
  typedef y::vector<float> coefficient_list;
  y::vector<coefficient_list> coefficient_map;
  for (auto it = y::cartesian(scale_vec); it; ++it) {
    // Position of offset within grid-cell, in [0, 1)^N.
    fv cell_point = fv(*it) / scale;

    // Applying smoothing function to soften the interpolation.
    // TODO: allow custom smoothing functions?
    cell_point = 3.f * y::pow(cell_point, 2.f) - 2.f * y::pow(cell_point, 3.f);

    // Loop through the 2^N grid-corners (*it in {0, 1}^N), and add their
    // coefficients to the map. These coefficients linearly interpolate the
    // values at the corners through the cell (with respect to the smoothed
    // offset).
    coefficient_map.emplace_back();
    coefficient_list& list = *(coefficient_map.end() - 1);
    for (auto it = y::cartesian(grid_corners); it; ++it) {
      float coefficient = 1;
      for (y::size i = 0; i < N; ++i) {
        y::size c = (*it)[i];
        coefficient *= c * cell_point[i] + (1 - c) * (1 - cell_point[i]);
      }
      list.push_back(coefficient);
    }
  }

  // Loop through all the points in the output.
  for (auto it = y::cartesian(scale * grid_size); it; ++it) {
    y::size power = 1;
    y::size map_index = 0;
    for (y::size i = 0; i < N; ++i) {
      map_index += power * ((*it)[i] % scale);
      power *= scale;
    }
    sv truncated_grid = *it / scale;

    // Loop through the 2^N grid-corners (*it in {0, 1}^N), and add their
    // contribution to the value.
    T value{};
    y::size i = 0;
    const coefficient_list& list = coefficient_map[map_index];
    for (auto it = y::cartesian(grid_corners); it; ++it) {
      sv corner = *it + truncated_grid;
      for (y::size j = 0; j < N; ++j) {
        corner[j] %= grid_count;
      }
      value += list[i++] * field_lookup(f, grid_count, corner);
    }
    output.emplace_back(value);
  }
}

template<typename T, typename G>
template<y::size N>
void Perlin<T, G>::generate_perlin(
    field& output, y::size grid_count, y::size scale, y::size layers)
{
  y::vector<field> layer_data;
  y::vector<y::size> layer_side_lengths;

  y::size pow = 1;
  for (y::size i = 0; i < layers; ++i) {
    layer_data.emplace_back();

    // If we can't divide exactly, make sure we have enough to sample.
    y::size layer_grid_count = grid_count / pow;
    y::size layer_scale = pow * scale;
    if (grid_count % pow) {
      ++layer_grid_count;
    }

    generate_layer<N>(*(layer_data.end() - 1), layer_grid_count, layer_scale);
    layer_side_lengths.emplace_back(layer_grid_count * layer_scale);
    pow *= 2;
  }

  // TODO: allow custom weighting?
  y::vec<y::size, N> size;
  for (y::size i = 0; i < N; ++i) {
    size[i] = grid_count * scale;
  }
  for (auto it = y::cartesian(size); it; ++it) {
    T out{};
    for (y::size i = 0; i < layers; ++i) {
      out += field_lookup<N>(layer_data[i], layer_side_lengths[i], *it);
    }
    output.emplace_back(out / layers);
  }
}

template<typename T, typename G>
template<y::size N>
void Perlin<T, G>::generate_field(field& output, y::size side_length)
{
  y::size count = 1;
  for (y::size i = 0; i < N; ++i) {
    count *= side_length;
  }

  for (y::size i = 0; i < count; ++i) {
    output.emplace_back(_generator());
  }
}

template<typename T, typename G>
template<y::size N>
const T& Perlin<T, G>::field_lookup(const field& f, y::size side_length,
                                    const y::vec<y::size, N>& index) const
{
  y::size field_index = 0;
  y::size power = 1;
  for (y::size i = 0; i < N; ++i) {
    field_index += power * index[i];
    power *= side_length;
  }
  return f[field_index];
}

#endif
