// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vhex7seg.h for the primary calling header

#include "Vhex7seg__pch.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vhex7seg___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vhex7seg___024root___eval_triggers__ico(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___eval_triggers__ico\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VicoTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VicoTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VicoFirstIteration)));
    vlSelfRef.__VicoFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vhex7seg___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
    }
#endif
}

bool Vhex7seg___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___trigger_anySet__ico\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

extern const VlUnpacked<CData/*6:0*/, 16> Vhex7seg__ConstPool__TABLE_hc4cbd47f_0;

void Vhex7seg___024root___ico_sequent__TOP__0(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___ico_sequent__TOP__0\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*3:0*/ __Vtableidx1;
    __Vtableidx1 = 0;
    // Body
    __Vtableidx1 = vlSelfRef.a;
    vlSelfRef.y = Vhex7seg__ConstPool__TABLE_hc4cbd47f_0
        [__Vtableidx1];
}

void Vhex7seg___024root___eval_ico(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___eval_ico\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*3:0*/ __Vinline__ico_sequent__TOP__0___Vtableidx1;
    __Vinline__ico_sequent__TOP__0___Vtableidx1 = 0;
    // Body
    if ((1ULL & vlSelfRef.__VicoTriggered[0U])) {
        __Vinline__ico_sequent__TOP__0___Vtableidx1 
            = vlSelfRef.a;
        vlSelfRef.y = Vhex7seg__ConstPool__TABLE_hc4cbd47f_0
            [__Vinline__ico_sequent__TOP__0___Vtableidx1];
    }
}

bool Vhex7seg___024root___eval_phase__ico(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___eval_phase__ico\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VicoExecute;
    // Body
    Vhex7seg___024root___eval_triggers__ico(vlSelf);
    __VicoExecute = Vhex7seg___024root___trigger_anySet__ico(vlSelfRef.__VicoTriggered);
    if (__VicoExecute) {
        Vhex7seg___024root___eval_ico(vlSelf);
    }
    return (__VicoExecute);
}

void Vhex7seg___024root___eval(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___eval\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VicoIterCount;
    // Body
    __VicoIterCount = 0U;
    vlSelfRef.__VicoFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VicoIterCount)))) {
#ifdef VL_DEBUG
            Vhex7seg___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
#endif
            VL_FATAL_MT("hex7seg.sv", 1, "", "Input combinational region did not converge after 100 tries");
        }
        __VicoIterCount = ((IData)(1U) + __VicoIterCount);
    } while (Vhex7seg___024root___eval_phase__ico(vlSelf));
}

#ifdef VL_DEBUG
void Vhex7seg___024root___eval_debug_assertions(Vhex7seg___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vhex7seg___024root___eval_debug_assertions\n"); );
    Vhex7seg__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.a & 0xf0U)))) {
        Verilated::overWidthError("a");
    }
}
#endif  // VL_DEBUG
