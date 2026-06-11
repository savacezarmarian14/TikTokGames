#pragma once

namespace tw {

/**
 * @brief Logical owner of a tower or unit.
 */
enum class Team {
    Red = 0,
    Blue,
    Purple,
    Green,
    None
};

/**
 * @brief Player or integration actions consumed by gameplay systems.
 */
enum class CommandType {
    SpawnAttack,
    SpawnHeal,
    ResetGame
};

/**
 * @brief Unit archetypes used to drive rendering and collision rules.
 */
enum class UnitKind {
    Normal,
    Shielded,
    Spiked
};

/**
 * @brief Gameplay notifications emitted for rendering, audio, and effects.
 */
enum class EventType {
    TowerDamaged,
    TowerHealed,
    TowerDestroyed,
    UnitDestroyed,
    UnitSpawned,
    RoundEnded
};

} // namespace tw
