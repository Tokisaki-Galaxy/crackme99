#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <variant>
#include <coroutine>
#include <functional>
#include <chrono>
#include <random>
#include <ranges>
#include <thread>
#include <atomic>

// ==========================================
// 1. 编译期字符串加密
// 使得静态分析工具无法直接看到字符串
// ==========================================
template <size_t N>
struct XStr {
    std::array<char, N> buffer;
    char key;

    consteval XStr(const char(&str)[N]) : key(0x55) { // 简单密钥，编译期确定
        for (size_t i = 0; i < N; ++i) {
            buffer[i] = str[i] ^ key ^ (i % 3);
        }
    }

    // 运行时解密
    std::string decrypt() const {
        std::string s;
        s.resize(N);
        for (size_t i = 0; i < N; ++i) {
            s[i] = buffer[i] ^ key ^ (i % 3);
        }
        return s.data(); // 去掉结尾的\0
    }
};

// 宏定义方便使用
#define _S(x) XStr<sizeof(x)>(x).decrypt()

// ==========================================
// 2. 虚拟机指令集定义
// 使用 std::variant 造成大量模板代码膨胀
// ==========================================

// 虚拟寄存器状态
struct VmContext {
    std::array<int64_t, 8> regs = { 0 }; // R0-R7
    std::vector<int64_t> stack;
    bool flag_zero = false;
    bool is_trapped = false; // 反调试触发标志

    // 简单的输入缓冲区映射
    std::string user_input;
};

// 指令定义
struct OpLoadImm { int reg_idx; int64_t value; };
struct OpLoadInput { int reg_idx; int input_idx; }; // 从用户输入读取一个字符到寄存器
struct OpAdd { int dest; int src; };
struct OpXor { int dest; int src; };
struct OpMul { int dest; int src; };
struct OpCheck { int reg_idx; int64_t expected; }; // 检查点
struct OpTrap {}; // 隐蔽的陷阱指令

// 所有指令的集合
using Instruction = std::variant<OpLoadImm, OpLoadInput, OpAdd, OpXor, OpMul, OpCheck, OpTrap>;

// ==========================================
// 3. 协程基础设施
// 打破线性调用栈
// ==========================================

struct VmTask {
    struct promise_type {
        VmTask get_return_object() { return VmTask{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        std::suspend_always yield_value(bool) { return {}; }
    };

    std::coroutine_handle<promise_type> h;
    explicit VmTask(std::coroutine_handle<promise_type> h) : h(h) {}
    ~VmTask() { if (h) h.destroy(); }

    void resume() { if (h && !h.done()) h.resume(); }
    bool done() const { return !h || h.done(); }
};

// ==========================================
// 4. 虚拟机核心
// ==========================================

class VirtualMachine {
    VmContext ctx;
    std::vector<Instruction> bytecode;

public:
    VirtualMachine(const std::string& input) {
        ctx.user_input = input;
        init_bytecode();
    }

    // 这里构建逻辑：(Input[0] + 10) ^ 0xDEADBEEF == ...
    // 但我们用一大堆指令来实现它
    void init_bytecode() {
        // 这里的逻辑对应：检查 Input[0] 是否等于 'A' (65)
        // 实际上：((Input[0] * 2) ^ 123) == (65 * 2) ^ 123

        bytecode.push_back(OpLoadInput{ 0, 0 }); // R0 = Input[0]
        bytecode.push_back(OpLoadImm{ 1, 2 });   // R1 = 2
        bytecode.push_back(OpMul{ 0, 1 });       // R0 = R0 * R1
        bytecode.push_back(OpLoadImm{ 2, 123 }); // R2 = 123
        bytecode.push_back(OpXor{ 0, 2 });       // R0 = R0 ^ R2

        // 计算目标值: 'A'(65) * 2 = 130; 130 ^ 123 = 249
        bytecode.push_back(OpCheck{ 0, 249 });
    }

    // 协程运行器
    VmTask run() {
        auto last_time = std::chrono::high_resolution_clock::now();

        for (const auto& inst : bytecode) {

            // --- 反调试：时间检测 ---
            auto now = std::chrono::high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();

            // 如果两条指令之间间隔超过 100ms，说明有人在单步调试
            if (diff > 100) {
                ctx.is_trapped = true;
            }
            last_time = now;
            // ---------------------

            // 利用 std::visit 混淆控制流
            std::visit([this](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                // 如果触发了陷阱，所有计算结果悄悄变异
                int64_t mutation = ctx.is_trapped ? 0x1337 : 0;

                if constexpr (std::is_same_v<T, OpLoadImm>) {
                    ctx.regs[arg.reg_idx] = arg.value + mutation;
                }
                else if constexpr (std::is_same_v<T, OpLoadInput>) {
                    if (arg.input_idx < ctx.user_input.size())
                        ctx.regs[arg.reg_idx] = (unsigned char)ctx.user_input[arg.input_idx];
                    else
                        ctx.regs[arg.reg_idx] = 0;
                }
                else if constexpr (std::is_same_v<T, OpAdd>) {
                    ctx.regs[arg.dest] += ctx.regs[arg.src] + mutation;
                }
                else if constexpr (std::is_same_v<T, OpXor>) {
                    ctx.regs[arg.dest] ^= ctx.regs[arg.src];
                }
                else if constexpr (std::is_same_v<T, OpMul>) {
                    ctx.regs[arg.dest] *= ctx.regs[arg.src];
                }
                else if constexpr (std::is_same_v<T, OpCheck>) {
                    // 这是校验点
                    if (ctx.regs[arg.reg_idx] != arg.expected) {
                        ctx.flag_zero = false;
                    }
                    else {
                        ctx.flag_zero = true;
                    }
                }
                }, inst);

            // 挂起协程，切回主线程
            // 这让堆栈看起来断断续续
            co_yield true;
        }
    }

    bool is_success() const {
        return ctx.flag_zero && !ctx.is_trapped;
    }
};

int main() {
    // 简单的界面
    std::cout << _S("################################") << std::endl;
    std::cout << _S("#   TOP TIER CRACKME v1.0      #") << std::endl;
    std::cout << _S("################################") << std::endl;
    std::cout << _S("Enter Key: ");

    std::string key;
    std::cin >> key;

    if (key.empty()) return 0;

    // 初始化虚拟机
    VirtualMachine vm(key);
    auto task = vm.run();

    // 驱动协程执行
    // 破解者在这里单步调试会非常痛苦，因为不断在 main 和 vm 之间跳跃
    while (!task.done()) {
        task.resume();
        // 这里可以加入一些垃圾代码或者随机延迟来干扰时间检测
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    if (vm.is_success()) {
        std::cout << _S("\n[+] ACCESS GRANTED. Welcome, Master.") << std::endl;
    }
    else {
        std::cout << _S("\n[-] ACCESS DENIED. The system is locked.") << std::endl;
    }

    return 0;
}
