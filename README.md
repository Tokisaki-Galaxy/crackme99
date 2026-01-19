<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20%2F23-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
  <img src="https://img.shields.io/badge/Security-CrackMe-red?style=for-the-badge&logo=target" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" />
</p>

<p align="center">
  <svg width="500" height="60" viewBox="0 0 500 60" xmlns="http://www.w3.org/2000/svg">
    <text x="50%" y="50%" font-family="monospace" font-size="32" font-weight="bold" fill="#58a6ff" text-anchor="middle" dominant-baseline="middle">Modern C++ CrackMe Series<text>
    <path d="M 50 45 L 450 45" stroke="#30363d" stroke-width="2" />
  </svg>
</p>

[Jump to English Version](#-project-labyrinth-modern-c-crackme-series) | [跳转到中文版](#项目迷宫现代-c-crackme-系列)

# 🛡️ Project Labyrinth: Modern C++ CrackMe Series

Welcome to **Project Labyrinth**, a series of three progressively difficult CrackMes designed to challenge modern reverse engineering tools and mindsets. Unlike traditional CrackMes that rely on simple packing or obfuscation, this series leverages **C++20/23 language primitives** to create "organic" complexity.

## 🚀 The Philosophy
The goal is to move away from "detectable" protectors and move towards **logic-level entanglement**. By using coroutines, template metaprogramming, and exception-driven control flows, the binary becomes a labyrinth where the path to "Success" is mathematically tied to the input.

---

## 📂 Challenge Levels

### 🟢 Alpha: The Abstraction Maze
*   **Difficulty:** ★★★☆☆
*   **Core Tech:** `std::variant`, `std::visit`, C++20 Coroutines.
*   **Description:** A VM-based challenge where the dispatcher is implemented using coroutines. The linear execution flow is shattered into heap-allocated frames, making standard "Step Over" (F10) debugging a nightmare. 
*   **Objective:** Reverse the custom bytecode and bypass the compile-time string encryption.

### 🟡 Beta: The Deceptive Mirage
*   **Difficulty:** ★★★★★★☆
*   **Core Tech:** Exception-based Control Flow, Parallel Watchdog Threads, Chained Dependencies.
*   **Description:** 
    *   **Anti-Debug:** A high-frequency heartbeat monitor detects breakpoints. If a pause is detected, it silently "poisons" the internal registers, leading to a valid-looking but incorrect result.
    *   **Logic:** No `if-else` jumps. All branching is handled via C++ `throw/catch` mechanisms, rendering standard static CFG (Control Flow Graph) analysis useless.
*   **Objective:** Obtain the 4-character key. Remember: *Observation changes the outcome.*

### 🔴 Gamma: The Chaos Entropy
*   **Difficulty:** ★★★★★★★★★☆
*   **Core Tech:** Chaos Engine (Input-seeded PRNG), Polymorphic Instruction Decoding, Safe Sandboxing.
*   **Description:** 
    *   **Code-Data Entanglement:** The instructions do not exist in the binary in a static form. Your input acts as a seed for a PRNG that decrypts the instruction stream on the fly.
    *   **No Crash Execution:** Even with a wrong key, the VM executes "garbage" logic within a safe sandbox. There are no crashes to help you locate the error.
    *   **Result-Oriented:** There is no "success" flag. The final message is XOR-decrypted using the final state of the VM registers.
*   **Objective:** Recover the key. Static and dynamic analysis will fail without a mathematical approach or symbolic execution.

---

## 🛠️ Build Requirements
*   **Compiler:** MSVC (Visual Studio 2022 17.4+), GCC 11+, or Clang 13+.
*   **Standard:** C++20 or C++23.
*   **Configuration:** 
    *   **Optimization:** `/O2` (Critical for template inlining).
    *   **RTTI:** Disabled (`/GR-`) to prevent metadata leakage.
    *   **Symbols:** Stripped (No PDB).

```bash
# Example for MSVC
cl /std:c++20 /O2 /GR- /EHsc Alpha.cpp
```

---

## 🔍 Investigation Guide
1.  **Static Analysis:** Good luck with the `std::visit` template bloat. IDA/Ghidra will show thousands of STL-internal functions.
2.  **Dynamic Analysis:** Watch out for the parallel monitor. If you hit a breakpoint, the internal state becomes "polluted."
3.  **Keygenning:** For Gamma, you'll likely need to write a symbolic execution script (e.g., using Triton or Angr) or a custom emulator.

---

## 📜 Disclaimer
This project is for **educational and research purposes only**. It is intended to demonstrate how modern C++ features can be used (and abused) for software protection. Use of these techniques in unauthorized environments is strictly prohibited.

---

## 🤝 Contributing
Think you can make it harder? Pull requests with even more C++20 tricks are welcome.

**Happy Reversing!** 🗝️

---

# 项目迷宫：现代 C++ CrackMe 系列

欢迎来到 **Project Labyrinth**。这是一个由三个逐渐增加难度的 CrackMe 组成的系列，旨在挑战现代逆向工程工具和思维。与依赖简单加壳或混淆的传统 CrackMe 不同，本系列利用 **C++20/23 标准** 来实现复杂性。

## 🚀 核心理念
目标是摆脱“可检测”的保护壳，转向**逻辑层面的纠缠**。通过使用协程（Coroutines）、模板元编程和异常驱动的控制流，二进制文件变成了一个迷宫，通往“成功”的路径在数学上与输入绑定。

---

## 📂 挑战等级

### 🟢 Alpha: 抽象迷宫
*   **难度：** ★★★☆☆
*   **核心技术：** `std::variant`, `std::visit`, C++20 协程。
*   **描述：** 一个基于 VM 的挑战，其中调度器使用协程实现。线性执行流被粉碎成堆分配的帧，使标准的“单步跳过 (F10)”调试成为噩梦。
*   **目标：** 逆向自定义字节码并绕过编译时字符串加密。

### 🟡 Beta: 虚假海市蜃楼
*   **难度：** ★★★★★★☆
*   **核心技术：** 基于异常的控制流、并行看门狗线程、链式依赖。
*   **描述：**
    *   **反调试：** 高频心跳监测器检测断点。如果检测到暂停，它会秘密地“污染”内部寄存器，导致生成看似有效但实际错误的结果。
    *   **逻辑：** 没有 `if-else` 跳转。所有分支都通过 C++ `throw/catch` 机制处理，使标准的静态 CFG（控制流图）分析失效。
*   **目标：** 获取 4 位字符的密钥。记住：*观察会改变结果。*

### 🔴 Gamma: 混沌熵
*   **难度：** ★★★★★★★★★☆
*   **核心技术：** 混沌引擎（输入种子伪随机数生成器 PRNG）、多态指令解码、安全沙箱。
*   **描述：**
    *   **代码-数据纠缠：** 指令不以静态形式存在于二进制文件中。你的输入充当 PRNG 的种子，动态解密指令流。
    *   **无崩溃执行：** 即使输入错误的密钥，VM 也会在安全沙箱内执行“垃圾”逻辑。没有任何崩溃来帮助你定位错误。
    *   **结果导向：** 没有“成功”标志。最终消息使用 VM 寄存器的最终状态进行 XOR 解密。
*   **目标：** 恢复密钥。如果没有数学方法或符号执行，静态和动态分析都将失败。

---

## 🛠️ 构建要求
*   **编译器：** MSVC (Visual Studio 2022 17.4+), GCC 11+, 或 Clang 13+。
*   **标准：** C++20 或 C++23。
*   **配置：**
    *   **优化：** `/O2`（对模板内联非常重要）。
    *   **RTTI：** 禁用 (`/GR-`) 以防止元数据泄露。
    *   **符号：** 剥离（无 PDB）。

```bash
# MSVC 示例
cl /std:c++20 /O2 /GR- /EHsc Alpha.cpp
```

---

## 🔍 分析指南
1.  **静态分析：** 祝你好运，慢慢处理 `std::visit` 带来的模板膨胀。IDA/Ghidra 将显示数千个 STL 内部函数。
2.  **动态分析：** 注意并行监控器。如果你触发断点，内部状态将被“污染”。
3.  **算法破译：** 对于 Gamma，你可能需要编写符号执行脚本（例如使用 Triton 或 Angr）或自定义仿真器。

---

## 📜 免责声明
本项目仅用于**教育和研究目的**。旨在展示如何使用（及滥用）现代 C++ 特性进行软件保护。严禁在未经授权的环境中使用这些技术。

---

## 🤝 贡献
觉得你可以让它变得更难？欢迎提交包含更多 C++20 技巧的 Pull Request。

**祝逆向愉快！** 🗝️
