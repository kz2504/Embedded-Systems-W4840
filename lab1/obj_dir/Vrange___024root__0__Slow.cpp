// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vrange.h for the primary calling header

#include "Vrange__pch.h"

VL_ATTR_COLD void Vrange___024root___eval_static(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_static\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP__clk__0 = vlSelfRef.clk;
}

VL_ATTR_COLD void Vrange___024root___eval_initial__TOP(Vrange___024root* vlSelf);
VL_ATTR_COLD void Vrange___024root____Vm_traceActivitySetAll(Vrange___024root* vlSelf);

VL_ATTR_COLD void Vrange___024root___eval_initial(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_initial\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vrange___024root___eval_initial__TOP(vlSelf);
    Vrange___024root____Vm_traceActivitySetAll(vlSelf);
}

VL_ATTR_COLD void Vrange___024root___eval_initial__TOP(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_initial__TOP\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.range__DOT__cgo = 0U;
    vlSelfRef.range__DOT__n = 0U;
    vlSelfRef.range__DOT__num = 0U;
    vlSelfRef.range__DOT__reset_we = 0U;
    vlSelfRef.range__DOT__write_counter = 0U;
    vlSelfRef.range__DOT__din = 0U;
    vlSelfRef.count = 0U;
    vlSelfRef.range__DOT__state = 0U;
    vlSelfRef.range__DOT__c1__DOT__busy = 0U;
    vlSelfRef.range__DOT__c1__DOT__dout = 0U;
    vlSelfRef.range__DOT__cdone = 0U;
}

VL_ATTR_COLD void Vrange___024root___eval_final(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_final\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vrange___024root___eval_phase__stl(Vrange___024root* vlSelf);

VL_ATTR_COLD void Vrange___024root___eval_settle(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_settle\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Vrange___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("range.sv", 1, "", "Settle region did not converge after 100 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
    } while (Vrange___024root___eval_phase__stl(vlSelf));
}

VL_ATTR_COLD void Vrange___024root___eval_triggers__stl(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_triggers__stl\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VstlTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VstlTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VstlFirstIteration)));
    vlSelfRef.__VstlFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vrange___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
}

VL_ATTR_COLD bool Vrange___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Vrange___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Vrange___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___trigger_anySet__stl\n"); );
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

VL_ATTR_COLD void Vrange___024root___stl_sequent__TOP__0(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___stl_sequent__TOP__0\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.range__DOT__c1__DOT__next = ((1U & vlSelfRef.range__DOT__c1__DOT__dout)
                                            ? ((IData)(1U) 
                                               + ((IData)(3U) 
                                                  * vlSelfRef.range__DOT__c1__DOT__dout))
                                            : VL_SHIFTR_III(32,32,32, vlSelfRef.range__DOT__c1__DOT__dout, 1U));
    vlSelfRef.range__DOT__we = ((~ (IData)(vlSelfRef.range__DOT__reset_we)) 
                                & (IData)(vlSelfRef.range__DOT__cdone));
    vlSelfRef.range__DOT__addr = (0x0000000fU & ((IData)(vlSelfRef.range__DOT__we)
                                                  ? (IData)(vlSelfRef.range__DOT__num)
                                                  : vlSelfRef.start));
}

VL_ATTR_COLD void Vrange___024root___eval_stl(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_stl\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
        Vrange___024root___stl_sequent__TOP__0(vlSelf);
        Vrange___024root____Vm_traceActivitySetAll(vlSelf);
    }
}

VL_ATTR_COLD bool Vrange___024root___eval_phase__stl(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_phase__stl\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VstlExecute;
    // Body
    Vrange___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = Vrange___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        Vrange___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

bool Vrange___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___dump_triggers__ico\n"); );
    // Body
    if ((1U & (~ (IData)(Vrange___024root___trigger_anySet__ico(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'ico' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

bool Vrange___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vrange___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vrange___024root____Vm_traceActivitySetAll(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root____Vm_traceActivitySetAll\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vm_traceActivity[0U] = 1U;
    vlSelfRef.__Vm_traceActivity[1U] = 1U;
}

VL_ATTR_COLD void Vrange___024root___ctor_var_reset(Vrange___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___ctor_var_reset\n"); );
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->clk = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16707436170211756652ull);
    vlSelf->go = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 9942418676787815235ull);
    vlSelf->start = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 9867861323841650631ull);
    vlSelf->done = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 10296494685231209730ull);
    vlSelf->count = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 10730142128712957955ull);
    vlSelf->range__DOT__cgo = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 4379999920339108339ull);
    vlSelf->range__DOT__cdone = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 12581039484530685089ull);
    vlSelf->range__DOT__n = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 2931250089763635780ull);
    vlSelf->range__DOT__num = VL_SCOPED_RAND_RESET_I(4, __VscopeHash, 2799255514935540747ull);
    vlSelf->range__DOT__state = 0;
    vlSelf->range__DOT__reset_we = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 5434001327574429999ull);
    vlSelf->range__DOT__write_counter = VL_SCOPED_RAND_RESET_I(5, __VscopeHash, 9405270263262822162ull);
    vlSelf->range__DOT__we = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 15380956425345825180ull);
    vlSelf->range__DOT__din = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 1814400464142951486ull);
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->range__DOT__mem[__Vi0] = VL_SCOPED_RAND_RESET_I(16, __VscopeHash, 3085873439144697956ull);
    }
    vlSelf->range__DOT__addr = VL_SCOPED_RAND_RESET_I(4, __VscopeHash, 16735560361318500677ull);
    vlSelf->range__DOT__c1__DOT__dout = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 5685290967489692436ull);
    vlSelf->range__DOT__c1__DOT__busy = VL_SCOPED_RAND_RESET_I(1, __VscopeHash, 16051778922446211463ull);
    vlSelf->range__DOT__c1__DOT__next = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 9195544915223658005ull);
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VicoTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP__clk__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->__Vm_traceActivity[__Vi0] = 0;
    }
}
