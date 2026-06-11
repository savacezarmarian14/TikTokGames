#pragma once

#include "model/Unit.hpp"

namespace tw {

/**
 * @brief A standard attack unit with minimal health and damage.
 */
class NormalUnit : public Unit {
public:
    /**
     * @brief Constructs a normal unit with 1 hit point and 1 damage.
     */
    NormalUnit(int id,
               Team owner,
               int sourceTowerId,
               int targetTowerId,
               float speed);

    UnitKind kind() const override;
};

} // namespace tw
