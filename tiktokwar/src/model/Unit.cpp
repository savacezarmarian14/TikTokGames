/**
 * @file Unit.cpp
 * @brief Implements the shared base unit behavior.
 */

#include "model/Unit.hpp"
#include <algorithm>

namespace tw {

Unit::Unit(int id,
           Team owner,
           int sourceTowerId,
           int targetTowerId,
           float speed,
           int damage,
           int health)
    : id_(id),
      owner_(owner),
      sourceTowerId_(sourceTowerId),
      targetTowerId_(targetTowerId),
      progress_(0.0f),
      speed_(speed),
      damage_(damage),
      health_(std::max(1, health)),
      maxHealth_(std::max(1, health)),
      alive_(true) {}

int Unit::id() const { return id_; }
Team Unit::owner() const { return owner_; }
int Unit::sourceTowerId() const { return sourceTowerId_; }
int Unit::targetTowerId() const { return targetTowerId_; }
float Unit::progress() const { return progress_; }
void Unit::setProgress(float value) { progress_ = std::max(0.0f, std::min(1.0f, value)); }
float Unit::speed() const { return speed_; }
int Unit::damage() const { return damage_; }
int Unit::health() const { return health_; }
int Unit::maxHealth() const { return maxHealth_; }
bool Unit::isAlive() const { return alive_; }
void Unit::takeDamage(int amount) {
    health_ -= std::max(1, amount);
    if (health_ <= 0) {
        destroy();
    }
}

void Unit::destroy() {
    alive_ = false;
    health_ = 0;
}

} // namespace tw
