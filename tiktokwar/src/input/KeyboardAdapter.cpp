#include "input/KeyboardAdapter.hpp"

#include <SFML/Window/Keyboard.hpp>
#include <array>
#include <vector>

namespace tw {

namespace {

using KeyRow = std::array<sf::Keyboard::Key, 4>;

constexpr std::array<KeyRow, 4> kRows = {{
    {sf::Keyboard::Num1, sf::Keyboard::Num2, sf::Keyboard::Num3, sf::Keyboard::Num4},
    {sf::Keyboard::Q,    sf::Keyboard::W,    sf::Keyboard::E,    sf::Keyboard::R},
    {sf::Keyboard::A,    sf::Keyboard::S,    sf::Keyboard::D,    sf::Keyboard::F},
    {sf::Keyboard::Z,    sf::Keyboard::X,    sf::Keyboard::C,    sf::Keyboard::V}
}};

std::vector<int> attackTargetsForSource(std::size_t towerCount, int sourceId) {
    std::vector<int> targets;
    targets.reserve(towerCount > 0 ? towerCount - 1 : 0);

    for (std::size_t i = 0; i < towerCount; ++i) {
        if (static_cast<int>(i) != sourceId) {
            targets.push_back(static_cast<int>(i));
        }
    }

    return targets;
}

} // namespace

std::vector<GameCommand> KeyboardAdapter::handleEvent(const sf::Event& event, std::size_t towerCount) const {
    std::vector<GameCommand> commands;

    if (event.type != sf::Event::KeyPressed) {
        return commands;
    }

    if (towerCount < 2 || towerCount > 4) {
        return commands;
    }

    for (std::size_t source = 0; source < towerCount; ++source) {
        const auto targets = attackTargetsForSource(towerCount, static_cast<int>(source));
        const std::size_t attackSlots = targets.size();
        const std::size_t healSlot = attackSlots;

        for (std::size_t keyIndex = 0; keyIndex < towerCount; ++keyIndex) {
            if (event.key.code != kRows[source][keyIndex]) {
                continue;
            }

            if (keyIndex < attackSlots) {
                const int sourceTowerId = static_cast<int>(source);
                const int targetTowerId = targets[keyIndex];
                commands.push_back({
                    CommandType::SpawnAttack,
                    static_cast<Team>(sourceTowerId),
                    sourceTowerId,
                    targetTowerId,
                    1
                });
                return commands;
            }

            if (keyIndex == healSlot) {
                const int sourceTowerId = static_cast<int>(source);
                commands.push_back({
                    CommandType::SpawnHeal,
                    static_cast<Team>(sourceTowerId),
                    sourceTowerId,
                    sourceTowerId,
                    1
                });
                return commands;
            }
        }
    }

    return commands;
}

} // namespace tw