cmake_minimum_required(VERSION 3.10)
project(TikTokWar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 2.5 REQUIRED COMPONENTS graphics window system audio)

include_directories(include)

add_executable(tiktok_war
    src/main.cpp
    src/core/Game.cpp
    src/model/Tower.cpp
    src/model/Unit.cpp
    src/model/NormalUnit.cpp
    src/model/ShieldedUnit.cpp
    src/model/SpikedUnit.cpp
    src/model/GameState.cpp
    src/model/GameEvent.cpp
    src/systems/SpawnSystem.cpp
    src/systems/MovementSystem.cpp
    src/systems/CollisionSystem.cpp
    src/systems/CombatSystem.cpp
    src/systems/RoundSystem.cpp
    src/render/Renderer.cpp
    src/render/TowerRenderer.cpp
    src/render/UnitRenderer.cpp
    src/render/HudRenderer.cpp
    src/render/EffectsRenderer.cpp
    src/input/KeyboardAdapter.cpp
    src/input/InputManager.cpp
    src/input/TikTokAdapter.cpp
    src/audio/AudioManager.cpp
    src/utils/Math.cpp
    src/utils/IdGenerator.cpp
)

target_link_libraries(tiktok_war
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

if(WIN32)
    target_link_libraries(tiktok_war ws2_32)
endif()

add_custom_command(TARGET tiktok_war POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:tiktok_war>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/gifts.xml"
            "$<TARGET_FILE_DIR:tiktok_war>/gifts.xml"
)
