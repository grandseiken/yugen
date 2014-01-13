#ifndef PERLIN_H
#define PERLIN_H

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

// Default weighting scheme computes average of layers. Any weighting scheme
// must have a const operator() taking the layer and returning a scalar.
template<typename T>
struct DefaultWeights {
  T operator()(y::size layer) const
  {
    (void)layer;
    return T(1);
  }
};

// Weighting scheme that uses a linear sequence of weights.
template<typename T>
struct LinearWeights {
  LinearWeights(const T& first, const T& delta)
    : first(first)
    , delta(delta)
  {
  }

  T operator()(y::size layer) const
  {
    return first + delta * layer;
  }

  T first;
  T delta;
};

// No smoothing. Any smoothing scheme must have a const operator() taking the
// cell-coordinate in [0, 1)^N and returning a smoothed coordinate in [0, 1)^N.
// The function should have zero derivative at the endpoints {0, 1}. It must
// also typedef the scalar coefficient type as coefficient_type.
template<typename T>
struct NoSmoothing {
  typedef T coefficient_type;

  template<y::size N>
  const y::vec<T, N>& operator()(const y::vec<T, N>& coordinate) const
  {
    return coordinate;
  }
};

// Power smoothing. Negative powers are trippy.
template<typename T>
struct PowerSmoothing {
  typedef T coefficient_type;

  PowerSmoothing()
    : power(2)
  {
  }

  PowerSmoothing(const T& power)
    : power(power)
  {
  }

  template<y::size N>
  y::vec<T, N> operator()(const y::vec<T, N>& coordinate) const
  {
    T a = power < 0 ? -1 : 1;
    return (a + power) * y::pow(coordinate, power) -
        power * y::pow(coordinate, a + power);
  }

  T power;
};

// Arbitrary-dimensional perlin noise over type T, using random number
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
  template<y::size N, typename Smoothing = PowerSmoothing<T>>
  void generate_layer(field& output,
                      y::size grid_count, y::size scale,
                      const Smoothing& smoothing);

  // Combine several perlin layers into one. The first layer is as above; each
  // subsequent layer has scale doubled, and they are combined using the given
  // weighting scheme.
  template<y::size N, typename Weights = DefaultWeights<T>,
                      typename Smoothing = PowerSmoothing<T>>
  void generate_perlin(field& output,
                       y::size grid_count, y::size scale, y::size layers);
  template<y::size N, typename Weights = DefaultWeights<T>,
                      typename Smoothing = PowerSmoothing<T>>
  void generate_perlin(field& output,
                       y::size grid_count, y::size scale, y::size layers,
                       const Weights& weights);
  template<y::size N, typename Weights = DefaultWeights<T>,
                      typename Smoothing = PowerSmoothing<T>>
  void generate_perlin(field& output,
                       y::size grid_count, y::size scale, y::size layers,
                       const Weights& weights, const Smoothing& smoothing);

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
template<y::size N, typename Smoothing>
void Perlin<T, G>::generate_layer(field& output,
                                  y::size grid_count, y::size scale,
                                  const Smoothing& smoothing)
{
  typedef typename Smoothing::coefficient_type coefficient;
  typedef y::vec<y::size, N> sv;
  typedef y::vec<coefficient, N> fv;

  sv grid_corners;
  sv grid_size;
  sv scale_vec;
  y::size scale_power = 1;
  y::size total_power = 1;
  for (y::size i = 0; i < N; ++i) {
    grid_corners[i] = 2;
    grid_size[i] = grid_count;
    scale_vec[i] = scale;
    total_power *= scale * grid_count;
    scale_power *= scale;
  }

  field f;
  generate_field<N>(f, grid_count);
  output.reserve(total_power);

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
  typedef y::vector<coefficient> coefficient_list;
  y::vector<coefficient_list> coefficient_map;
  coefficient_map.reserve(scale_power);
  for (auto it = y::cartesian(scale_vec); it; ++it) {
    // Position of offset within grid-cell, in [0, 1)^N and apply smoothing
    // function.
    fv cell_point = smoothing(fv(*it) / scale);

    // Loop through the 2^N grid-corners (*it in {0, 1}^N), and add their
    // coefficients to the map. These coefficients linearly interpolate the
    // values at the corners through the cell (with respect to the smoothed
    // offset).
    coefficient_map.push_back(coefficient_list());
    coefficient_list& list = *(coefficient_map.end() - 1);
    for (auto it = y::cartesian(grid_corners); it; ++it) {
      coefficient compound = 1;
      for (y::size i = 0; i < N; ++i) {
        y::size c = (*it)[i];
        compound *= c * cell_point[i] + (1 - c) * (1 - cell_point[i]);
      }
      list.emplace_back(compound);
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
    output.push_back(value);
  }
}

template<typename T, typename G>
template<y::size N, typename Weights, typename Smoothing>
void Perlin<T, G>::generate_perlin(
    field& output, y::size grid_count, y::size scale, y::size layers)
{
  generate_perlin<N, Weights, Smoothing>(
      output, grid_count, scale, layers, Weights());
}

template<typename T, typename G>
template<y::size N, typename Weights, typename Smoothing>
void Perlin<T, G>::generate_perlin(
    field& output, y::size grid_count, y::size scale, y::size layers,
    const Weights& weights)
{
  generate_perlin<N, Weights, Smoothing>(
      output, grid_count, scale, layers, weights, Smoothing());
}

template<typename T, typename G>
template<y::size N, typename Weights, typename Smoothing>
void Perlin<T, G>::generate_perlin(
    field& output, y::size grid_count, y::size scale, y::size layers,
    const Weights& weights, const Smoothing& smoothing)
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

    generate_layer<N>(*(layer_data.end() - 1),
                      layer_grid_count, layer_scale, smoothing);
    layer_side_lengths.emplace_back(layer_grid_count * layer_scale);
    pow *= 2;
  }
  if (layers) {
    output.reserve(layer_data[0].size());
  }

  y::vec<y::size, N> size;
  for (y::size i = 0; i < N; ++i) {
    size[i] = grid_count * scale;
  }
  decltype(weights(0)) weight_d = 0;
  for (y::size i = 0; i < layers; ++i) {
    weight_d += weights(i);
  }
  for (auto it = y::cartesian(size); it; ++it) {
    T out{};
    for (y::size i = 0; i < layers; ++i) {
      out += weights(i) *
          field_lookup<N>(layer_data[i], layer_side_lengths[i], *it);
    }
    output.push_back(out / weight_d);
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
  output.reserve(count);

  for (y::size i = 0; i < count; ++i) {
    output.push_back(_generator());
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
