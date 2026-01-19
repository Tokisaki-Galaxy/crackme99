#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <variant>
#include <coroutine>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <random>
#include <exception>

// ==========================================
// 1. 编译期混淆层
// ==========================================
template <size_t N>
struct XStr {
    std::array<char, N> buffer;
    char k;
    consteval XStr(const char(&str)[N]) : k(0x33) {
        for (size_t i = 0; i < N; ++i) buffer[i] = str[i] ^ k ^ (i % 7);
    }
    std::string s() const {
        std::string res(N, '\0');
        for (size_t i = 0; i < N; ++i) res[i] = buffer[i] ^ k ^ (i % 7);
        return res.data();
    }
};
#define _S(x) XStr<sizeof(x)>(x).s()

// ==========================================
// 2. 反调试守护核心
// ==========================================
namespace Guardian {
    // 这是一个原子心跳包
    std::atomic<int64_t> last_heartbeat = 0;
    // 如果检测到调试，这个掩码会变成非0，彻底破坏运算结果
    std::atomic<uint64_t> corruption_mask = 0;
    // 只有当程序真正退出时才停止监测
    std::atomic<bool> keep_running = true;

    void worker() {
        while (keep_running) {
            auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            auto last = last_heartbeat.load();

            // 检查心跳间隔。如果主线程被断点卡住超过 500ms
            // (注意：这里单位取决于系统tick，通常足够检测断点)
            if (last != 0 && (now - last) > 500000000) {
                // 惩罚：修改掩码，导致后续解密全部错误
                corruption_mask = 0xDEADBEEFCAFEBABE;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    void heartbeat() {
        last_heartbeat = std::chrono::steady_clock::now().time_since_epoch().count();
    }
}

// ==========================================
// 3. 虚拟机异常与指令
// ==========================================

// 自定义异常，用于控制流跳转
struct VmFlowException : std::exception {
    int jump_target;
    VmFlowException(int target) : jump_target(target) {}
};

// 指令集
struct OpLoadByte { int reg; size_t idx; }; // 从输入取字节
struct OpAdd { int r1; int r2; };
struct OpXor { int r1; int r2; };
struct OpRol { int r1; int shift; }; // 循环左移
struct OpAssertEq { int r1; uint64_t val; int fail_jump; }; // 核心：断言失败则抛异常跳转

using Instruction = std::variant<OpLoadByte, OpAdd, OpXor, OpRol, OpAssertEq>;

// ==========================================
// 4. 协程虚拟机
// ==========================================

struct VmTask {
    struct promise_type {
        VmTask get_return_object() { return VmTask{ std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); } // 这里的异常不应该逃逸
        std::suspend_always yield_value(bool) { return {}; }
    };
    std::coroutine_handle<promise_type> h;
    explicit VmTask(std::coroutine_handle<promise_type> h) : h(h) {}
    ~VmTask() { if (h) h.destroy(); }
    void resume() { if (h && !h.done()) h.resume(); }
    bool done() const { return !h || h.done(); }
};

class VirtualMachine {
    std::array<uint64_t, 8> regs = { 0 };
    std::vector<Instruction> code;
    std::string input;
    std::string secret_data;

public:
    VirtualMachine(std::string_view user_input) : input(user_input) {
        secret_data = _S("Access Granted! Welcome to the BETA sector.");

        // 1. 长度检查
        if (input.length() != 4) {
            // 故意用一个永远不会成立的断言来触发 999 错误分支
            code.push_back(OpAssertEq{ 0, 0xDEADBEEFULL, 999 });
        }

        // 2. 验证 'B' -> R0 变为 0x84
        code.push_back(OpLoadByte{ 0, 0 });
        code.push_back(OpAdd{ 0, 0 });
        code.push_back(OpAssertEq{ 0, 0x84ULL, 999 });

        // 3. 验证 'E' -> R1 变为 0xC1
        code.push_back(OpLoadByte{ 1, 1 });
        code.push_back(OpXor{ 1, 0 });
        code.push_back(OpAssertEq{ 1, 0xC1ULL, 999 });

        // 4. 验证 'T' -> R2 变为 0x1150
        code.push_back(OpLoadByte{ 2, 2 });
        code.push_back(OpAdd{ 2, 1 });
        code.push_back(OpRol{ 2, 4 });
        code.push_back(OpAssertEq{ 2, 0x1150ULL, 999 });

        // 5. 验证 '@' -> R3 变为 0x1194
        code.push_back(OpLoadByte{ 3, 3 });
        code.push_back(OpXor{ 3, 2 });
        code.push_back(OpXor{ 3, 0 });
        code.push_back(OpAssertEq{ 3, 0x1194ULL, 999 });

        code.push_back(OpXor{ 0, 3 });                 // R0 = 0x84 ^ 0x1194 = 0x1110
        code.push_back(OpAssertEq{ 0, 0x1110ULL, 999 }); // 验证中间混淆状态
        code.push_back(OpXor{ 0, 3 });                 // R0 = 0x1110 ^ 0x1194 = 0x84 (还原成功!)
    }

    VmTask run(std::string& output_buffer) {
        int pc = 0; // 程序计数器

        while (pc < code.size()) {
            // !!! 喂狗：更新心跳 !!!
            Guardian::heartbeat();

            // 如果 pc 乱飞（比如到了999），说明输入错误
            if (pc >= 999) {
                // 进入错误分支，生成乱码
                // 我们不直接退出，而是用错误的 Key 解密
                regs[0] = 0xDEAD;
                break;
            }

            try {
                // 获取指令
                const auto& inst = code[pc];

                // 执行指令
                std::visit([&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;

                    // 读取反调试掩码。如果被调试，mask 会变成乱七八糟的值
                    uint64_t noise = Guardian::corruption_mask.load();

                    if constexpr (std::is_same_v<T, OpLoadByte>) {
                        if (arg.idx < input.size())
                            regs[arg.reg] = input[arg.idx] ^ noise; // 注入噪音
                        else
                            regs[arg.reg] = 0;
                    }
                    else if constexpr (std::is_same_v<T, OpAdd>) {
                        regs[arg.r1] += regs[arg.r2];
                    }
                    else if constexpr (std::is_same_v<T, OpXor>) {
                        regs[arg.r1] ^= regs[arg.r2];
                    }
                    else if constexpr (std::is_same_v<T, OpRol>) {
                        // 简单的循环左移实现
                        regs[arg.r1] = (regs[arg.r1] << arg.shift) | (regs[arg.r1] >> (64 - arg.shift));
                    }
                    else if constexpr (std::is_same_v<T, OpAssertEq>) {
                        // 核心：利用异常改变流向
                        if (regs[arg.r1] != arg.val) {
                            throw VmFlowException(arg.fail_jump);
                        }
                    }
                    }, inst);

                pc++; // 正常步进
            }
            catch (const VmFlowException& e) {
                // 捕获到逻辑错误，修改 PC
                // 调试器在这里会很头疼，因为 "Next Instruction" 不在下一行
                pc = e.jump_target;
            }

            // 协程挂起，切碎栈帧
            co_yield true;
        }

        // 最终解密阶段
        // 使用寄存器的状态作为 Key 来“还原”输出
        // 如果中间任何一步错了（或者被调试干扰了），regs[0] 的值就不对
        // 输出就会是一堆乱码
        output_buffer = secret_data;
        // 简单的异或解密演示，实际上应该更复杂
        // 假设正确流程结束时 regs[0] 应该是 0x84
        uint64_t final_key = regs[0];

        for (char& c : output_buffer) {
            // 只有 final_key 是 0x84 时，下面的计算才是 (c ^ 0)
            c ^= (static_cast<uint8_t>(final_key & 0xFF) ^ 0x84);
        }
    }
};

int main() {
    // 启动反调试线程
    std::jthread monitor(Guardian::worker);

    std::cout << _S("--- BETA LOCK SYSTEM ---") << std::endl;
    std::cout << _S("Authenticate: ");

    std::string key;
    std::cin >> key;

    std::string result;
    VirtualMachine vm(key);
    auto task = vm.run(result);

    // 驱动虚拟机
    while (!task.done()) {
        task.resume();
        // 极短的休眠，防止 CPU 占用过高，同时给 Monitor 线程调度机会
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    // 停止监控
    Guardian::keep_running = false;

    // 输出结果。
    // 注意：这里没有 if(success)。
    // 只有当 Key 正确时，result 才会打印出人话。
    // 否则就是乱码。
    std::cout << _S("System Response: ") << result << std::endl;

    // 暂停查看结果
    std::cin.ignore();
    std::cin.get();

    return 0;
}
