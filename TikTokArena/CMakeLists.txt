cmake_minimum_required(VERSION 3.20)

project(TikTokArena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/raylib)

add_executable(TikTokArena
    src/main.cpp
    src/core/Game.cpp
    src/core/GameConfig.cpp
    src/graphics/WindowManager.cpp
    src/ui/ArenaGrid.cpp
    src/ui/HealthBarRenderer.cpp
    src/ui/WaitingPlayersRenderer.cpp
    src/effects/BackgroundParticles.cpp
    src/physics/MotionIntegrator.cpp
    src/physics/WallCollisionResolver.cpp
    src/physics/CircleCollisionResolver.cpp
    src/physics/PhysicsSystem.cpp
    src/gameplay/WeaponSelection.cpp
    src/entities/Player.cpp
    src/entities/Weapon.cpp
    src/entities/RotatingWeapon.cpp
    src/entities/BasicWeapon.cpp
    src/entities/BatWeapon.cpp
    src/entities/LaserWeapon.cpp
    src/entities/GunWeapon.cpp
    src/entities/PowerUp.cpp
    src/entities/BasicPowerUp.cpp
    src/duel/Duel.cpp
    src/duel/DuelManager.cpp
    src/test/TestPlayerFactory.cpp
)

target_include_directories(TikTokArena PRIVATE
    inc
)

target_link_libraries(TikTokArena PRIVATE raylib)
