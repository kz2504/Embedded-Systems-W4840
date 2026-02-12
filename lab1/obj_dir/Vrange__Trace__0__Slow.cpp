// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals

#include "verilated_vcd_c.h"
#include "Vrange__Syms.h"


VL_ATTR_COLD void Vrange___024root__trace_init_sub__TOP__0(Vrange___024root* vlSelf, VerilatedVcd* tracep) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_init_sub__TOP__0\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const int c = vlSymsp->__Vm_baseCode;
    tracep->pushPrefix("$rootio", VerilatedTracePrefixType::SCOPE_MODULE);
    tracep->declBit(c+31,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+32,0,"go",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+33,0,"start",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+34,0,"done",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+35,0,"count",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 15,0);
    tracep->popPrefix();
    tracep->pushPrefix("range", VerilatedTracePrefixType::SCOPE_MODULE);
    tracep->declBus(c+37,0,"RAM_WORDS",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::PARAMETER, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+38,0,"RAM_ADDR_BITS",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::PARAMETER, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+31,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+32,0,"go",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+33,0,"start",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+34,0,"done",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+35,0,"count",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 15,0);
    tracep->declBit(c+1,0,"cgo",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+2,0,"cdone",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+3,0,"n",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+4,0,"num",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->declBit(c+5,0,"running",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+6,0,"reset_we",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+7,0,"reset_done",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+8,0,"creset",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+9,0,"wr_count",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 4,0);
    tracep->declBit(c+10,0,"we",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+11,0,"din",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 15,0);
    tracep->pushPrefix("mem", VerilatedTracePrefixType::ARRAY_UNPACKED);
    for (int i = 0; i < 16; ++i) {
        tracep->declBus(c+12+i*1,0,"",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, true,(i+0), 15,0);
    }
    tracep->popPrefix();
    tracep->declBus(c+36,0,"addr",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 3,0);
    tracep->pushPrefix("c1", VerilatedTracePrefixType::SCOPE_MODULE);
    tracep->declBit(c+31,0,"clk",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+1,0,"go",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+3,0,"n",-1, VerilatedTraceSigDirection::INPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBus(c+28,0,"dout",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->declBit(c+2,0,"done",-1, VerilatedTraceSigDirection::OUTPUT, VerilatedTraceSigKind::WIRE, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBit(c+29,0,"busy",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1);
    tracep->declBus(c+30,0,"temp",-1, VerilatedTraceSigDirection::NONE, VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false,-1, 31,0);
    tracep->popPrefix();
    tracep->popPrefix();
}

VL_ATTR_COLD void Vrange___024root__trace_init_top(Vrange___024root* vlSelf, VerilatedVcd* tracep) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_init_top\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vrange___024root__trace_init_sub__TOP__0(vlSelf, tracep);
}

VL_ATTR_COLD void Vrange___024root__trace_const_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
VL_ATTR_COLD void Vrange___024root__trace_full_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
void Vrange___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp);
void Vrange___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/);

VL_ATTR_COLD void Vrange___024root__trace_register(Vrange___024root* vlSelf, VerilatedVcd* tracep) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_register\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    tracep->addConstCb(&Vrange___024root__trace_const_0, 0, vlSelf);
    tracep->addFullCb(&Vrange___024root__trace_full_0, 0, vlSelf);
    tracep->addChgCb(&Vrange___024root__trace_chg_0, 0, vlSelf);
    tracep->addCleanupCb(&Vrange___024root__trace_cleanup, vlSelf);
}

VL_ATTR_COLD void Vrange___024root__trace_const_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp);

VL_ATTR_COLD void Vrange___024root__trace_const_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_const_0\n"); );
    // Body
    Vrange___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vrange___024root*>(voidSelf);
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    Vrange___024root__trace_const_0_sub_0((&vlSymsp->TOP), bufp);
}

VL_ATTR_COLD void Vrange___024root__trace_const_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_const_0_sub_0\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode);
    bufp->fullIData(oldp+37,(0x00000010U),32);
    bufp->fullIData(oldp+38,(4U),32);
}

