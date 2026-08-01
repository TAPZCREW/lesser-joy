module;

#include "windows.hpp"

export module lesserjoy;

import std;

import stormkit.core;
import stormkit.log;

import lesserjoy.log;

module: private;

using namespace stormkit;

#pragma code_seg("INIT")

auto logger = heap_ptr<lj::KernelLogger> {};

////////////////////////////////////////
////////////////////////////////////////
extern "C" __declspec(dllexport) auto APIENTRY DllMain(HMODULE module, DWORD, LPVOID) -> BOOL {
    if (not logger) logger = log::Logger::allocate_logger_instance<lj::KernelLogger>();
#ifdef STORMKIT_DEBUG_MODE
    logger->set_severity_mask(logger->severity_mask() | log::Severity::DEBUG);
#endif

    DisableThreadLibraryCalls(module);

    return TRUE;
}

#pragma code_seg()
