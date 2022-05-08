#include <iostream>
#include <natives.hpp>
#include <ysc.hpp>

#include "process.hpp"
#include "pattern.hpp"
#include "pointers.hpp"
#include "rage/natives.hpp"
#include "natives.hpp"
#include "script_loader.hpp"
#include "renderer.hpp"
#pragma comment(lib, "C:\\Users\\iiFaint\\Desktop\\External\\bin\\lib\\Release\\ImGui.lib")

using namespace ext;

inline std::uint32_t find_gta_proc_id() 
{
	auto win = FindWindowA("grcWindow", nullptr);
	if (!win) {
		LOG(FATAL) << "Cannot find game window";
	}

	DWORD a;
	GetWindowThreadProcessId(win, &a);

	return a;
}

int main()
{
	std::filesystem::path base_dir = std::getenv("appdata");
	base_dir /= "External";

	auto file_manager_instance = std::make_unique<file_manager>(base_dir);

	auto logger_instance = std::make_unique<logger>(
		"External",
		file_manager_instance->get_project_file("./cout.log")
	);

	LOG(INFO) << "External loaded.";

	auto process_instance = std::make_unique<process>(find_gta_proc_id());
	LOG(INFO) << "Process initalized.";

	auto pointers_instance = std::make_unique<pointers>();
	LOG(INFO) << "Pointers initialized.";

	auto renderer_instance = std::make_unique<renderer>();
	LOG(INFO) << "Renderer initialized.";
	g_renderer.start_renderer();

	auto script_loader_instance = std::make_unique<script_loader>();
	LOG(INFO) << "Script loader initialized.";

	while (g_running && g_process->is_running()) 
	{
		g_renderer.present_hk();
		if (GetAsyncKeyState(VK_END))
			g_running = false;
		while (PeekMessage(&g_msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&g_msg);
			DispatchMessage(&g_msg);
			if (g_msg.message == WM_QUIT)
				exit(0);
		}
	}

	script_loader_instance.reset();
	LOG(INFO) << "Script loader uninitialized.";

	pointers_instance.reset();
	LOG(INFO) << "Pointers uninitialized.";

	g_renderer.stop_renderer();
	renderer_instance.reset();
	LOG(INFO) << "Renderer uninitialized.";

	process_instance.reset();
	LOG(INFO) << "Process uninitialized.";

	LOG(INFO) << "Farewell!";

	logger_instance.reset();
	file_manager_instance.reset();

	return 0;
}