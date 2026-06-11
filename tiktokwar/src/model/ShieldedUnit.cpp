/**
 * @file ShieldedUnit.cpp
 * @brief Implements the shielded unit type.
 */

#include "model/ShieldedUnit.hpp"

namespace tw {

ShieldedUnit::ShieldedUnit(int id,
                           Team owner,
                           int sourceTowerId,
                           int targetTowerId,
                           float speed)
    : Unit(id, owner, sourceTowerId, targetTowerId, speed, 5, 5) {}

UnitKind ShieldedUnit::kind() const {
    return UnitKind::Shielded;
}

} // namespace tw
