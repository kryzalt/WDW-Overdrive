# ⚡ WDW-Overdrive

[![Release](https://img.shields.io/badge/Release-v1.0.0-blue.svg)](https://github.com/your-username/wdw-overdrive/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/Platform-Windows%20(x86)-lightgrey.svg)](#)
[![Target](https://img.shields.io/badge/Game-WDW%20Magical%20Racing%20Tour-red.svg)](#)
[![Donate](https://img.shields.io/badge/Donate-Crypto-f39c12.svg)](#-support--donations)

> **The definitive modernization overhaul and enhancement engine for *Walt Disney World Quest: Magical Racing Tour* (Crystal Dynamics, 2000).**

**WDW-Overdrive** is an all-in-one `dinput.dll` wrapper and engine overhaul that elevates the classic Disney racing title to modern PC standards. From eliminating retro PS1 polygon jitter using PGXP-style subpixel geometry math, to injecting a dynamic multi-tiered **Nemesis AI** system and seamless Xbox / XInput controller support.

---
## ☕ Support & Donations

If **WDW-Overdrive** brought back nostalgic Disney racing memories with rock-solid modern controls and challenging AI, consider supporting ongoing retro game preservation projects:

| Asset | Network | Wallet Address |
| :--- | :--- | :--- |
| **USDT** | TRC-20 | `TBgghhAiMGUbh4Du5JVpzv8zFBv9npugdC` |
| **BTC** | Bitcoin | `bc1q5mvca58yaa2dc49yzhds3n7h28xzg0zjwxss03` |
| **LTC** | Litecoin | `Lhdef5oPpxXFzH1vwxMFhaVZcyacyzJHgF` |
| **ETH** | ERC-20 | `0x58876e3cd3f3a4b3ab4763a68bf9955b6d127c9a` |
| **TON / GRAM** | TON | `UQCwabfDSQXKs0rtzSZXOxknia6RN4t0bOwV5O_P00AIh11h` |

---

## 🌟 Key Features

### 📐 Modern Display & PGXP Subpixel Precision
- **Native Widescreen & Ultrawide:** Automatically adapts the camera aspect ratio and 3D rendering pipeline to any desktop resolution without stretching.
- **HUD & Minimap Correction:** Corrects 2D sprite scaling, HUD elements, and track minimap coordinate projection according to the display aspect ratio.
- **PGXP-Style Vertex Wobble Fix:** Intercepts low-precision PS1 GTE fixed-point perspective division and replaces it with subpixel-accurate geometry caching. Walls, tracks, and karts remain rock-solid without vertex jitter.

### 🤖 Dynamic Difficulty & AI Overhaul
- **Nemesis AI Mode:** Replaces stagnant AI with an adaptive threat system:
  - **Heat Level 1 (Leading > 4s):** Chasing bots actively prioritize offensive items (Acorns, Guided Rockets) aimed directly at Player 1.
  - **Heat Level 2 (Leading > 10s):** Bots cast Frog Spells and aggressive traps to break your lead.
  - **Heat Level 3 (Final Laps):** Bots gain wall collision immunity, perfect trajectory steering, and automatic item deployment.
- **Rubberband Catch-Up Boost:** Eliminates artificial runaway races by dynamically scaling trailing bots' maximum speed relative to distance gap.

### 🎮 Full Modern Controller Support (XInput)
- Native Xbox 360 / One / Series and modern gamepad compatibility without third-party mappers.
- **Analog Steering:** Smooth, linear turning via the Left Thumbstick with configurable deadzones.
- **Analog Triggers:** Right Trigger (RT) accelerates; Left Trigger (LT) brakes/reverses.
- **Complete Menu Navigation:** Full D-pad and analog stick navigation across all game screens and pause menus.

### 🎥 Freecam Mode & In-Game Debug Cheats
- **Detached Free Camera (`F3` / Back + Y):** Move freely across Disney theme park tracks using mouse look and WASD keys.
- **Custom Pause / Cheat Menu (`F1` / Back + Start):**
  - Instant character unlocks (Jiminy Cricket, Ned Shredbetter, XUD 71).
  - Track selection jumper.
  - Live AI Heat Level and missile drop monitoring.
  - Real-time PGXP, Widescreen, and Nemesis AI toggles.

### 🛠️ Modern Windows 10/11 Quality-of-Life Fixes
- **No-CD Built-in:** Bypasses legacy CD-ROM drive checks and automatically resolves cinematic video paths (you have to copy BGMUSIC.PKR into game folder from disk).
- **Direct3D Device & Resolution Override:** Bypasses legacy system requirements check and DirectDraw mode limits.
- **Single-Core Affinity:** Prevents high-speed multi-core race desyncs and audio loops.
- **Focus Auto-Pause:** Automatically pauses races when minimizing or losing window focus.
- **Flyby Auto-Skip:** Skips pre-race camera flybys via Space/Enter or controller button press.

---

## 🚀 Installation

1. Download **`WDW-Overdrive-v1.0.0.zip`** from [Releases](https://github.com/your-username/wdw-overdrive/releases).
2. Extract `dinput.dll` and `wdw_fix.ini` directly into your game installation directory (where `wdwracing.exe` is located).
3. Open `wdw_fix.ini` to tweak resolution or AI settings if desired.
4. Launch the game!

---

## ⚙️ Configuration (`wdw_fix.ini`)

```ini
[Display]
; Set to 0 to automatically detect desktop resolution
Width=0
Height=0

[Cheats]
; Enable in-game cheat/debug menu (F1 / Back + Start)
EnableDebugMenu=1
DebugMenuHotkey=112        ; 112 = F1

; Freecam toggle hotkey (F3 / Back + Y)
FreecamHotkey=114          ; 114 = F3

; AI Behavior modifiers
AICatchUp=1                ; Rubberband boost for distant bots
NemesisAI=1                ; Adaptive dynamic difficulty mode
DebugOverlay=0             ; Engine telemetry overlay
AutoSkipFlyby=0            ; 1 = Skip intro camera flyby automatically
```
---

## 🎮 Controls Summary

| Action | Controller (XInput) | Keyboard / Mouse |
| :--- | :--- | :--- |
| **Accelerate** | Right Trigger (RT) | Up Arrow |
| **Brake / Reverse** | Left Trigger (LT) | Down Arrow |
| **Steer** | Left Stick | Left / Right Arrows |
| **Hop / Jump** | A / RB | Spacebar |
| **Fire Item** | X / B | Ctrl |
| **Look Behind** | Y / LB | Shift |
| **Pause Menu** | Start | Esc |
| **Cheat Menu** | Back + Start | `F1` |
| **Toggle Freecam** | Back + Y *(or L3 + R3)* | `F3` |
| **Freecam Flight** | — | WASD + Mouse |
| **Freecam Up / Down**| — | Space / R (Up), Ctrl / F (Down) |

---

## 🛠️ Building from Source

### Requirements
- Microsoft Visual Studio 2022 (Community, Professional, or Build Tools) with the **C++ Desktop Development** workload installed.
- Windows SDK.
- `dinput.def` definition file located in the root repository folder.

### Automated Build (Recommended)
Simply double-click or run:
```bat
build.bat
```
The script will locate your Visual Studio 2022 environment, statically link the C runtime (`/MT`) so users do not require Visual C++ Redistributable packages, apply `/DEF:dinput.def` export definitions, and output a ready-to-use `dinput.dll`.

### Manual Compilation via Developer Command Prompt
```cmd
cl.exe /O2 /MT /LD /Fe:dinput.dll main.cpp /link /DEF:dinput.def
```

## 📄 License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**. See the [LICENSE](LICENSE) file for details.

*Walt Disney World Quest: Magical Racing Tour is the property of Disney Interactive and Crystal Dynamics. This mod is an independent non-commercial preservation effort.*
