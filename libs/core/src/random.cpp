#include "core/random.hpp"

namespace fetch {
namespace random {

LaggedFibonacciGenerator<> Random::generator = LaggedFibonacciGenerator<>();

}  // namespace random
}  // namespace fetch
