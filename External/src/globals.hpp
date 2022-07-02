#pragma once
#include "common.hpp"
#include "pointers.hpp"

class globals {
private:
	uint64_t m_index;
	static uintptr_t getScriptGlobal(uint64_t index) {
		auto baseGlobalAddress = ext::g_process->read<uintptr_t>(ext::g_pointers->m_script_globals + 8 * ((index >> 0x12) & 0x3F));
		return 8 * (index & 0x3FFFF) + baseGlobalAddress;
	}
public:
	globals(uint64_t index) : m_index(index) {}
	globals at(uint64_t index) { return globals(m_index + index); }
	globals at(uint64_t index, uint64_t size) { return at(1 + (index * size)); }
	//Script Globals
	template <typename T>
	void as(T value) { ext::g_process->write<T>(getScriptGlobal(m_index), value); }
};