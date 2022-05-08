#pragma once
#include "pointers.hpp"
#include "rage/script_thread.hpp"
#include "rage/script_program.hpp"
#include "rage/natives.hpp"
#include <commdlg.h>

namespace ext
{
	class script_loader
	{
	public:
		script_loader();
		~script_loader();
	public:
		void find_thread();
		void restore_thread();
		void pick_script();
		void initalize_thread();
	public:
		std::map<uint64_t, uint64_t> m_handler_cache{};
		rage::scrNativeRegistrationTable m_native_registration;
		rage::joaat_t m_required_script;
		rage::scrThread m_thread{ 0 };
		rage::scrProgram m_program{ 0 };
		std::uint64_t m_fake_vft;
		TCHAR m_file_name[260]{};
		fs::path m_file_path;
		std::string m_selected_script_name;
	};

	inline script_loader* g_script_loader;
}