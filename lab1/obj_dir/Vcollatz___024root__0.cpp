// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcollatz.h for the primary calling header

#include "Vcollatz__pch.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcollatz___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vcollatz___024root___eval_triggers__act(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_triggers__act\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                    ((IData)(vlSelfRef.clk) 
                                                     & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP__clk__0)))));
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vcollatz___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
}

bool Vcollatz___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___trigger_anySet__act\n"); );
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

void Vcollatz___024root___nba_sequent__TOP__0(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___nba_sequent__TOP__0\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __Vdly__collatz__DOT__busy;
    __Vdly__collatz__DOT__busy = 0;
    // Body
    __Vdly__collatz__DOT__busy = vlSelfRef.collatz__DOT__busy;
    if (vlSelfRef.go) {
        vlSelfRef.dout = vlSelfRef.n;
        if ((1U == vlSelfRef.n)) {
            __Vdly__collatz__DOT__busy = 0U;
            vlSelfRef.done = 1U;
        } else {
            __Vdly__collatz__DOT__busy = 1U;
            vlSelfRef.done = 0U;
        }
    } else if (vlSelfRef.collatz__DOT__busy) {
        vlSelfRef.dout = vlSelfRef.collatz__DOT__next;
        if ((1U == vlSelfRef.collatz__DOT__next)) {
            __Vdly__collatz__DOT__busy = 0U;
            vlSelfRef.done = 1U;
        }
    }
    vlSelfRef.collatz__DOT__busy = __Vdly__collatz__DOT__busy;
    vlSelfRef.collatz__DOT__next = ((1U & vlSelfRef.dout)
                                     ? ((IData)(1U) 
                                        + ((IData)(3U) 
                                           * vlSelfRef.dout))
                                     : VL_SHIFTR_III(32,32,32, vlSelfRef.dout, 1U));
}

void Vcollatz___024root___eval_nba(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_nba\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VnbaTriggered[0U])) {
        Vcollatz___024root___nba_sequent__TOP__0(vlSelf);
    }
}

void Vcollatz___024root___trigger_orInto__act(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___trigger_orInto__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vcollatz___024root___eval_phase__act(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_phase__act\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vcollatz___024root___eval_triggers__act(vlSelf);
    Vcollatz___024root___trigger_orInto__act(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Vcollatz___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vcollatz___024root___eval_phase__nba(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_phase__nba\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vcollatz___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        Vcollatz___024root___eval_nba(vlSelf);
        Vcollatz___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vcollatz___024root___eval(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vcollatz___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("collatz.sv", 1, "", "NBA region did not converge after 100 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00000064U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vcollatz___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("collatz.sv", 1, "", "Active region did not converge after 100 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
        } while (Vcollatz___024root___eval_phase__act(vlSelf));
    } while (Vcollatz___024root___eval_phase__nba(vlSelf));
}

#ifdef VL_DEBUG
void Vcollatz___024root___eval_debug_assertions(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_debug_assertions\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (VL_UNLIKELY(((vlSelfRef.clk & 0xfeU)))) {
        Verilated::overWidthError("clk");
    }
    if (VL_UNLIKELY(((vlSelfRef.go & 0xfeU)))) {
        Verilated::overWidthError("go");
    }
}
#endif  // VL_DEBUG
