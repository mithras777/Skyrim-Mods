#pragma once

#if defined(SKSE_SUPPORT_XBYAK)

#include <xbyak/xbyak.h>

#include "util/LogUtil.h"

namespace stl {

    class Hook {

    public:
        /// <summary>
        /// Registers a short call/jump hook at the specified address.
        /// Automatically detects the branch type and instruction size (5 or 6 bytes),
        /// and stores the original continuation in T::func.
        /// </summary>
        /// <typeparam name="T">Type containing the trampoline (func) and thunk pointer.</typeparam>
        /// <param name="a_target">Address of the target instruction to hook.</param>
        template <class T>
        static void call(const std::uintptr_t a_target, std::string a_name = "")
        {
            const auto branch = detect_branch(a_target);

            m_hookEntries.emplace_back(HookEntry{
                a_name,
                a_target,
                reinterpret_cast<std::uintptr_t*>(&T::func),
                branch.size,
                branch.type,
                false,
                reinterpret_cast<void*>(&T::thunk)
            });
        }

        /// <summary>
        /// Registers a function hook for the specified target function.
        /// Detects whether the function entry already contains a supported branch:
        /// - Relative/indirect branches are chained through the existing hook.
        /// - RIP-relative jumps are resolved and chained through a trampoline.
        /// - Otherwise, the original prologue is copied and execution is redirected to T::thunk.
        /// </summary>
        /// <typeparam name="T">Type containing the trampoline (func) and thunk pointer.</typeparam>
        /// <param name="a_target">Address of the target function.</param>
        template <class T>
        static void function(const std::uintptr_t a_target, std::string a_name = "")
        {
            const auto branch = detect_branch(a_target);

            m_hookEntries.emplace_back(HookEntry{
                a_name,
                a_target,
                reinterpret_cast<std::uintptr_t*>(&T::func),
                decode_prologue(a_target),
                branch.type,
                true,
                reinterpret_cast<void*>(&T::thunk)
            });
        }

        /// <summary>
        /// Registers an inline patch at the specified address.
        /// Executes custom patch code in T::thunk without allocating a trampoline.
        /// </summary>
        /// <typeparam name="T">Type containing the thunk pointer to execute the patch.</typeparam>
        /// <param name="a_target">Address of the instruction block to patch.</param>
        /// <param name="a_patchSize">Size of the code block to patch.</param>
        template <class T>
        static void patch(std::uintptr_t a_target, std::size_t a_patchSize, std::string a_name = "")
        {
            m_hookEntries.emplace_back(HookEntry{
                a_name,
                a_target,
                nullptr,
                a_patchSize,
                BranchType::None,
                false,
                nullptr,
                &T::thunk
            });
        }

        /// <summary>
        /// Replaces the instruction block at the specified address with a direct jump to T::func.
        /// Automatically fills the overwritten region with INT3 and ensures the jump fits.
        /// </summary>
        /// <typeparam name="T">Type containing the trampoline pointer (func).</typeparam>
        /// <param name="a_from">Start address of the instructions to replace.</param>
        /// <param name="a_replaceSize">Number of bytes in the original block to overwrite.</param>
        template <class T>
        static void replace(const std::uintptr_t a_from, const std::size_t a_replaceSize)
        {
            REL::safe_fill(a_from, REL::INT3, a_replaceSize);

            struct Patch : Xbyak::CodeGenerator
            {
                Patch(std::uintptr_t a_dst) : Xbyak::CodeGenerator(64)
                {
                    mov(rax, a_dst);
                    jmp(rax);
                }
            };
            Patch patch{reinterpret_cast<std::uintptr_t>(T::func)};
            patch.ready();

            const auto patchSize = patch.getSize();
            assert(patchSize <= a_replaceSize && "replace: patch too large for overwritten region");

            std::span<const std::uint8_t> patchSpan{ reinterpret_cast<const std::uint8_t*>(patch.getCode()), patchSize };
            REL::safe_write(a_from, patchSpan);
        }

