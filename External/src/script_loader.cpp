#include "script_loader.hpp"
#include "crossmap.hpp"
#include "rage/natives.hpp"
#include "natives.hpp"
#include "ysc_file.hpp"
#include <urlmon.h>
#include "globals.hpp"
#pragma comment(lib, "urlmon")

namespace ext 
{
	script_loader::script_loader() :
		m_native_registration(g_pointers->m_native_registration_table),
		m_required_script($(animal_controller))
	{
		find_thread();
		m_file_path.append(std::getenv("appdata")).append("External").append("Scripts");
		if (!fs::exists(m_file_path))
			fs::create_directories(m_file_path);
		g_script_loader = this;
	}

	script_loader::~script_loader()
	{
		if (m_thread.valid() && g_running) {
			m_thread.set_state(rage::eThreadState::Paused);
			if (!m_selected_script_name.empty())
				restore_thread();
		}
		g_script_loader = nullptr;
	}

	void script_loader::find_thread()
	{
		LOG(INFO) << "Waiting for script...";
		while ((!m_thread.valid()) || (!m_program.valid())) {
			m_thread = rage::scrThread::get_thread_by_hash(m_required_script);
			m_program = rage::scrProgram::get_program_by_hash(m_required_script);
			Sleep(100);
		}
	}

	void script_loader::restore_thread() {
		auto path = fs::path(std::getenv("appdata")).append("External").append("Scripts");
		HRESULT hr = 1;
		if (!fs::exists(path.append("animal_controller.ysc")))
			hr = URLDownloadToFileA(
				NULL, "https://github.com/Sainan/GTA-V-Decompiled-Scripts/raw/master/scripts/animal_controller_ysc/animal_controller.ysc.full",
				path.append("animal_controller.ysc").string().c_str(),0, NULL);
		if (SUCCEEDED(hr)) {
			m_file_path = path.string();
			m_selected_script_name = "animal_controller.ysc";
			initalize_thread();
		}
	}
	void script_loader::initalize_thread() {
		auto newFilePath = m_file_path;
		newFilePath.append(m_selected_script_name.c_str());
		ysc_file file(newFilePath);
		file.load();
		for (auto& p : file.m_natives)
			m_handler_cache[p] = m_native_registration.get_entrypoint_from_hash(p);
		m_thread.set_state(rage::eThreadState::Paused);
		g_process->set_paused(true);
		m_thread.reset();
		if (m_program.get_code_size() < file.m_code_length)
			LOG(FATAL) << "Cannot fit " << file.m_code_length << " bytes into program, maximum is " << m_program.get_code_size();
		if (m_program.get_string_size() < file.m_string_size)
			LOG(FATAL) << "Cannot fit " << file.m_string_size << " string chars into program, maximum is " << m_program.get_string_size();
		if (m_program.get_num_natives() < file.m_natives_count)
			LOG(FATAL) << "Cannot fit " << file.m_natives_count << " natives into program, maximum is " << m_program.get_num_natives();
		if ((m_thread.get_stack_size() - 200) < file.m_static_count)
			LOG(FATAL) << "Cannot fit " << file.m_static_count << " statics into stack, maximum is " << (m_thread.get_stack_size() - 200);
		for (int i = 0; i < file.m_code_block_list.size(); i++) {
			int tablesize = ((i + 1) * 0x4000 >= file.m_code_length) ? file.m_code_length % 0x4000 : 0x4000;
			uint64_t codepage = m_program.get_code_page(i);
			g_process->write_raw(codepage, tablesize, file.m_code_block_list[i]);
		}
		for (int i = 0; i < file.m_string_block_list.size(); i++) {
			int tablesize = ((i + 1) * 0x4000 >= file.m_string_size) ? file.m_string_size % 0x4000 : 0x4000;
			uint64_t stringpage = m_program.get_string_page(i);
			g_process->write_raw(stringpage, tablesize, file.m_string_block_list[i]);
		}
		uint64_t natives = m_program.get_native_table();
		for (int i = 0; i < file.m_natives.size(); i++) {
			g_process->write<std::uint64_t>(natives + (uint64_t)(i * 8), m_handler_cache[file.m_natives[i]]);
		}
		g_process->write_raw(m_thread.get_stack(), file.m_static_count * 8, file.m_statics);
		m_thread.set_stack_ptr(file.m_static_count + 1);
		// model spawn bypass
		auto m_original_vft = m_thread.get_handler_vft();
		m_fake_vft = g_process->allocate(20 * 8);
		for (uint64_t i = 0; i < 20; i++) {
			g_process->write<uint64_t>(m_fake_vft + (i * 8), g_process->read<uint64_t>(m_original_vft + (i * 8)));
		}
		g_process->write<uint64_t>(m_fake_vft + (6 * 8), g_pointers->m_ret_true_function);
		m_thread.set_handler_vft(m_fake_vft);
		// dlc story mode bypass
		globals(4533757).as<bool>(true);
		m_program.mark_program_as_ours();
		g_process->set_paused(false);
		m_thread.set_state(rage::eThreadState::Running);
	}
}