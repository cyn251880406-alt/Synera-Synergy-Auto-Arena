cmake_minimum_required(VERSION 3.16)
project(SyneraBoard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

add_executable(SyneraBoard
    main.cpp
    core/Game.cpp
    core/Player.cpp
    board/BoardWidget.cpp
    bench/Bench.cpp
    bench/BenchWidget.cpp
    battle/BattleSystem.cpp
    battle/TargetingSystem.cpp
    battle/Pathfinder.cpp
    battle/EnemySpawner.cpp
    battle/BattleEffectOverlay.cpp
    shop/Shop.cpp
    shop/ShopWidget.cpp
    synergy/SynergySystem.cpp
    synergy/StarSystem.cpp
    save/SaveLoadManager.cpp
    ui/PlayerHud.cpp
    ui/EquipWidget.cpp
    ui/ResourceManager.cpp
    ui/ActionButtonPanel.cpp
    ui/SellZone.cpp
    ui/ResultOverlay.cpp
    ui/TraitPanel.cpp
    ui/BgmPlayer.cpp
    unit/Unit.cpp
    unit/UnitItem.cpp
    equipment/Equipment.cpp
    skill/Skill.cpp
    skill/HeroSkills.cpp
)

target_include_directories(SyneraBoard PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/board
    ${CMAKE_SOURCE_DIR}/bench
    ${CMAKE_SOURCE_DIR}/battle
    ${CMAKE_SOURCE_DIR}/shop
    ${CMAKE_SOURCE_DIR}/synergy
    ${CMAKE_SOURCE_DIR}/save
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_SOURCE_DIR}/unit
    ${CMAKE_SOURCE_DIR}/skill
    ${CMAKE_SOURCE_DIR}/equipment
)
target_link_libraries(SyneraBoard PRIVATE Qt6::Widgets Qt6::Multimedia)
