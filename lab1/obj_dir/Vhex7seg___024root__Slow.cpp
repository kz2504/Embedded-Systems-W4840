// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vhex7seg.h for the primary calling header

#include "Vhex7seg__pch.h"

void Vhex7seg___024root___ctor_var_reset(Vhex7seg___024root* vlSelf);

Vhex7seg___024root::Vhex7seg___024root(Vhex7seg__Syms* symsp, const char* namep)
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vhex7seg___024root___ctor_var_reset(this);
}

void Vhex7seg___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vhex7seg___024root::~Vhex7seg___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
