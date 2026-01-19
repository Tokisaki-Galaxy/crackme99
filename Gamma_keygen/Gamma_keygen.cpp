#include <iostream>
#include <vector>
#include <array>
#include <iomanip>
#include <bit>
#include "..\Gamma\Common.h"

// 模拟 GammaVM 的行为
int main() {
    std::string key;
    std::cout << "Enter the password you want to use as the VALID KEY: ";
    std::getline(std::cin, key);

    if (key.empty()) key = "1234";

    ChaosEngine chaos(key);

    // 1. 初始化模拟寄存器
    std::array<uint64_t, 16> regs = { 0 };
    for (auto& r : regs) r = chaos.next_byte();

    std::vector<uint8_t> final_code_blob;

    // 2. 模拟运行 256 步，并生成对应的字节码
    // 我们的策略：强制生成 'InstMov' (Type 1) 指令。
    // 因为 MOV 是确定性的，不会产生复杂的数学爆炸，便于我们预测最终状态。
    // InstMov 对应 switch(op % 4) == 1。所以我们需要 op = 1 (或者 5, 9...)

    std::cout << "\n[+] Simulating VM execution and generating bytecode...\n";

    for (int step = 0; step < 256; ++step) {
        // --- 模拟 VM 的取指阶段 ---
        uint8_t decrypt_mask = chaos.next_byte();

        // 我们希望解密出来的 op 是 0x01 (InstMov)
        // 因为: op = raw ^ mask
        // 所以: raw = op ^ mask
        uint8_t target_op = 0x01;
        uint8_t raw_byte = target_op ^ decrypt_mask;

        final_code_blob.push_back(raw_byte);

        // --- 模拟 VM 的操作数读取 ---
        uint8_t op1_idx = chaos.next_byte() % 16;
        uint8_t op2_idx = chaos.next_byte() % 16;

        // --- 模拟 VM 的执行 (InstMov) ---
        // 必须和 CrackMe 里的 InstMov 逻辑一模一样
        regs[op1_idx] = regs[op2_idx];
    }

    // 3. 计算最终的校验密文
    // 我们希望最终解密出这句话：
    std::string plaintext = "Congratulations! The Gamma core is dissolved.";
    std::vector<uint8_t> cipher_blob;

    for (size_t i = 0; i < plaintext.size(); ++i) {
        // Gamma 逻辑： plain = cipher ^ reg
        // 所以： cipher = plain ^ reg
        char k = static_cast<char>(regs[i % 16] & 0xFF);
        cipher_blob.push_back(plaintext[i] ^ k);
    }

    // 4. 输出 C++ 代码块
    std::cout << "\n// ============ COPY BELOW TO CRACKME_GAMMA KEY.H ============\n";

    // 输出 encrypted_code
    std::cout << "std::vector<uint8_t> encrypted_code = {";
    for (size_t i = 0; i < final_code_blob.size(); ++i) {
        if (i % 16 == 0) std::cout << "\n    ";
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)final_code_blob[i] << ", ";
    }
    std::cout << "\n};\n\n";

    // 输出 secret_data (在 CrackMe 里替换那个 XStr 或者直接用 byte array)
    std::cout << "std::vector<uint8_t> secret_cipher = {";
    for (size_t i = 0; i < cipher_blob.size(); ++i) {
        if (i % 16 == 0) std::cout << "\n    ";
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)cipher_blob[i] << ", ";
    }
    std::cout << "\n};\n";
    std::cout << "// =========================================================\n";

    return 0;
}