VL_ATTR_COLD void Vrange___024root__trace_full_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp);

VL_ATTR_COLD void Vrange___024root__trace_full_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_full_0\n"); );
    // Body
    Vrange___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vrange___024root*>(voidSelf);
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    Vrange___024root__trace_full_0_sub_0((&vlSymsp->TOP), bufp);
}

VL_ATTR_COLD void Vrange___024root__trace_full_0_sub_0(Vrange___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root__trace_full_0_sub_0\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode);
    bufp->fullBit(oldp+1,(vlSelfRef.range__DOT__cgo));
    bufp->fullBit(oldp+2,(vlSelfRef.range__DOT__cdone));
    bufp->fullIData(oldp+3,(vlSelfRef.range__DOT__n),32);
    bufp->fullCData(oldp+4,(vlSelfRef.range__DOT__num),4);
    bufp->fullBit(oldp+5,(vlSelfRef.range__DOT__running));
    bufp->fullBit(oldp+6,(vlSelfRef.range__DOT__reset_we));
    bufp->fullBit(oldp+7,(vlSelfRef.range__DOT__reset_done));
    bufp->fullBit(oldp+8,(vlSelfRef.range__DOT__creset));
    bufp->fullCData(oldp+9,(vlSelfRef.range__DOT__wr_count),5);
    bufp->fullBit(oldp+10,(vlSelfRef.range__DOT__we));
    bufp->fullSData(oldp+11,(vlSelfRef.range__DOT__din),16);
    bufp->fullSData(oldp+12,(vlSelfRef.range__DOT__mem[0]),16);
    bufp->fullSData(oldp+13,(vlSelfRef.range__DOT__mem[1]),16);
    bufp->fullSData(oldp+14,(vlSelfRef.range__DOT__mem[2]),16);
    bufp->fullSData(oldp+15,(vlSelfRef.range__DOT__mem[3]),16);
    bufp->fullSData(oldp+16,(vlSelfRef.range__DOT__mem[4]),16);
    bufp->fullSData(oldp+17,(vlSelfRef.range__DOT__mem[5]),16);
    bufp->fullSData(oldp+18,(vlSelfRef.range__DOT__mem[6]),16);
    bufp->fullSData(oldp+19,(vlSelfRef.range__DOT__mem[7]),16);
    bufp->fullSData(oldp+20,(vlSelfRef.range__DOT__mem[8]),16);
    bufp->fullSData(oldp+21,(vlSelfRef.range__DOT__mem[9]),16);
    bufp->fullSData(oldp+22,(vlSelfRef.range__DOT__mem[10]),16);
    bufp->fullSData(oldp+23,(vlSelfRef.range__DOT__mem[11]),16);
    bufp->fullSData(oldp+24,(vlSelfRef.range__DOT__mem[12]),16);
    bufp->fullSData(oldp+25,(vlSelfRef.range__DOT__mem[13]),16);
    bufp->fullSData(oldp+26,(vlSelfRef.range__DOT__mem[14]),16);
    bufp->fullSData(oldp+27,(vlSelfRef.range__DOT__mem[15]),16);
    bufp->fullIData(oldp+28,(vlSelfRef.range__DOT__c1__DOT__dout),32);
    bufp->fullBit(oldp+29,(vlSelfRef.range__DOT__c1__DOT__busy));
    bufp->fullIData(oldp+30,(vlSelfRef.range__DOT__c1__DOT__temp),32);
    bufp->fullBit(oldp+31,(vlSelfRef.clk));
    bufp->fullBit(oldp+32,(vlSelfRef.go));
    bufp->fullIData(oldp+33,(vlSelfRef.start),32);
    bufp->fullBit(oldp+34,(vlSelfRef.done));
    bufp->fullSData(oldp+35,(vlSelfRef.count),16);
    bufp->fullCData(oldp+36,(vlSelfRef.range__DOT__addr),4);
}