        /// <summary>
        /// Hooks a virtual function by replacing an entry in a vtable.
        /// Stores the original trampoline in T::func and increments the virtual hook count.
        /// </summary>
        /// <typeparam name="F">Class type containing the target vtable.</typeparam>
        /// <typeparam name="T">Type containing the trampoline (func) and thunk pointer.</typeparam>
        /// <param name="a_tableIndex">Index of the vtable to patch.</param>
        /// <param name="a_functionIndex">Index of the virtual function to hook.</param>
        template <class F, class T>
        static void virtual_function(const std::size_t a_tableIndex, const std::size_t a_functionIndex)
        {
            REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[a_tableIndex] };
            T::func = vtbl.write_vfunc(a_functionIndex, T::thunk);
            m_vTableCount++;
        }

        /// <summary>
        /// Returns the number of registered non-virtual hooks.
        /// </summary>
        /// <returns>Number of non-virtual hooks in the hook list.</returns>
        static std::size_t count() noexcept
        {
            return m_hookEntries.size();
        }

        /// <summary>
        /// Returns the number of registered virtual function hooks.
        /// </summary>
        /// <returns>Number of virtual function hooks installed.</returns>
        static std::size_t virtualCount() noexcept
        {
            return m_vTableCount;
        }

        /// <summary>
        /// Installs all registered hooks and patches in the order they were added.
        /// Allocates a single contiguous trampoline block for all hooks that require it.
        /// Automatically handles:
        /// - Relative and indirect call/jump hooks
        /// - Existing relative jump hooks for function chaining
        /// - RIP-relative indirect jumps for function chaining
        /// - Manual prologue patches
        /// - Inline patches using handlers
        /// After installation, all entries are cleared to prevent reinstallation.
        /// </summary>
        static void install()
        {
            auto& trampoline = SKSE::GetTrampoline();
            std::size_t totalSize{ 0 };

            for (auto& entry : m_hookEntries) {

                std::size_t allocBytes = 0;
                // Inline/handler patch allocation
                if (entry.handler) {
                    allocBytes = entry.size;
                }
                else if (!entry.functionOverride) {

                    switch (entry.branchType) {
                        case BranchType::CallRelative:
                        case BranchType::JumpRelative: {
                            allocBytes = TRAMPOLINE_OVERHEAD;
                            break;
                        }
                        case BranchType::CallIndirect:
                        case BranchType::JumpIndirect: {
                            allocBytes = TRAMPOLINE_OVERHEAD - entry.size;
                            break;
                        }
                        case BranchType::JumpIndirectRip: {
                            allocBytes = TRAMPOLINE_OVERHEAD;
                            break;
                        }
                        case BranchType::None:
                        default: {
                            allocBytes = TRAMPOLINE_OVERHEAD;
                            break;
                        }
                    }
                }
                else {
                    // Check whether the function is already hooked
                    const auto alreadyHooked = detect_branch(entry.target);

                    if (entry.size == 0) {
                        entry.functionOverride = false;
                        entry.func = 0;
                    }
                    else if (alreadyHooked.type == BranchType::JumpRelative) {
                        entry.size = alreadyHooked.size;
                        entry.functionOverride = false;
                        entry.branchType = alreadyHooked.type;
                        allocBytes = TRAMPOLINE_OVERHEAD;
                    }
                    else if (alreadyHooked.type == BranchType::JumpIndirectRip) {
                        const auto destination = resolve_jump_indirect_rip(entry.target);
                        if (destination == 0) {
                            entry.functionOverride = false;
                            entry.func = 0;
                            LOG_WARN("Failed to resolve destination: {} <{}>", entry.target, entry.name);
                        }
                        else {
                            entry.size = alreadyHooked.size;
                            entry.functionOverride = false;
                            entry.branchType = BranchType::JumpIndirectRip;
                            allocBytes = TRAMPOLINE_OVERHEAD;
                        }
                    }
                    // Function override: prologue + jmp back + branch
                    else {
                        allocBytes = entry.size + (TRAMPOLINE_OVERHEAD * 2);
                    }
                }

                totalSize += allocBytes;
            }

            if (totalSize > 0) {
                SKSE::AllocTrampoline(totalSize);
            }
            else {
                m_hookEntries.clear();
                return;
            }

            // Install each hook
            for (auto& entry : m_hookEntries) {

                if (entry.handler) {
                    entry.handler(entry.target);
                    continue;
                }

                if (!entry.func) {
                    continue;
                }

                if (!entry.functionOverride) {

                    switch (entry.branchType) {

                        case BranchType::CallRelative:
                        case BranchType::CallIndirect: {
                            if (entry.size == 5) {
                                *entry.func = trampoline.write_call<5>(entry.target, entry.thunk);
                            }
                            else if (entry.size == 6) {
                                *entry.func = trampoline.write_call<6>(entry.target, entry.thunk);
                            }
                            break;
                        }
                        case BranchType::JumpRelative:
                        case BranchType::JumpIndirect: {
                            if (entry.size == 5) {
                                *entry.func = trampoline.write_branch<5>(entry.target, entry.thunk);
                            }
                            else if (entry.size == 6) {
                                *entry.func = trampoline.write_branch<6>(entry.target, entry.thunk);
                            }
                            break;
                        }
                        case BranchType::JumpIndirectRip: {
                            const auto destination = resolve_jump_indirect_rip(entry.target);
                            if (destination == 0) {
                                LOG_ERROR("Failed to resolve destination: {:X} <{}>", entry.target, entry.name);
                                continue;
                            }

                            auto alloc = trampoline.allocate(TRAMPOLINE_OVERHEAD);
                            const auto continuation = reinterpret_cast<std::uintptr_t>(alloc);

                            trampoline.write_branch<6>(continuation, destination);
                            *entry.func = continuation;

                            trampoline.write_branch<5>(entry.target,entry.thunk);
                            REL::safe_write(entry.target + 5, std::uint8_t{ REL::NOP });

                            break;
                        }
                        case BranchType::None:
                        default: {
                            break;
                        }
                    }
                }
                else {
                    struct Patch : Xbyak::CodeGenerator
                    {
                        Patch(const std::uintptr_t a_src, const std::size_t a_copyBytes)
                        {
                            std::size_t offset = 0;
                            auto src = reinterpret_cast<const std::uint8_t*>(a_src);

                            while (offset < a_copyBytes) {
                                auto instrLen = decode_instruction_length(src + offset);

                                for (size_t i = 0; i < instrLen; i++) {
                                    db(src[offset + i]);
                                }

                                offset += instrLen;
                            }
                            jmp(ptr[rip]);
                            dq(a_src + a_copyBytes);
                        }
                    };
                    Patch patch(entry.target, entry.size);
                    patch.ready();

                    auto alloc = trampoline.allocate(patch.getSize());
                    std::memcpy(alloc, patch.getCode(), patch.getSize());
                    *entry.func = reinterpret_cast<std::uintptr_t>(alloc);
                    // Install branch from original to thunk (E9)
                    trampoline.write_branch<5>(entry.target, entry.thunk);
                    // NOP remaining bytes
                    auto nopSize = entry.size - 5;
                    if (nopSize > 0) {
                        REL::safe_fill(entry.target + 5, REL::NOP, entry.size - 5);
                    }
                }
            }
            m_hookEntries.clear();
        }

