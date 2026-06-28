# Synera: Synergy Auto-Arena

> 南京大学高级程序设计课程 PA — 单机自走棋游戏 (PVE)

## 1 基本信息

| 项目 | 内容 |
|------|------|
| **姓名** | 曹裕宁 |
| **学号** | 251880406 |
| **项目名称** | Synera: Synergy Auto-Arena |
| **开发环境** | Qt 6.11.0 + MinGW 13.1.0 + CMake 3.30+ |
| **语言标准** | C++17 |

### 各阶段完成度

| 阶段 | 内容 | 状态 |
|------|------|------|
| 阶段一 | 棋盘、备战区、单位、拖拽、GUI | ✅ |
| 阶段二 | 三阶段循环、FSM 状态机、索敌系统、BFS 寻路、技能多态 | ✅ |
| 阶段三 | 商店概率刷新、羁绊系统、升星(3合1)、装备掉落/穿戴、JSON存档 | ✅ |
| 阶段四 | 利息系统、连胜/连败、战斗特效、10回合制、经验升级 | ✅ |

---

## 2 文件树结构

```
gamehomework/
├── CMakeLists.txt                   # Qt6 + C++17 构建配置
├── main.cpp                         # 程序入口，创建窗口与组件
├── README.md                        # 本文档
│
├── core/                            # 控制层
│   ├── Game.h / Game.cpp            #   游戏主控制器（QTimer主循环、三阶段流转）
│   └── Player.h / Player.cpp        #   玩家实体（HP/金币/等级/人口/利息）
│
├── board/                           # 棋盘
│   └── BoardWidget.h / BoardWidget.cpp  # 8×8 QGraphicsView 棋盘 + 占用表
│
├── bench/                           # 备战区
│   ├── Bench.h / Bench.cpp          #   逻辑层 8 槽位管理（std::array<Unit*,8>）
│   └── BenchWidget.h / BenchWidget.cpp # 视图层 + 拖拽高亮
│
├── battle/                          # 战斗系统
│   ├── BattleSystem.h / BattleSystem.cpp       # 战斗引擎（每帧索敌/攻击/移动）
│   ├── TargetingSystem.h / TargetingSystem.cpp # 四级索敌
│   ├── Pathfinder.h / Pathfinder.cpp           # BFS 最短路径 + 单步移动
│   ├── EnemySpawner.h / EnemySpawner.cpp       # 敌方轮次生成 + 属性缩放
│   └── BattleEffectOverlay.h / BattleEffectOverlay.cpp # 弹道/AOE/治疗/死亡特效
│
├── shop/                            # 商店
│   ├── Shop.h / Shop.cpp            #   6 英雄模板池 + 按轮次概率刷新
│   └── ShopWidget.h / ShopWidget.cpp  # 5 招募卡 + 购买/刷新按钮
│
├── synergy/                         # 羁绊与升星
│   ├── SynergySystem.h / SynergySystem.cpp  # 羁绊计数 + 战斗前后属性加成
│   └── StarSystem.h / StarSystem.cpp        # 3合1升星合并（级联合成）
│
├── save/                            # 存档
│   └── SaveLoadManager.h / SaveLoadManager.cpp # JSON 序列化/反序列化
│
├── ui/                              # UI 面板
│   ├── PlayerHud.h / PlayerHud.cpp           # 顶部 HUD 信息条
│   ├── TraitPanel.h / TraitPanel.cpp         # 羁绊弹窗
│   ├── EquipWidget.h / EquipWidget.cpp       # 装备池面板
│   ├── ActionButtonPanel.h / ActionButtonPanel.cpp # 开始战斗/刷新按钮
│   ├── SellZone.h / SellZone.cpp             # 出售区（拖拽出售）
│   ├── ResultOverlay.h / ResultOverlay.cpp   # 胜利/失败结算画面
│   ├── ResourceManager.h / ResourceManager.cpp # 外部素材加载
│   └── BgmPlayer.h / BgmPlayer.cpp           # 背景音乐播放器
│
├── unit/                            # 单位系统
│   ├── Unit.h / Unit.cpp            #   逻辑层单位基类 + FSM 五态转换
│   └── UnitItem.h / UnitItem.cpp    #   视图层 QGraphicsRectItem 渲染 + 拖拽
│
├── skill/                           # 技能系统
│   ├── Skill.h / Skill.cpp          #   抽象基类（纯虚接口 + 目标解析）
│   └── HeroSkills.h / HeroSkills.cpp  # 3 种派生技能
│
├── equipment/                       # 装备系统
│   ├── Equipment.h                  #   抽象基类 + 4 种派生装备
│   └── Equipment.cpp                #   全局 equipItem/removeEquipment 函数
│
└── assets/                          # 外部素材
    ├── units/                       #   角色肖像
    ├── board/                       #   棋盘底纹
    ├── ui/                          #   UI 背景
    ├── equipment/                   #   装备图标
    ├── effects/                     #   技能特效帧
    └── bgm/                         #   背景音乐
```

