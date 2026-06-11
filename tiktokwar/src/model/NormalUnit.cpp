/**
 * @file NormalUnit.cpp
 * @brief Implements the normal unit type.
 */

#include "model/NormalUnit.hpp"

namespace tw {

NormalUnit::NormalUnit(int id,
                       Team owner,
                       int sourceTowerId,
                       int targetTowerId,
                       float speed)
    : Unit(id, owner, sourceTowerId, targetTowerId, speed, 1, 1) {}

UnitKind NormalUnit::kind() const {
    return UnitKind::Normal;
}

} // namespace tw