    private:
        enum class BranchType {
            None,
            CallRelative,
            JumpRelative,
            CallIndirect,
            JumpIndirect,
            JumpIndirectRip
        };

        struct BranchInfo {
            BranchType type{ BranchType::None };
            std::size_t size{ 0 };
        };
        /// <summary>
        /// Describes a single hook or patch operation.
        ///
        /// A HookEntry defines how execution at a target address is intercepted,
        /// whether the original code is preserved via a trampoline, and how
        /// control is redirected (CALL, JMP, or inline patch).
        ///
        /// This struct is consumed by HookUtil and interpreted as a single
        /// atomic hook installation request.
        /// </summary>
        /// <param name="name -">
        /// Name of your hook.
        /// </param>
        /// <param name="target -">
        /// Address of the original function or instruction block to hook or patch.
        /// </param>
        /// <param name="func -">
        /// Pointer to trampoline storage receiving the continuation address
        /// (typically T::func). Null for pure inline patches.
        /// </param>
        /// <param name="size -">
        /// Number of bytes to overwrite or copy.
        /// For function hooks, this is the decoded prologue size.
        /// For manual patches, this must be explicitly provided.
        /// </param>
        /// <param name="branchType -">
        /// Describes the type of call or jump instruction at the target.
        /// BranchType::None is used when the entry is not a branch hook.
        /// </param>
        /// <param name="functionOverride -">
        /// Forces manual prologue copying instead of automatic instruction decoding.
        /// Used for function hooks requiring explicit control or complex chain-hook scenarios.
        /// </param>
        /// <param name="thunk -">
        /// Address of the thunk function that execution is redirected to.
        /// </param>
        /// <param name="handler -">
        /// Optional handler invoked for inline patches or custom code replacement.
        /// Receives the target address as its argument.
        /// </param>
        struct HookEntry
        {
            std::string name{};
            std::uintptr_t target{};
            std::uintptr_t* func{};
            std::size_t size{};
            BranchType branchType{ BranchType::None };
            bool functionOverride{};
            void* thunk{};
            using HandlerFn = void(*)(std::uintptr_t);
            HandlerFn handler{};
        };

