// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vrange.h for the primary calling header

#include "Vrange__pch.h"
#include "Vrange___024root.h"

VL_INLINE_OPT void Vrange___024root___ico_sequent__TOP__0(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___ico_sequent__TOP__0\n"); );
    // Body
    vlSelf->range__DOT__addr = (0xfU & ((IData)(vlSelf->range__DOT__we)
                                         ? (IData)(vlSelf->range__DOT__num)
                                         : vlSelf->start));
}

void Vrange___024root___eval_ico(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_ico\n"); );
    // Body
    if ((1ULL & vlSelf->__VicoTriggered.word(0U))) {
        Vrange___024root___ico_sequent__TOP__0(vlSelf);
    }
}

void Vrange___024root___eval_triggers__ico(Vrange___024root* vlSelf);

bool Vrange___024root___eval_phase__ico(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_phase__ico\n"); );
    // Init
    CData/*0:0*/ __VicoExecute;
    // Body
    Vrange___024root___eval_triggers__ico(vlSelf);
    __VicoExecute = vlSelf->__VicoTriggered.any();
    if (__VicoExecute) {
        Vrange___024root___eval_ico(vlSelf);
    }
    return (__VicoExecute);
}

void Vrange___024root___eval_act(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_act\n"); );
}

VL_INLINE_OPT void Vrange___024root___nba_sequent__TOP__0(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___nba_sequent__TOP__0\n"); );
    // Init
    CData/*0:0*/ __Vdly__range__DOT__cgo;
    __Vdly__range__DOT__cgo = 0;
    IData/*31:0*/ __Vdly__range__DOT__n;
    __Vdly__range__DOT__n = 0;
    CData/*3:0*/ __Vdly__range__DOT__num;
    __Vdly__range__DOT__num = 0;
    SData/*15:0*/ __Vdly__range__DOT__din;
    __Vdly__range__DOT__din = 0;
    CData/*4:0*/ __Vdly__range__DOT__wr_count;
    __Vdly__range__DOT__wr_count = 0;
    CData/*3:0*/ __Vdlyvdim0__range__DOT__mem__v0;
    __Vdlyvdim0__range__DOT__mem__v0 = 0;
    SData/*15:0*/ __Vdlyvval__range__DOT__mem__v0;
    __Vdlyvval__range__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__range__DOT__mem__v0;
    __Vdlyvset__range__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdly__range__DOT__c1__DOT__busy;
    __Vdly__range__DOT__c1__DOT__busy = 0;
    CData/*0:0*/ __Vdly__range__DOT__cdone;
    __Vdly__range__DOT__cdone = 0;
    // Body
    __Vdlyvset__range__DOT__mem__v0 = 0U;
    __Vdly__range__DOT__c1__DOT__busy = vlSelf->range__DOT__c1__DOT__busy;
    __Vdly__range__DOT__cdone = vlSelf->range__DOT__cdone;
    __Vdly__range__DOT__din = vlSelf->range__DOT__din;
    __Vdly__range__DOT__n = vlSelf->range__DOT__n;
    __Vdly__range__DOT__cgo = vlSelf->range__DOT__cgo;
    __Vdly__range__DOT__wr_count = vlSelf->range__DOT__wr_count;
    __Vdly__range__DOT__num = vlSelf->range__DOT__num;
    vlSelf->count = vlSelf->range__DOT__mem[vlSelf->range__DOT__addr];
    if (vlSelf->go) {
        vlSelf->range__DOT__reset_done = 0U;
        __Vdly__range__DOT__cgo = 1U;
        __Vdly__range__DOT__n = vlSelf->start;
        __Vdly__range__DOT__num = 0U;
        __Vdly__range__DOT__din = 1U;
        __Vdly__range__DOT__wr_count = 0U;
        if (vlSelf->range__DOT__cgo) {
            __Vdly__range__DOT__cgo = 0U;
        }
        vlSelf->range__DOT__running = 1U;
    } else {
        if ((1U & (~ (IData)(vlSelf->done)))) {
            if (((IData)(vlSelf->range__DOT__running) 
                 & (~ (IData)(vlSelf->range__DOT__cdone)))) {
                __Vdly__range__DOT__din = (0xffffU 
                                           & ((IData)(1U) 
                                              + (IData)(vlSelf->range__DOT__din)));
                __Vdly__range__DOT__cgo = 0U;
                vlSelf->range__DOT__reset_we = 0U;
            } else if (((IData)(vlSelf->range__DOT__running) 
                        & (IData)(vlSelf->range__DOT__cdone))) {
                if ((0xfU != (IData)(vlSelf->range__DOT__wr_count))) {
                    __Vdly__range__DOT__cgo = 1U;
                }
                if (vlSelf->range__DOT__cgo) {
                    __Vdly__range__DOT__cgo = 0U;
                }
                if (vlSelf->range__DOT__creset) {
                    __Vdly__range__DOT__din = 1U;
                    vlSelf->range__DOT__creset = 0U;
                } else {
                    __Vdly__range__DOT__num = (0xfU 
                                               & ((IData)(1U) 
                                                  + (IData)(vlSelf->range__DOT__num)));
                    __Vdly__range__DOT__n = ((IData)(1U) 
                                             + vlSelf->range__DOT__n);
                    __Vdly__range__DOT__wr_count = 
                        (0x1fU & ((IData)(1U) + (IData)(vlSelf->range__DOT__wr_count)));
                    vlSelf->range__DOT__reset_we = 1U;
                    vlSelf->range__DOT__creset = 1U;
                }
            }
        }
        if (vlSelf->done) {
            vlSelf->range__DOT__reset_done = 1U;
            vlSelf->range__DOT__running = 0U;
        }
    }
    if (vlSelf->range__DOT__cgo) {
        vlSelf->range__DOT__c1__DOT__dout = vlSelf->range__DOT__n;
        __Vdly__range__DOT__c1__DOT__busy = 1U;
        __Vdly__range__DOT__cdone = 0U;
    } else if (((IData)(vlSelf->range__DOT__c1__DOT__busy) 
                & (~ (IData)(vlSelf->range__DOT__cdone)))) {
        vlSelf->range__DOT__c1__DOT__dout = vlSelf->range__DOT__c1__DOT__temp;
        if ((1U == vlSelf->range__DOT__c1__DOT__temp)) {
            __Vdly__range__DOT__c1__DOT__busy = 0U;
            __Vdly__range__DOT__cdone = 1U;
        }
    }
    if (vlSelf->range__DOT__we) {
        __Vdlyvval__range__DOT__mem__v0 = vlSelf->range__DOT__din;
        __Vdlyvset__range__DOT__mem__v0 = 1U;
        __Vdlyvdim0__range__DOT__mem__v0 = vlSelf->range__DOT__addr;
    }
    if (__Vdlyvset__range__DOT__mem__v0) {
        vlSelf->range__DOT__mem[__Vdlyvdim0__range__DOT__mem__v0] 
            = __Vdlyvval__range__DOT__mem__v0;
    }
    vlSelf->range__DOT__c1__DOT__busy = __Vdly__range__DOT__c1__DOT__busy;
    vlSelf->range__DOT__cgo = __Vdly__range__DOT__cgo;
    vlSelf->range__DOT__din = __Vdly__range__DOT__din;
    vlSelf->range__DOT__n = __Vdly__range__DOT__n;
    vlSelf->range__DOT__wr_count = __Vdly__range__DOT__wr_count;
    vlSelf->range__DOT__num = __Vdly__range__DOT__num;
    vlSelf->range__DOT__cdone = __Vdly__range__DOT__cdone;
    vlSelf->range__DOT__c1__DOT__temp = ((1U == (1U 
                                                 & vlSelf->range__DOT__c1__DOT__dout))
                                          ? ((IData)(1U) 
                                             + ((IData)(3U) 
                                                * vlSelf->range__DOT__c1__DOT__dout))
                                          : VL_SHIFTR_III(32,32,32, vlSelf->range__DOT__c1__DOT__dout, 1U));
    vlSelf->range__DOT__we = ((~ (IData)(vlSelf->range__DOT__reset_we)) 
                              & (IData)(vlSelf->range__DOT__cdone));
    vlSelf->range__DOT__addr = (0xfU & ((IData)(vlSelf->range__DOT__we)
                                         ? (IData)(vlSelf->range__DOT__num)
                                         : vlSelf->start));
    vlSelf->done = ((~ (IData)(vlSelf->range__DOT__reset_done)) 
                    & (0x10U == (IData)(vlSelf->range__DOT__wr_count)));
}

