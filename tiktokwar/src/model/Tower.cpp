#include "model/Tower.hpp"
#include <algorithm>

namespace tw {

Tower::Tower(int id, Team team, const std::string& name, int maxHp)
    : id_(id),
      team_(team),
      name_(name),
      hp_(std::max(0, maxHp)),
      maxHp_(std::max(0, maxHp)),
      alive_(maxHp_ > 0) {}

int Tower::id() const { return id_; }
Team Tower::team() const { return team_; }
const std::string& Tower::name() const { return name_; }
int Tower::hp() const { return hp_; }
int Tower::maxHp() const { return maxHp_; }
bool Tower::isAlive() const { return alive_; }

void Tower::takeDamage(int amount) {
    if (!alive_ || amount <= 0) {
        return;
    }
    hp_ = std::max(0, hp_ - amount);
    if (hp_ == 0) {
        alive_ = false;
    }
}

void Tower::heal(int amount) {
    if (!alive_ || amount <= 0) {
        return;
    }
    hp_ = std::min(maxHp_, hp_ + amount);
}

void Tower::reset() {
    hp_ = maxHp_;
    alive_ = maxHp_ > 0;
}

} // namespace tw
