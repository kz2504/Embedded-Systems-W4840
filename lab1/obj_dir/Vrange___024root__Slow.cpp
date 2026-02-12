// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vrange.h for the primary calling header

#include "Vrange__pch.h"

void Vrange___024root___ctor_var_reset(Vrange___024root* vlSelf);

Vrange___024root::Vrange___024root(Vrange__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vrange___024root___ctor_var_reset(this);
}

void Vrange___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vrange___024root::~Vrange___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
