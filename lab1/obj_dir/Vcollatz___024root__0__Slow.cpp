// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vcollatz.h for the primary calling header

#include "Vcollatz__pch.h"

VL_ATTR_COLD void Vcollatz___024root___eval_static(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_static\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
}

VL_ATTR_COLD void Vcollatz___024root___eval_initial__TOP(Vcollatz___024root* vlSelf);

VL_ATTR_COLD void Vcollatz___024root___eval_initial(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_initial\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vcollatz___024root___eval_initial__TOP(vlSelf);
}

VL_ATTR_COLD void Vcollatz___024root___eval_initial__TOP(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_initial__TOP\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.collatz__DOT__busy = 0U;
    vlSelfRef.dout = 0U;
    vlSelfRef.done = 0U;
}

VL_ATTR_COLD void Vcollatz___024root___eval_final(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_final\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcollatz___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vcollatz___024root___eval_phase__stl(Vcollatz___024root* vlSelf);

VL_ATTR_COLD void Vcollatz___024root___eval_settle(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_settle\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Vcollatz___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("collatz.sv", 1, "", "Settle region did not converge after 100 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
    } while (Vcollatz___024root___eval_phase__stl(vlSelf));
}

VL_ATTR_COLD void Vcollatz___024root___eval_triggers__stl(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_triggers__stl\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VstlTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VstlTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VstlFirstIteration)));
    vlSelfRef.__VstlFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vcollatz___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
}

VL_ATTR_COLD bool Vcollatz___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcollatz___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Vcollatz___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Vcollatz___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___trigger_anySet__stl\n"); );
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

VL_ATTR_COLD void Vcollatz___024root___stl_sequent__TOP__0(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___stl_sequent__TOP__0\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.collatz__DOT__next = ((1U & vlSelfRef.dout)
                                     ? ((IData)(1U) 
                                        + ((IData)(3U) 
                                           * vlSelfRef.dout))
                                     : VL_SHIFTR_III(32,32,32, vlSelfRef.dout, 1U));
}

VL_ATTR_COLD void Vcollatz___024root___eval_stl(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_stl\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
        Vcollatz___024root___stl_sequent__TOP__0(vlSelf);
    }
}

VL_ATTR_COLD bool Vcollatz___024root___eval_phase__stl(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___eval_phase__stl\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VstlExecute;
    // Body
    Vcollatz___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = Vcollatz___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        Vcollatz___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

bool Vcollatz___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vcollatz___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vcollatz___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vcollatz___024root___ctor_var_reset(Vcollatz___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vcollatz___024root___ctor_var_reset\n"); );
    Vcollatz__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->clk = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16707436170211756652ull);
    vlSelf->go = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 9942418676787815235ull);
    vlSelf->n = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 1489474827855109852ull);
    vlSelf->dout = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 11474705599699299244ull);
    vlSelf->done = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 10296494685231209730ull);
    vlSelf->collatz__DOT__busy = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 6296662097233115533ull);
    vlSelf->collatz__DOT__next = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 15007738115429209570ull);
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
}
