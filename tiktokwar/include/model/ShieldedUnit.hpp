#pragma once

#include "model/Unit.hpp"

namespace tw {

/**
 * @brief A durable forward unit with a shield in front.
 */
class ShieldedUnit : public Unit {
public:
    /**
     * @brief Constructs a shielded unit with 5 hit points and 5 damage.
     */
    ShieldedUnit(int id,
                 Team owner,
                 int sourceTowerId,
                 int targetTowerId,
                 float speed);

    UnitKind kind() const override;
};

} // namespace tw
