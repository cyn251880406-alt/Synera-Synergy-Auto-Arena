# 游戏素材下载指南

本目录存放 Synera 自走棋游戏的所有外部图像素材。  
代码已内置**程序化绘制回退**——素材缺失时自动使用纯色+文字渲染，不影响功能。

## 目录结构

```
assets/
├── README.md           ← 本文件
├── units/              ← 单位/英雄图标（64×64 PNG）
├── board/              ← 棋盘格子底纹
├── ui/                 ← UI 面板/按钮底图
├── equipment/          ← 装备图标（32×32 PNG）
└── effects/            ← 技能特效帧（可选）
```

## 推荐免费素材源

### 角色图标（units/）

| 来源 | 链接 | 说明 |
|------|------|------|
| **Raven Fantasy Icons**（免费版） | https://clockworkraven.itch.io/raven-fantasy-icons | ~2000 个 RPG 像素图标，含 64×64 尺寸，非商业用途免费 |
| **XYEzawr RPG Character Pack** | https://xyezawr.itch.io/pixel-art-rpg-character-pack-1 | 8 个免费 64×64 角色 |
| **OcO Medieval Fantasy Pack** | https://oco.itch.io/medieval-fantasy-character-pack | 免费骑士角色，含动画帧 |

### 棋盘底纹（board/）

| 来源 | 链接 | 说明 |
|------|------|------|
| **CodeSpree Seamless RPG Tiles** | https://codespree.itch.io/seamless-rpg-tiles | 64×64 无缝地形砖块，CC BY-SA 4.0 |
| **Kenney Board Game Icons** | https://kenney.nl/assets/board-game-icons | 250+ 桌游图标，CC0（完全免费） |

### UI 素材（ui/）

| 来源 | 链接 | 说明 |
|------|------|------|
| **Kenney Pixel UI Pack** | https://kenney.nl/assets/pixel-ui-pack | 750+ UI 元素，CC0 |
| **Kenney UI Pack - Pixel Adventure** | https://kenney.nl/assets/ui-pack-pixel-adventure | 500+ 像素风格 UI，CC0 |

### 装备图标（equipment/）

Raven Fantasy Icons 免费版中包含武器、盔甲、药水等图标，可直接使用。

## 文件命名约定

### 单位图标 — `assets/units/<英文名>.png`

| 中文名 | 文件名 |
|--------|--------|
| 神射手 | `marksman.png` |
| 重装兵 | `heavy_tank.png` |
| 剑术师 | `swordsman.png` |
| 鹰眼 | `eagle_eye.png` |
| 守护者 | `guardian.png` |
| 狂战士 | `berserker.png` |
| 敌射手 | `enemy_archer.png` |
| 敌坦克 | `enemy_tank.png` |
| 敌战士 | `enemy_warrior.png` |

### 棋盘底纹 — `assets/board/`

| 文件名 | 用途 |
|--------|------|
| `light.png` | 浅色格底纹（64×64） |
| `dark.png` | 深色格底纹（64×64） |
| `bench_slot.png` | 备战区槽位底纹（64×64） |

### 装备图标 — `assets/equipment/`

| 文件名 | 对应装备 |
|--------|----------|
| `sword.png` | 铁剑（Sword） |
| `armor.png` | 锁子甲（Armor） |
| `glove.png` | 急速手套（Glove） |
| `crystal.png` | 蓝水晶（Crystal） |

### UI 底图 — `assets/ui/`

| 文件名 | 用途 |
|--------|------|
| `shop_card.png` | 商店卡片背景 |
| `btn_normal.png` | 按钮常态 |
| `btn_hover.png` | 按钮悬停 |
| `panel_bg.png` | 面板背景 |

## 快速开始

1. 从上述链接下载素材包
2. 解压后，将对应的 PNG 文件重命名并按上表放入对应子目录
3. 重新启动游戏，素材将自动加载
4. 若无素材，游戏仍可正常运行（使用程序化渲染回退）

## 版权声明

请遵守各素材源的授权协议。Kenney 素材为 CC0（可自由使用），Raven Fantasy Icons 免费版限非商业用途。本项目为学生课程项目，符合非商业使用条件。