        static constexpr std::size_t TRAMPOLINE_OVERHEAD = 14;
        static inline std::vector<HookEntry> m_hookEntries;
        static inline std::size_t m_vTableCount{ 0 };

        struct PrefixState {
            bool rex{ false };
            bool rexW{ false };
            bool opSize{ false };
            bool addrSize{ false };
        };

        static const std::uint8_t* consume_prefix(const std::uint8_t* a_code, PrefixState& a_prefixState) noexcept
        {
            while (true) {
                // REX prefix
                if (*a_code >= 0x40 && *a_code <= 0x4F) {
                    a_prefixState.rex = true;
                    // REX.W
                    if (*a_code & 0x08) {
                        a_prefixState.rexW = true;
                    }
                    ++a_code;
                    continue;
                }
                // Operand-size override
                if (*a_code == 0x66) {
                    a_prefixState.opSize = true;
                    ++a_code;
                    continue;
                }
                // Address-size override
                if (*a_code == 0x67) {
                    a_prefixState.addrSize = true;
                    ++a_code;
                    continue;
                }
                // LOCK / REP / segment overrides
                if (*a_code == 0xF0 || *a_code == 0xF2 || *a_code == 0xF3 ||
                    *a_code == 0x2E || *a_code == 0x36 || *a_code == 0x3E ||
                    *a_code == 0x26 || *a_code == 0x64 || *a_code == 0x65) {
                    ++a_code;
                    continue;
                }
                break;
            }
            return a_code;
        }

        static std::size_t decode_modrm(const std::uint8_t* a_code) noexcept
        {
            std::size_t len = 1; // ModRM byte
            std::uint8_t modrm = *a_code;

            std::uint8_t mod = (modrm >> 6) & 0x3;
            std::uint8_t rm = modrm & 0x7;

            // SIB
            if (rm == 4 && mod != 3) {
                // SIB byte
                ++len;

                std::uint8_t sib = *(a_code + len - 1);
                std::uint8_t base = sib & 0x7;

                if (mod == 0 && base == 5) {
                    len += 4;
                }
            }
            // Displacement
            if (mod == 0 && rm == 5) {
                // RIP-relative in x64
                len += 4;
            }
            else if (mod == 1) {
                len += 1;
            }
            else if (mod == 2) {
                len += 4;
            }
            return len;
        }

        static std::size_t imm_size(std::uint8_t a_opcode, bool a_opSizeOverride) noexcept
        {
            switch (a_opcode) {
                case 0xC2: case 0xCA: return 2;
                case 0xC6: return 1;
                case 0xC7: return a_opSizeOverride ? 2 : 4;
                case 0x68: return 4;
                case 0x6A: return 1;
                case 0xE8: case 0xE9: return 4;
                default:   return 0;
            }
        }

