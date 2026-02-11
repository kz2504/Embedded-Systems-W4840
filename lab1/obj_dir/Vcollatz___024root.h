// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vcollatz.h for the primary calling header

#ifndef VERILATED_VCOLLATZ___024ROOT_H_
#define VERILATED_VCOLLATZ___024ROOT_H_  // guard

#include "verilated.h"


class Vcollatz__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vcollatz___024root final {
  public:

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(go,0,0);
    VL_OUT8(done,0,0);
    CData/*0:0*/ collatz__DOT__busy;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP__clk__0;
    VL_IN(n,31,0);
    VL_OUT(dout,31,0);
    IData/*31:0*/ collatz__DOT__temp;
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;

    // INTERNAL VARIABLES
    Vcollatz__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vcollatz___024root(Vcollatz__Syms* symsp, const char* namep);
    ~Vcollatz___024root();
    VL_UNCOPYABLE(Vcollatz___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