核心文件夹说明：
- **core/** — 顶层控制。`Game` 是总调度中心，`Player` 管理玩家资源
- **battle/** — 纯逻辑战斗引擎，4 个子系统（战斗/索敌/寻路/生成）各司其职
- **unit/** — 单位系统。`Unit`（逻辑）+ `UnitItem`（视图）分离
- **synergy/** — 羁绊计数 + 升星合并，两个独立策略
- **shop/** — 商店概率刷新 + UI 购买面板

---

## 3 核心类与数据结构

### 3.1 控制层

| 类名 | 主要功能 |
|------|---------|
| `Game` | 主控制器。QTimer 16ms 循环，三阶段（准备/战斗/结算）流转，统一管理单位注册、部署/回收、购买/出售、存档/读档 |
| `Player` | 纯逻辑层实体。HP（100）、金币（初始10）、等级/EXP（每级×4经验）、人口上限（初始4，满级8）、利息（每10金+1，上限5）、连胜/连败追踪 |

### 3.2 战斗与逻辑层

| 类名 | 主要功能 |
|------|---------|
| `Unit` | 单位基类。HP/ATK/Range/Mana + FSM五态（Idle/Moving/Attacking/Casting/Dead），攻击冷却（60帧）+ 普攻回蓝（10/次），组合持有 Skill* 和 Equipment* |
| `BattleSystem` | 每帧为每个存活单位索敌→在射程内攻击→不在射程内BFS寻路→检测死亡。战斗结束条件：一方全灭 |
| `TargetingSystem` | 四级优先级选最优目标：欧氏距离最近→血量最高→从左向右→从下到上 |
| `Pathfinder` | 四方向 BFS 最短路径，搜索到目标攻击范围内即停。所有占据格均不可通行（含目标），MOVE_SPEED=20帧/格 |
| `EnemySpawner` | 6种敌方模板（3职业各基础+精英版），数量=1+round，属性每3轮×1.2缩放 |
| `Bench` | 备战区 8 槽位 std::array，提供存入/移除/交换/查找 |
| `Shop` | 6种英雄模板（费用1×3 + 费用2×3），5招募位，按轮次概率刷新（高轮次高费英雄更常见） |

### 3.3 策略系统

| 类名 | 主要功能 |
|------|---------|
| `SynergySystem` | 羁绊计数。3职业各双阈值（2/3），达标触发 ATK% 或 MaxHP% 加成。战斗前 applyBuffs()→战斗后 clearBuffs() |
| `StarSystem` | 3合1升星。findMerges() 按 name+starLevel 分组检测→executeMerge() 创建高星Unit（HP/ATK×1.8）。级联合成：循环至无更多合并 |

### 3.4 技能系统

| 技能类 | 对应职业 | 目标类型 | 效果 |
|--------|---------|----------|------|
| `PowerStrike` 重击 | 战士 | 单体敌人 | 造成 ATK×2.5 伤害 |
| `IronWall` 铁壁 | 坦克 | 自身 | 恢复 ATK×2.0 HP |
| `ArrowRain` 箭雨 | 射手 | AOE敌人(半径2格) | 范围内敌人各受 ATK×1.5 伤害 |

所有技能 stateless，可跨单位共享。

### 3.5 装备系统

| 装备 | 效果 | 适用场景 |
|------|------|---------|
| 铁剑 Sword | ATK+15 | 物理输出单位 |
| 锁子甲 Armor | MaxHP+200 | 前排坦克 |
| 急速手套 Glove | 攻击间隔×0.8 | 普攻依赖型单位 |
| 蓝水晶 Crystal | MaxMana−30（最低20） | 技能依赖型单位 |

每单位至多1件，胜利后40%概率随机掉落。

### 3.6 商店英雄池

| 英雄 | 费用 | 职业 | HP | ATK | Range | MaxMana |
|------|------|------|-----|-----|-------|---------|
| 神射手 | 1 | 射手 | 220 | 32 | 3 | 60 |
| 重装兵 | 1 | 坦克 | 450 | 18 | 1 | 70 |
| 剑术师 | 1 | 战士 | 320 | 28 | 1 | 55 |
| 鹰眼 | 2 | 射手 | 200 | 42 | 3 | 50 |
| 守护者 | 2 | 坦克 | 550 | 22 | 1 | 75 |
| 狂战士 | 2 | 战士 | 350 | 38 | 1 | 50 |

全部以 1★ 出现，通过 3 合 1 升星最高至 3★。

### 3.7 视图层

| 类名 | 基类 | 职责 |
|------|------|------|
| `BoardWidget` | QGraphicsView | 8×8 棋盘渲染，维护 occupancy[8][8] 占用表，响应键盘事件 |
| `UnitItem` | QGraphicsRectItem | 单位可视化：底色(蓝/红)+名称+血条(绿→红渐变)+蓝条+星级+装备标记。鼠标拖拽全流程 |
| `BenchWidget` | QGraphicsView | 备战区 8 槽位视图 + 拖拽高亮（绿=可放，红=禁放） |
| `ShopWidget` | QWidget | 5 张招募卡片（名称/星级/费用/羁绊/属性预览）+ 购买/刷新按钮 |
| `PlayerHud` | QWidget | 顶部信息条：轮次/阶段/HP条/金币/Lv/人口/装备/[羁绊]按钮 |
| `BattleEffectOverlay` | QGraphicsItem | 战斗特效：攻击弹道（近战白线/远程黄点）、技能AOE扩环、死亡粒子、治疗光环 |
| `EquipWidget` | QWidget | 装备卡片展示 + 点击选中 + 再次点击取消 |
| `SellZone` | QWidget | 拖拽出售区，显示回收价格 |
| `TraitPanel` | QDialog | 羁绊弹窗：显示各职业计数与当前加成状态 |
| `ResultOverlay` | QWidget | 结算画面覆盖层：胜利/失败动画 + 继续按钮 |
| `BgmPlayer` | QObject | 背景音乐播放器：QMediaPlayer + QAudioOutput，无限循环，支持暂停/静音 |

---

## 4 关键算法描述

### 4.1 BFS 寻路（Pathfinder）

**问题**：Unit 需要从当前位置走到攻击范围内的格子，且不能穿过任何单位（敌我双方均不可通行）。

**算法**：
1. 标准 BFS 从起点四方向（上下左右）搜索 8×8 网格
2. 所有被占据的格子（含目标敌人所在格）均不可通行，BFS 在首次到达与目标距离 ≤ attackRange 的格子时停止搜索
3. 用 `prev[row][col]` 记录前驱，BFS 结束后回溯构造正向路径
4. 每 20 帧（MOVE_SPEED）推进一格，通过 Unit 的 m_path 队列 + m_moveTimer 倒计时实现逐步移动
5. `moveStep()` 在目标格被阻塞或进入攻击范围时清空路径回到 Idle，由 BattleSystem 下一帧重新索敌

**碰撞防护**：`Game::update()` 同步棋盘占用表时，若检测到目标格已被其他单位占据，会回退单位位置而非覆盖写入，确保棋盘上不会出现两个单位重叠。

**时间复杂度**：O(8×8) = O(64)，棋盘固定大小所以实际是常数时间。

### 4.2 四级索敌（TargetingSystem）

从候选敌方单位列表中选最优攻击目标，四级优先级：

1. **欧氏距离最小**（主规则）— 用平方值比较避免开根号
2. **血量最高优先**（平局规则1）— 优先集火满血目标而非残血
3. **从左向右**（平局规则2）— 列号小的优先
4. **从下到上**（平局规则3）— 行号大的优先

实现上使用 `std::min_element` + 自定义比较 lambda，单次遍历即可完成。

### 4.3 羁绊计算（SynergySystem）

1. `calculateTraitCounts()` — 遍历已部署单位，统计每个 Trait 标签的出现次数
2. `applyBuffs()` — 战斗前对每个单位查找其所有激活标签的最高阈值层，累加 ATK%/MaxHP% 加成，保存原始值到 m_savedStats
3. `clearBuffs()` — 战斗后从 m_savedStats 恢复所有单位的原始属性

关键设计：多羁绊可叠加（如一个单位同时有 Warrior 和 Tank 标签），同一标签只取最高阈值层。

### 4.4 单位状态机（FSM）

```
Idle ←→ Moving     （寻路分配路径 / 到达目标进入射程）
Idle ←→ Attacking  （发起攻击进入冷却 / 冷却结束）
Attacking → Casting （满蓝触发技能，瞬时）
Casting → Idle     （技能执行完毕，回到空闲）
任意状态 → Dead     （HP ≤ 0，不可逆终态）
```

战后 `reviveAfterBattle()` 绕过 Dead 守卫将单位恢复到 HP=1→Idle，再由 startNewRound 的 heal() 回满血。

---

## 5 辅助函数

项目中主要辅助函数按类别说明：

### 坐标转换（UnitItem 静态方法）

| 函数 | 作用 |
|------|------|
| `boardToPixel(QPoint board)` → QPointF | 棋盘坐标（col,row）转像素坐标（用于定位 UnitItem） |
| `pixelToBoard(QPointF pixel)` → QPoint | 像素坐标转棋盘坐标（拖拽松手时吸附最近格子） |

同类静态转换也存在于 `BenchWidget::slotToPixel()` / `BenchWidget::pixelToSlot()` 和 `BoardWidget::screenToCell()`。

### 装备自由函数（equipment/Equipment.cpp）

| 函数 | 作用 |
|------|------|
| `equipItem(Unit*, Equipment*)` | 为单位穿戴装备（自动卸下旧装 + apply属性 + 注册） |
| `removeEquipment(Unit*)` → Equipment* | 卸下装备（remove属性 + 归还指针给调用方） |

### 存档 Trait 转换（SaveLoadManager 静态方法）

| 函数 | 作用 |
|------|------|
| `traitToString(Trait)` → QString | Trait 枚举 → 字符串（读档用） |
| `stringToTrait(QString)` → Trait | 字符串 → Trait 枚举（存档恢复用） |

### 其他工具函数

| 函数/方法 | 位置 | 作用 |
|-----------|------|------|
| `Game::findUnitItem(Unit*)` | Game.cpp | 平行数组查找：Unit* → UnitItem* |
| `Game::sellPriceMultiplier(int starLevel)` | Game.cpp | 星级→售价倍率（1★×1, 2★×3, 3★×9） |
| `StarSystem::statForStar(baseStat, fromStar, toStar)` | StarSystem.cpp | 计算升星后属性值（baseStat × 1.8^(to-from)） |
| `EnemySpawner::scalingFactor(int round)` | EnemySpawner.cpp | 轮次→属性缩放因子（每3轮×1.2） |
| `Shop::heroCost(name)` / `costToStarLevel(cost)` | Shop.cpp | 英雄名称↔费用查询、费用↔初始星级映射 |

---

## 6 AI 使用说明

本项目的开发过程中使用了 Claude Code 接 deepseek-v4 模型进行辅助。以下详细说明 AI 的使用方式和我的掌控程度。

### 6.1 项目规划

在项目启动阶段，我先独立设计了整体架构：**四层分离**（控制层 Game → 逻辑层纯 C++ → 视图层 Qt Widgets → 场景层 QGraphicsView），然后将此架构作为约束条件提交给 AI。

具体的拆分策略：
1. **先定接口，再写实现**：对于每个新模块（如寻路、技能、升星），我先定义好头文件中的公开接口（输入什么、输出什么、与哪些现有类交互），然后让 AI 辅助生成 .cpp 实现
2. **逻辑层与视图层严格分离**：逻辑层（BattleSystem、Pathfinder、Shop、SynergySystem 等）全部使用 STL/C++ 标准库，不引入 Qt 依赖；视图层（BoardWidget、BenchWidget、ShopWidget 等）专门处理 Qt 渲染和交互
3. **逐阶段叠加**：严格按照 PA 的四个阶段推进。阶段一搭好棋盘+单位+拖拽骨架，阶段二加入战斗 AI（FSM+索敌+寻路），阶段三加入商店/羁绊/升星/装备等策略层，阶段四做视觉打磨（特效/动画/GUI 布局）
4. **每个模块独立文件**：无论模块多小（如 TargetingSystem 只有 ~70 行），都独立 .h/.cpp，确保可以单独理解、单独测试、单独修改

在 AI 辅助过程中，我始终保持对代码的审查和调整。AI 生成的代码经过我阅读确认逻辑正确后才纳入项目。

### 6.2 核心代码解析：BFS 寻路系统

**模块位置**：[battle/Pathfinder.h](battle/Pathfinder.h) / [battle/Pathfinder.cpp](battle/Pathfinder.cpp)

这是由 AI 辅助生成、我逐行审查并理解的核心模块。

**运作原理**：

**数据结构**：Pathfinder 是一个无状态的纯算法类。它不存储任何成员变量，所有计算都通过 const 方法完成。棋盘大小固定为 8×8。

**BFS 搜索** (`findPath`)：
1. 输入：起点坐标 start、目标坐标 target、攻击距离 range、8×8 占用表 occupied
2. 用队列 `std::queue<QPoint>` 做标准 BFS，`visited[8][8]` 标记已访问，`prev[8][8]` 记录前驱
3. 四方向遍历（`DIRS[4][2]` 常量数组）。所有被占据的格子（含目标敌人）均不可通行
4. BFS 在首次到达与 target 距离 ≤ range 的格子时停止搜索，而非走到 target 所在格
5. 通过 prev 回溯构造正向路径（从起点到目标邻格的序列）

**单步移动** (`moveStep`)：
1. 检查路径、递减计时器（MOVE_SPEED=20 帧/格）
2. 弹出下一格 → 检查是否被阻塞 → 检测是否进入攻击范围 → 更新位置

**我做的调整**：原始 BFS 将目标格标记为可通行导致单位能穿过敌人，我修改为所有格子均阻塞、BFS 停在攻击范围内。同时在 `Game::update()` 的棋盘同步中加入了碰撞检测，当两个单位意外到达同一格时回退后者，确保棋盘上不会出现重叠。

### 6.3 核心代码解析：技能系统（策略模式）

**模块位置**：[skill/Skill.h](skill/Skill.h) / [skill/HeroSkills.h](skill/HeroSkills.h) / [skill/HeroSkills.cpp](skill/HeroSkills.cpp)

**运作原理**：

**基类设计**（`Skill`）：
- 纯虚接口：`name()`、`targetType()`、`aoeRadius()`、`cast()`
- `SkillTargetType` 枚举定义 4 种目标选取模式：SingleEnemy（单体敌方）、AOEEnemy（范围敌方）、AOEEnemyAroundSelf（自身周围敌方）、HealFriendly（友方治疗）
- 所有派生类 stateless（无成员变量），同一 Skill 实例可被多个 Unit 共享

**目标解析**（`Unit::castSkill()`）：
1. 读取 `skill->targetType()` 确定目标类型
2. 从战斗上下文 `m_visibleEnemies` / `m_visibleAllies` 中按规则过滤：
   - SingleEnemy → 取当前攻击目标
   - AOEEnemy → 以目标为中心，`aoeRadius()` 范围内的所有敌方单位
   - AOEEnemyAroundSelf → 以自身为中心的范围敌方
   - HealFriendly → 所有存活的友方单位
3. 调用 `skill->cast(caster, targets)` 对每个目标生效

**三种具体技能**：
- `PowerStrike`（重击/战士）：取 targets[0]，对其 `takeDamage(caster->atk() * 2.5)`
- `IronWall`（铁壁/坦克）：对 caster 自身调用 `heal(caster->atk() * 2.0)`
- `ArrowRain`（箭雨/射手）：遍历所有 targets，每个 `takeDamage(caster->atk() * 1.5)`

**触发时机**：Unit 的 `attack()` 方法在普攻回蓝后检测法力是否满 → 满则 `castSkill()` → 清空法力 → FSM: Attacking → Casting → Idle

**我做的调整**：AI 最初将技能设计为每个 Unit 各自持有独立的 Skill 实例（堆分配），我改为 static 栈分配 + 共享指针（stateless 设计），避免了内存管理复杂度。

### 6.4 AI 使用边界

以下工作完全由我独立完成：
- 整体四层架构设计
- 所有头文件的公开接口定义
- 跨控件拖拽（Board↔Bench↔SellZone）的全局坐标转换逻辑
- GUI 布局参数调整（字体大小、间距、配色）
- 战斗特效的时间曲线参数调试（弹道速度、扩环半径、粒子衰减）
- 最终代码审查、清理与重构

AI 辅助的范围：
- 算法细节的实现（BFS 回溯、概率采样、JSON 序列化）
- 重复性代码的生成（getter/setter、paint 绘制分支、UI 布局样板）
- 初始版本的 Doxygen 注释框架（已由我精简）

---

## 7 游戏流程

```
第 1 轮开始
  ├── 准备阶段：拖拽布阵、购买单位、穿戴装备、刷新商店
  ├── 战斗阶段：空格开始 → AI 自动索敌/寻路/攻击/放技能
  ├── 结算阶段：胜负判定 → 扣血/金币/利息/EXP/装备掉落
  └── 下一轮：复活阵亡单位、回满血、回到己方半场
...
第 10 轮通关或 HP 归零结束
```

关键数值：
- 10 回合通关，每 2 回合升 1 级（胜+3EXP/败+2EXP）
- 第1级人口上限4，每级+1（满级5→上限8）
- 利息：每10金+1，上限5
- 敌方：数量=1+round，每3轮属性×1.2
- 背景音乐：启动时自动加载 `assets/bgm/bgm.mp3`，循环播放
- 存档：Ctrl+S 保存 / Ctrl+L 读取（JSON 格式，含 try-catch 异常保护）

---

## 8 操作说明

| 操作 | 方式 |
|------|------|
| 拖拽单位 | 鼠标左键（棋盘↔备战区，准备阶段可用） |
| 开始战斗 | 空格键 或 点击"开始战斗"按钮 |
| 购买单位 | 点击商店卡片 [购买] 按钮 |
| 刷新商店 | 点击 ⟳ 刷新（消耗 2 金币） |
| 穿戴装备 | 装备面板点击选中 → 左键点单位 |
| 卸下装备 | 右键点单位 |
| 出售单位 | 拖拽到右下角出售区 |
| 查看羁绊 | 点击顶部 HUD [羁绊] 按钮 |
| 拖拽升星 | 同名同星单位拖到另一个上方（自动触发 3 合 1） |
| 存档/读档 | 顶部 HUD 按钮（JSON 格式） |

---

## 9 构建与运行

```bash
cd gamehomework/build2
cmake --build .
./SyneraBoard.exe
```

环境要求：Qt 6.11.0 + MinGW 13.1.0 + CMake 3.30+

---

## 参考资料

- Qt 6 官方文档: https://doc.qt.io/qt-6/
- PA 原始需求: [PA说明.md](PA说明.md)
- 游戏玩法参考: Teamfight Tactics (云顶之弈)

---

*本项目为南京大学高级程序设计课程 PA，遵守课程学术诚信规范。*
