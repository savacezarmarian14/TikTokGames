#include "model/Unit.hpp"
#include <algorithm>

namespace tw {

Unit::Unit(int id, Team owner, int sourceTowerId, int targetTowerId, float speed, int damage, bool healing)
    : id_(id), owner_(owner), sourceTowerId_(sourceTowerId), targetTowerId_(targetTowerId),
      speed_(speed), damage_(damage), healing_(healing), alive_(true) {}

int Unit::id() const { return id_; }
Team Unit::owner() const { return owner_; }
int Unit::sourceTowerId() const { return sourceTowerId_; }
int Unit::targetTowerId() const { return targetTowerId_; }
float Unit::progress() const { return progress_; }
void Unit::setProgress(float value) { progress_ = std::max(0.0f, std::min(1.0f, value)); }
float Unit::speed() const { return speed_; }
int Unit::damage() const { return damage_; }
bool Unit::isHealing() const { return healing_; }
bool Unit::isAlive() const { return alive_; }
void Unit::destroy() { alive_ = false; }

} // namespace tw
