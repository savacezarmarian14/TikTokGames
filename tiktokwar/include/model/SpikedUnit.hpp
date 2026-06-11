#pragma once

#include "model/Unit.hpp"

namespace tw {

/**
 * @brief A heavy attack unit with spinning spikes.
 */
class SpikedUnit : public Unit {
public:
    /**
     * @brief Constructs a spiked unit with 10 hit points and 10 damage.
     */
    SpikedUnit(int id,
               Team owner,
               int sourceTowerId,
               int targetTowerId,
               float speed);

    UnitKind kind() const override;
};

} // namespace tw
