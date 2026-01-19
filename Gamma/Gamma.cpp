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
#include <concepts>
#include <span>

// ==========================================
// 1. 编译期混淆
// ==========================================
template <size_t N>
struct XStr {
    std::array<char, N> buffer;
    uint8_t k;
    consteval XStr(const char(&str)[N]) : k(0xAA) {
        for (size_t i = 0; i < N; ++i) buffer[i] = str[i] ^ k ^ (i % 13);
    }
    std::string s() const {
        std::string res(N, '\0');
        for (size_t i = 0; i < N; ++i) res[i] = buffer[i] ^ k ^ (i % 13);
        return res.data();
    }
};
#define _S(x) XStr<sizeof(x)>(x).s()

// ==========================================
// 2. 混沌引擎
// 自定义 PRNG，用于将用户输入转化为指令流
// ==========================================
class ChaosEngine {
    uint64_t state;
public:
    // 将字符串哈希化作为种子
    ChaosEngine(std::string_view seed_str) {
        state = 0xCBF29CE484222325; // FNV offset basis
        for (char c : seed_str) {
            state ^= (uint8_t)c;
            state *= 0x100000001B3; // FNV prime
        }
    }

    // 生成下一个“混乱因子”
    uint8_t next_byte() {
        // Xorshift 变种
        uint64_t x = state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return static_cast<uint8_t>(state & 0xFF);
    }
};

// ==========================================
// 3. 反调试与完整性监视
// ==========================================
namespace Watchdog {
    std::atomic<uint64_t> last_tick{ 0 };
    std::atomic<bool> active{ true };
    // 污染因子：如果被调试，这个值会变成非0，彻底破坏解密结果
    std::atomic<uint8_t> pollution{ 0 };

    void patrol() {
        while (active) {
            auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            auto last = last_tick.load(std::memory_order_relaxed);

            if (last != 0) {
                // 阈值检测：单位纳秒
                // 如果主线程停顿超过 200ms
                if ((now - last) > 200'000'000) {
                    pollution = 0xFF; // 注入毒药
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void feed() {
        last_tick.store(std::chrono::steady_clock::now().time_since_epoch().count(), std::memory_order_relaxed);
    }
}

// ==========================================
// 4. 虚拟机指令定义
// ==========================================
// 指令不再包含操作数，操作数也从数据流中动态读取
struct InstMath { uint8_t opcode_type; }; // 0:Add, 1:Sub, 2:Xor, 3:Mul
struct InstMov {};
struct InstJmp {};
struct InstSys {}; // 系统调用/结束

using Instruction = std::variant<InstMath, InstMov, InstJmp, InstSys>;

// ==========================================
// 5. 动态虚拟机
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

class GammaVM {
    std::array<uint64_t, 16> regs = { 0 };

    // 这里将由 Keygen 生成的数据填充
    std::vector<uint8_t> code_store;
    std::vector<uint8_t> cipher_store;

    ChaosEngine chaos;

public:
    // 构造函数接收 Key，同时也需要外部传入生成好的静态数据
    GammaVM(std::string_view key,
        const std::vector<uint8_t>& code,
        const std::vector<uint8_t>& cipher)
        : chaos(key), code_store(code), cipher_store(cipher)
    {
        // 初始化寄存器
        for (auto& r : regs) r = chaos.next_byte();
    }

    VmTask run(std::string& out_ref) {
        int pc = 0;
        int steps = 0;

        // 必须和 Keygen 一致，运行 256 步
        while (steps < 256) {
            Watchdog::feed();

            // 1. 取指
            // 注意：现在我们用 code_store
            uint8_t raw_byte = code_store[pc % code_store.size()];
            uint8_t decrypt_mask = chaos.next_byte();
            uint8_t poison = Watchdog::pollution.load();

            uint8_t op = raw_byte ^ decrypt_mask ^ poison;

            // 2. 将字节映射为指令 Variant (Polymorphism)
            Instruction inst;
            switch (op % 4) {
            case 0: inst = InstMath{ static_cast<uint8_t>(chaos.next_byte() % 4) }; break;
            case 1: inst = InstMov{}; break;
            case 2: inst = InstJmp{}; break;
            default: inst = InstSys{}; break;
            }

            // 3. 执行 (Execute)
            // 所有的内存访问都取模，保证“乱跑”也不会崩溃 (No Crash)
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                // 获取操作数（同样也是动态解密的）
                uint8_t op1_idx = chaos.next_byte() % 16;
                uint8_t op2_idx = chaos.next_byte() % 16;

                if constexpr (std::is_same_v<T, InstMath>) {
                    switch (arg.opcode_type) {
                    case 0: regs[op1_idx] += regs[op2_idx]; break;
                    case 1: regs[op1_idx] -= regs[op2_idx]; break;
                    case 2: regs[op1_idx] ^= regs[op2_idx]; break;
                    case 3: regs[op1_idx] *= (regs[op2_idx] | 1); break; // 防止乘0清空
                    }
                }
                else if constexpr (std::is_same_v<T, InstMov>) {
                    regs[op1_idx] = regs[op2_idx];
                }
                else if constexpr (std::is_same_v<T, InstJmp>) {
                    // 即使乱跳也是在 encrypted_code 的范围内循环
                    pc += (regs[op1_idx] & 0x1F);
                }
                else if constexpr (std::is_same_v<T, InstSys>) {
                    // 对寄存器进行混淆变换
                    regs[0] = std::rotl(regs[0], 3);
                }
                }, inst);

            pc++;
            steps++;

            // 协程切换：打碎调用栈
            co_yield true;
        }

        // 4. 结果生成
        // 使用 cipher_store 进行解密
        std::string result = "";
        for (size_t i = 0; i < cipher_store.size(); ++i) {
            char k = static_cast<char>(regs[i % 16] & 0xFF);
            result += (char)(cipher_store[i] ^ k);
        }

        out_ref = result;
    }
};

int main() {
    // ============ PASTE KEYGEN OUTPUT HERE ============
    // 示例数据 (必须用 Keygen 生成覆盖这里)
    std::vector<uint8_t> encrypted_code = { /* ... */ };
    std::vector<uint8_t> secret_cipher = { /* ... */ };
    // ==================================================

    std::jthread dog(Watchdog::patrol);

    std::cout << _S("\n=== GAMMA SECURITY LAYER ===\n");
    std::cout << _S("Input Authorization Key: ");

    std::string key;
    std::getline(std::cin, key);

    std::string output;
    // 传入生成的数组
    GammaVM vm(key, encrypted_code, secret_cipher);
    auto task = vm.run(output);

    while (!task.done()) {
        task.resume();
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    Watchdog::active = false;
    std::cout << _S("System Output: [ ") << output << _S(" ]") << std::endl;

    return 0;
}