        static std::size_t decode_instruction_length(const std::uint8_t* a_instrStart) noexcept
        {
            const std::uint8_t* cursor = a_instrStart;
            PrefixState ps{};
            cursor = consume_prefix(cursor, ps);

            std::uint8_t opcode = *cursor++;
            std::size_t length = cursor - a_instrStart;

            // Two-byte opcode (0F xx)
            if (opcode == 0x0F) {
                opcode = *cursor++;
                length++;
                
                // Jcc rel32
                if ((opcode & 0xF0) == 0x80) {
                    return length + 4;
                }
                // No ModRM instructions, SYSCALL, RDTSC
                if (opcode == 0x05 || opcode == 0x31) {
                    return length;
                }
                // Assume ModRM
                return length + decode_modrm(cursor);
            }
            // Fast-path single-byte opcodes
            if ((opcode & 0xF8) == 0x50 || // PUSH/POP
                opcode == 0x90 ||          // NOP
                opcode == 0xC3) {          // RET
                return length;
            }
            // Relative CALL/JMP
            if (opcode == 0xE8 || opcode == 0xE9) {
                return length + 4;
            }
            // FF group (CALL/JMP r/m64)
            if (opcode == 0xFF) {
                return length + decode_modrm(cursor);
            }
            // MOV reg, imm
            if ((opcode & 0xF8) == 0xB8) {
                return length + (ps.rexW ? 8 : ps.opSize ? 2 : 4);
            }
            // ALU r/m, imm (80 / 81 / 82 / 83)
            if (opcode == 0x80 || opcode == 0x81 ||
                opcode == 0x82 || opcode == 0x83) {

                length += decode_modrm(cursor);
                // imm8
                if (opcode == 0x80 || opcode == 0x82 || opcode == 0x83) {
                    length += 1;
                }
                // imm16 / imm32
                else {
                    length += ps.opSize ? 2 : 4;
                }
                return length;
            }
            // TEST / NOT / NEG / MUL / IMUL / DIV / IDIV
            if (opcode == 0xF6 || opcode == 0xF7) {
                std::uint8_t modrm = *cursor;
                std::uint8_t reg = (modrm >> 3) & 0x7;

                length += decode_modrm(cursor);
                // TEST r/m, imm
                if (reg == 0) {
                    // imm8
                    if (opcode == 0xF6) {
                        length += 1;
                    }
                    // imm16 / imm32
                    else {
                        length += ps.opSize ? 2 : 4;
                    }
                }
                return length;
            }
            // Instructions using ModRM
            if (opcode == 0x8B || opcode == 0x89 || // MOV
                opcode == 0x8D ||                   // LEA
                opcode == 0x85 || opcode == 0x39 || // TEST/CMP
                opcode == 0x01 || opcode == 0x29    // ADD/SUB
                ) {
                return length + decode_modrm(cursor);
            }
            // Fallback
            return length + imm_size(opcode, ps.opSize);
        }

        /// <summary>
        /// Determines the minimum number of whole instructions required to safely
        /// overwrite at least <paramref name="a_minBytes"/> bytes at the given address.
        /// </summary>
        /// <param name="a_addr">Address of the function entry or patch location.</param>
        /// <param name="a_minBytes">
        /// Minimum number of bytes required (typically 5 for a relative JMP).
        /// </param>
        /// <returns>
        /// The total number of bytes that must be copied/overwritten to safely
        /// place a hook without corrupting instructions.
        /// </returns>
        static std::size_t decode_prologue(const std::uintptr_t a_target, std::size_t a_minBytes = 5)
        {
            constexpr std::size_t MAX_INSTRUCTION_LENGTH = 15;
            constexpr std::size_t MAX_PROLOGUE = 64;

            std::size_t total = 0;
            const auto* instrStart = reinterpret_cast<const std::uint8_t*>(a_target);

            while (total < a_minBytes) {

                if (total >= MAX_PROLOGUE) {
                    LOG_ERROR("Prologue exceeded max length at {:X}", a_target);
                    return 0;
                }

                std::size_t length = decode_instruction_length(instrStart + total);

                if (length == 0 || length > MAX_INSTRUCTION_LENGTH) {
                    LOG_ERROR("Invalid instruction at {:X}", a_target + total);
                    return 0;
                }

                total += length;
            }

            if (total < a_minBytes) {
                LOG_ERROR("Failed to gather enough bytes for hook at {:X}", a_target);
                return 0;
            }
            return total;
        }

