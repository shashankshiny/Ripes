#pragma once

#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_memory.h"
#include "VSRTL/core/vsrtl_wire.h"

#include "riscv.h"

namespace vsrtl {
namespace core {
using namespace Ripes;

template <bool readBypass>
class RegisterFile : public Component {
public:
    SetGraphicsType(ClockedComponent);
    RegisterFile(std::string name, SimComponent* parent) : Component(name, parent) {
        // Writes

        // Disable writes to register 0
        wr_en_0->setSensitiveTo(wr_en);
        wr_en_0->out << [=] { return wr_en.uValue() && wr_addr.uValue() != 0; };

        wr_addr >> _wr_mem->addr;
        wr_en_0->out >> _wr_mem->wr_en;
        data_in >> _wr_mem->data_in;
        4 >> _wr_mem->wr_width;

        // Reads
        r1_addr >> _rd1_mem->addr;
        r2_addr >> _rd2_mem->addr;

        /** If read bypassing is enabled, we may read the next-state register value in the current state.
         * Also note that, given that the RegisterFile is of type Component, all inputs must be propagated before
         * outputs are propagated. Thus, we are sure to have received the next-state write address when we clock the
         * output ports. This would >not< be the case if the RegisterFile was a clocked component.
         */
        if constexpr (readBypass) {
            r1_out << [=] {
                const int rd_idx = r1_addr.uValue();
                if (rd_idx == 0)
                    return static_cast<unsigned>(0);

                const unsigned wr_idx = wr_addr.uValue();
                if (wr_en.uValue() && wr_idx == r1_addr.uValue()) {
                    return data_in.uValue();
                } else {
                    return _rd1_mem->data_out.uValue();
                }
            };

            r2_out << [=] {
                const unsigned rd_idx = r2_addr.uValue();
                if (rd_idx == 0)
                    return static_cast<unsigned>(0);

                const unsigned wr_idx = wr_addr.uValue();
                if (wr_en.uValue() && wr_idx == r2_addr.uValue()) {
                    return data_in.uValue();
                } else {
                    return _rd2_mem->data_out.uValue();
                }
            };
        } else {
            _rd1_mem->data_out >> r1_out;
            _rd2_mem->data_out >> r2_out;
        }
    }

    SUBCOMPONENT(_wr_mem, TYPE(WrMemory<RV_REGS_BITS, RV_REG_WIDTH, false>));
    SUBCOMPONENT(_rd1_mem, TYPE(RdMemory<RV_REGS_BITS, RV_REG_WIDTH, false>));
    SUBCOMPONENT(_rd2_mem, TYPE(RdMemory<RV_REGS_BITS, RV_REG_WIDTH, false>));

    INPUTPORT(r1_addr, RV_REGS_BITS);
    INPUTPORT(r2_addr, RV_REGS_BITS);
    INPUTPORT(wr_addr, RV_REGS_BITS);

    INPUTPORT(data_in, RV_REG_WIDTH);
    WIRE(wr_en_0, 1);
    INPUTPORT(wr_en, 1);
    OUTPUTPORT(r1_out, RV_REG_WIDTH);
    OUTPUTPORT(r2_out, RV_REG_WIDTH);

    VSRTL_VT_U getRegister(unsigned i) { return m_memory->readMem<false>(i); }

    std::vector<VSRTL_VT_U> getRegisters() {
        std::vector<VSRTL_VT_U> regs;
        for (int i = 0; i < RV_REGS; i++)
            regs.push_back(getRegister(i));
        return regs;
    }

    void setMemory(SparseArray* mem) {
        m_memory = mem;
        // All memory components must point to the same memory
        _wr_mem->setMemory(m_memory);
        _rd1_mem->setMemory(m_memory);
        _rd2_mem->setMemory(m_memory);
    }

private:
    SparseArray* m_memory = nullptr;
};

}  // namespace core
}  // namespace vsrtl
