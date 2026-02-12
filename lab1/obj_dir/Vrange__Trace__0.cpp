// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals
#include "verilated_vcd_c.h"
#include "Vrange__Syms.h"


void Vrange___024root__trace_chg_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp);

void Vrange___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_chg_0\n"); );
    // Init
    Vrange___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vrange___024root*>(voidSelf);
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;
    // Body
    Vrange___024root__trace_chg_0_sub_0((&vlSymsp->TOP), bufp);
}

void Vrange___024root__trace_chg_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_chg_0_sub_0\n"); );
    // Init
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode + 1);
    // Body
    if (VL_UNLIKELY(vlSelf->__Vm_traceActivity[1U])) {
        bufp->chgBit(oldp+0,(vlSelf->range__DOT__cgo));
        bufp->chgBit(oldp+1,(vlSelf->range__DOT__cdone));
        bufp->chgIData(oldp+2,(vlSelf->range__DOT__n),32);
        bufp->chgCData(oldp+3,(vlSelf->range__DOT__num),4);
        bufp->chgBit(oldp+4,(vlSelf->range__DOT__running));
        bufp->chgBit(oldp+5,(vlSelf->range__DOT__reset_we));
        bufp->chgBit(oldp+6,(vlSelf->range__DOT__reset_done));
        bufp->chgBit(oldp+7,(vlSelf->range__DOT__creset));
        bufp->chgCData(oldp+8,(vlSelf->range__DOT__wr_count),5);
        bufp->chgBit(oldp+9,(vlSelf->range__DOT__we));
        bufp->chgSData(oldp+10,(vlSelf->range__DOT__din),16);
        bufp->chgSData(oldp+11,(vlSelf->range__DOT__mem[0]),16);
        bufp->chgSData(oldp+12,(vlSelf->range__DOT__mem[1]),16);
        bufp->chgSData(oldp+13,(vlSelf->range__DOT__mem[2]),16);
        bufp->chgSData(oldp+14,(vlSelf->range__DOT__mem[3]),16);
        bufp->chgSData(oldp+15,(vlSelf->range__DOT__mem[4]),16);
        bufp->chgSData(oldp+16,(vlSelf->range__DOT__mem[5]),16);
        bufp->chgSData(oldp+17,(vlSelf->range__DOT__mem[6]),16);
        bufp->chgSData(oldp+18,(vlSelf->range__DOT__mem[7]),16);
        bufp->chgSData(oldp+19,(vlSelf->range__DOT__mem[8]),16);
        bufp->chgSData(oldp+20,(vlSelf->range__DOT__mem[9]),16);
        bufp->chgSData(oldp+21,(vlSelf->range__DOT__mem[10]),16);
        bufp->chgSData(oldp+22,(vlSelf->range__DOT__mem[11]),16);
        bufp->chgSData(oldp+23,(vlSelf->range__DOT__mem[12]),16);
        bufp->chgSData(oldp+24,(vlSelf->range__DOT__mem[13]),16);
        bufp->chgSData(oldp+25,(vlSelf->range__DOT__mem[14]),16);
        bufp->chgSData(oldp+26,(vlSelf->range__DOT__mem[15]),16);
        bufp->chgIData(oldp+27,(vlSelf->range__DOT__c1__DOT__dout),32);
        bufp->chgBit(oldp+28,(vlSelf->range__DOT__c1__DOT__busy));
        bufp->chgIData(oldp+29,(vlSelf->range__DOT__c1__DOT__temp),32);
    }
    bufp->chgBit(oldp+30,(vlSelf->clk));
    bufp->chgBit(oldp+31,(vlSelf->go));
    bufp->chgIData(oldp+32,(vlSelf->start),32);
    bufp->chgBit(oldp+33,(vlSelf->done));
    bufp->chgSData(oldp+34,(vlSelf->count),16);
    bufp->chgCData(oldp+35,(vlSelf->range__DOT__addr),4);
}

void Vrange___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_cleanup\n"); );
    // Init
    Vrange___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vrange___024root*>(voidSelf);
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    // Body
    vlSymsp->__Vm_activity = false;
    vlSymsp->TOP.__Vm_traceActivity[0U] = 0U;
    vlSymsp->TOP.__Vm_traceActivity[1U] = 0U;
}
