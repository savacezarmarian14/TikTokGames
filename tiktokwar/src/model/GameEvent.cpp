#include "model/GameEvent.hpp"

namespace tw {

GameEvent::GameEvent(EventType type, int towerId, Team team, int value)
    : type_(type), towerId_(towerId), team_(team), value_(value) {}

EventType GameEvent::type() const { return type_; }
int GameEvent::towerId() const { return towerId_; }
Team GameEvent::team() const { return team_; }
int GameEvent::value() const { return value_; }

} // namespace tw