void Vrange___024root___eval_nba(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_nba\n"); );
    // Body
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        Vrange___024root___nba_sequent__TOP__0(vlSelf);
        vlSelf->__Vm_traceActivity[1U] = 1U;
    }
}

void Vrange___024root___eval_triggers__act(Vrange___024root* vlSelf);

bool Vrange___024root___eval_phase__act(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_phase__act\n"); );
    // Init
    VlTriggerVec<1> __VpreTriggered;
    CData/*0:0*/ __VactExecute;
    // Body
    Vrange___024root___eval_triggers__act(vlSelf);
    __VactExecute = vlSelf->__VactTriggered.any();
    if (__VactExecute) {
        __VpreTriggered.andNot(vlSelf->__VactTriggered, vlSelf->__VnbaTriggered);
        vlSelf->__VnbaTriggered.thisOr(vlSelf->__VactTriggered);
        Vrange___024root___eval_act(vlSelf);
    }
    return (__VactExecute);
}

bool Vrange___024root___eval_phase__nba(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_phase__nba\n"); );
    // Init
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = vlSelf->__VnbaTriggered.any();
    if (__VnbaExecute) {
        Vrange___024root___eval_nba(vlSelf);
        vlSelf->__VnbaTriggered.clear();
    }
    return (__VnbaExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__ico(Vrange___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__nba(Vrange___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vrange___024root___dump_triggers__act(Vrange___024root* vlSelf);
#endif  // VL_DEBUG

void Vrange___024root___eval(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval\n"); );
    // Init
    IData/*31:0*/ __VicoIterCount;
    CData/*0:0*/ __VicoContinue;
    IData/*31:0*/ __VnbaIterCount;
    CData/*0:0*/ __VnbaContinue;
    // Body
    __VicoIterCount = 0U;
    vlSelf->__VicoFirstIteration = 1U;
    __VicoContinue = 1U;
    while (__VicoContinue) {
        if (VL_UNLIKELY((0x64U < __VicoIterCount))) {
#ifdef VL_DEBUG
            Vrange___024root___dump_triggers__ico(vlSelf);
#endif
            VL_FATAL_MT("range.sv", 1, "", "Input combinational region did not converge.");
        }
        __VicoIterCount = ((IData)(1U) + __VicoIterCount);
        __VicoContinue = 0U;
        if (Vrange___024root___eval_phase__ico(vlSelf)) {
            __VicoContinue = 1U;
        }
        vlSelf->__VicoFirstIteration = 0U;
    }
    __VnbaIterCount = 0U;
    __VnbaContinue = 1U;
    while (__VnbaContinue) {
        if (VL_UNLIKELY((0x64U < __VnbaIterCount))) {
#ifdef VL_DEBUG
            Vrange___024root___dump_triggers__nba(vlSelf);
#endif
            VL_FATAL_MT("range.sv", 1, "", "NBA region did not converge.");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        __VnbaContinue = 0U;
        vlSelf->__VactIterCount = 0U;
        vlSelf->__VactContinue = 1U;
        while (vlSelf->__VactContinue) {
            if (VL_UNLIKELY((0x64U < vlSelf->__VactIterCount))) {
#ifdef VL_DEBUG
                Vrange___024root___dump_triggers__act(vlSelf);
#endif
                VL_FATAL_MT("range.sv", 1, "", "Active region did not converge.");
            }
            vlSelf->__VactIterCount = ((IData)(1U) 
                                       + vlSelf->__VactIterCount);
            vlSelf->__VactContinue = 0U;
            if (Vrange___024root___eval_phase__act(vlSelf)) {
                vlSelf->__VactContinue = 1U;
            }
        }
        if (Vrange___024root___eval_phase__nba(vlSelf)) {
            __VnbaContinue = 1U;
        }
    }
}

#ifdef VL_DEBUG
void Vrange___024root___eval_debug_assertions(Vrange___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vrange__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vrange___024root___eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((vlSelf->clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
    if (VL_UNLIKELY((vlSelf->go & 0xfeU))) {
        Verilated::overWidthError("go");}
}
#endif  // VL_DEBUG
