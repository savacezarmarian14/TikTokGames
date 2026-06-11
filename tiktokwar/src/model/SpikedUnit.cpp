/**
 * @file SpikedUnit.cpp
 * @brief Implements the spiked unit type.
 */

#include "model/SpikedUnit.hpp"

namespace tw {

SpikedUnit::SpikedUnit(int id,
                       Team owner,
                       int sourceTowerId,
                       int targetTowerId,
                       float speed)
    : Unit(id, owner, sourceTowerId, targetTowerId, speed, 10, 10) {}

UnitKind SpikedUnit::kind() const {
    return UnitKind::Spiked;
}

} // namespace tw