        static const std::uint8_t* ignore_prefix(const std::uint8_t* a_target) noexcept
        {
            while (true) {
                // REX prefix (0x40-0x4F)
                if ((*a_target >= 0x40 && *a_target <= 0x4F) ||
                    // LOCK/REP prefixes
                    *a_target == 0xF0 || *a_target == 0xF2 || *a_target == 0xF3 ||
                    // Segment overrides
                    *a_target == 0x2E || *a_target == 0x36 || *a_target == 0x3E ||
                    *a_target == 0x26 || *a_target == 0x64 || *a_target == 0x65) {
                    ++a_target;
                }
                else {
                    break;
                }
            }
            return a_target;
        }

        [[nodiscard]] static std::uintptr_t resolve_jump_indirect_rip(std::uintptr_t a_addr) noexcept
        {
            const auto branch = detect_branch(a_addr);
            if (branch.type != BranchType::JumpIndirectRip) {
                return 0;
            }

            const auto* code = reinterpret_cast<const std::uint8_t*>(a_addr);

            std::int32_t displacement{};
            std::memcpy(&displacement, code + 2, sizeof(displacement));

            const auto ptrAddress = a_addr + branch.size + static_cast<std::intptr_t>(displacement);

            return *reinterpret_cast<const std::uintptr_t*>(ptrAddress);
        }

        /// <summary>
        /// Detects the size (in bytes) of the instruction at the given address, specifically for 
        /// call/jump instructions. Returns 5 or 6 for recognized CALL/JMP opcodes, or 0 if the 
        /// instruction is not a recognized call/jump.
        /// </summary>
        /// <param name="a_target">The address of the instruction to inspect.</param>
        /// <returns>The size of the instruction in bytes, or 0 if unknown.</returns>
        static BranchInfo detect_branch(const std::uintptr_t a_target) noexcept
        {
            const auto* instrStart = reinterpret_cast<const std::uint8_t*>(a_target);

            PrefixState ps{};
            const auto* cursor = consume_prefix(instrStart, ps);
            const auto opcode = *cursor;

            BranchInfo branch{};

            switch (opcode) {
                // CALL rel32
                case 0xE8: {
                    branch.type = BranchType::CallRelative;
                    branch.size = decode_instruction_length(instrStart);
                    break;
                }
                // JMP rel32
                case 0xE9: {
                    branch.type = BranchType::JumpRelative;
                    branch.size = decode_instruction_length(instrStart);
                    break;
                }
                case 0xFF: {
                    const auto modrm = *(cursor + 1);
                    const auto reg = (modrm >> 3) & 0x7;
                    const auto mod = (modrm >> 6) & 0x3;
                    const auto rm = modrm & 0x7;

                    const auto size = decode_instruction_length(instrStart);

                    // FF /2 = CALL r/m64
                    if (reg == 2) {
                        branch.type = BranchType::CallIndirect;
                        branch.size = size;
                    }
                    // FF /4 = JMP r/m64
                    else if (reg == 4) {
                        // FF 25 = JMP qword ptr [RIP + disp32]
                        // mod = 00
                        // rm  = 101
                        if (mod == 0 && rm == 5) {
                            branch.type = BranchType::JumpIndirectRip;
                        }
                        else {
                            branch.type = BranchType::JumpIndirect;
                        }
                        branch.size = size;
                    }
                    break;
                }
                default: {
                    break;
                }
            }

            return branch;
        }
    };
}

#endif
