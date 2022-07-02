#include "renderer.hpp"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <functional>
#include <D3D11.h>
#include <script_loader.hpp>

std::string nameWithoutExt = "";
void renderer::window() {
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	static ImVec2 mainWindowSize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowSize(ImVec2(600.f, 300.f));
	if (isWndOpen) {
		if (ImGui::Begin("External", &ext::g_running, windowFlags)) {
			ImGui::BeginTabBar("MainTabBar");
			if (ImGui::BeginTabItem("Script Selector")) {
				if (ImGui::ListBoxHeader("##scriptSelectorBox", ImVec2(250.f, 190.f))) {
					for (auto&& dirEntry : fs::directory_iterator{ ext::g_script_loader->m_file_path }) {
						if (dirEntry.is_regular_file()) {
							auto path = dirEntry.path();
							auto isYscFull = path.string().find(".ysc.full") != -1;
							if (path.has_filename() && (path.extension() == ".ysc" || isYscFull)) {
								if (ImGui::Selectable(path.filename().string().c_str(), (ext::g_script_loader->m_selected_script_name == path.filename().string()))) {
									ext::g_script_loader->m_selected_script_name = path.filename().string();
									if (isYscFull)
										nameWithoutExt = path.stem().stem().string();
									else
										nameWithoutExt = path.stem().string();
								}
							}
						}
					}
					ImGui::ListBoxFooter();
				}
				if (ImGui::Button("Restore orginial script")) {
					if (ext::g_script_loader->m_thread.valid())
						ext::g_script_loader->m_thread.set_state(rage::eThreadState::Paused);
				}
				if (!ext::g_script_loader->m_selected_script_name.empty()) {
					ImGui::SameLine();
					if (ImGui::Button(("Load " + nameWithoutExt).c_str()))
						ext::g_script_loader->initalize_thread(); //Start script execution
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Settings")) {
				static char pathTxt[256]{};
				ImGui::InputText("Path", pathTxt, sizeof(pathTxt));
				ImGui::SameLine();
				if (ImGui::Button("Set")) {
					std::string str = pathTxt;
					if (!str.empty()) {
						if (auto v = str.find("%appdata%"); v != -1)
							ext::g_script_loader->m_file_path = fs::path(std::getenv("appdata")).append(str.substr(v + 10));
						else
							ext::g_script_loader->m_file_path = fs::path(str);
					}
				}
				if (ImGui::Button("Unload"))
					ext::g_running = false;
				ImGui::SameLine();
				if (ImGui::Button("Close"))
					exit(0);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
			ImGui::End();
		}
	}
}
bool renderer::present_hk() {
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	window();
	ImGui::EndFrame();
	//Rendering
	ImGui::Render();
	const float clear_color_with_alpha[4] = { 0.20f, 0.20f, 0.20f, 0.00f };
	g_renderer.m_context->OMSetRenderTargets(1, &g_renderer.m_targetview, NULL);
	g_renderer.m_context->ClearRenderTargetView(g_renderer.m_targetview, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImGui::UpdatePlatformWindows(); //Called for the viewport
	ImGui::RenderPlatformWindowsDefault(); //Called for the viewport
	g_renderer.m_swapchain->Present(1, 0); //Present with vsync
	//g_renderer.m_swapchain->Present(0, 0); //Present without vsync
	return true;
}
LRESULT WINAPI renderer::wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam))
		return true;
	if (Msg == WM_SIZE && g_renderer.m_device != NULL && wParam != SIZE_MINIMIZED) {
		g_renderer.cleanup_target();
		g_renderer.m_swapchain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
		g_renderer.create_target();
		return 0;
	}
	else if (Msg == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}
void renderer::start_renderer() {
	m_wndclsex = { sizeof(WNDCLASSEX), CS_CLASSDC, wndproc, 0L, 0L, GetModuleHandle(0), 0, 0, 0, 0, L"wndClass", 0 };
	RegisterClassEx(&m_wndclsex);
	m_hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		m_wndclsex.lpszClassName, L"External Overlay",
		100, 100, 1280, 800,
		NULL, NULL, NULL, NULL,
		m_wndclsex.hInstance
	);
	if (!create_d3d(m_hwnd)) {
		cleanup_d3d();
		UnregisterClass(m_wndclsex.lpszClassName, m_wndclsex.hInstance);
	}
	SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
	//Show the window
	ShowWindow(m_hwnd, SW_HIDE);
	UpdateWindow(m_hwnd);
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; //Enable Viewport (Allows for no window background)
	auto&& style = ImGui::GetStyle(); auto&& colors = style.Colors;
	style.Alpha = 1.f;
	style.WindowPadding = ImVec2(8, 8);
	style.PopupRounding = 0.f;
	style.FramePadding = { 8.f, 4.f };
	style.ItemSpacing = ImVec2(8, 4);
	style.ItemInnerSpacing = ImVec2(4, 4);
	style.TouchExtraPadding = { 0.f, 0.f };
	style.IndentSpacing = 21.f;
	style.ScrollbarSize = 8.f;
	style.ScrollbarRounding = 8.f;
	style.GrabMinSize = 6.f;
	style.GrabRounding = 4.25f;
	style.WindowBorderSize = 0.f;
	style.ChildBorderSize = 0.f;
	style.PopupBorderSize = 0.f;
	style.FrameBorderSize = 0.f;
	style.TabBorderSize = 0.f;
	style.WindowRounding = 0.f;
	style.ChildRounding = 3.f;
	style.FrameRounding = 5.f;
	style.TabRounding = 7.f;
	style.WindowTitleAlign = { 0.5f, 0.5f };
	style.ButtonTextAlign = { 0.5f, 0.5f };
	style.DisplaySafeAreaPadding = { 3.f, 3.f };
	colors[ImGuiCol_Text] = ImVec4(1.f, 1.f, 1.f, 1.f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.f);
	colors[ImGuiCol_Border] = ImVec4(1.f, 1.f, 1.f, 0.88f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.f, 1.f, 1.f, 1.f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.f);
	colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
	colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.1f, 0.1f, 0.1f, 1.f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.f);
	colors[ImGuiCol_Separator] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.f, 0.00f, 1.f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.f, 0.00f, 1.f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.f, 0.00f, 0.43f);
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(g_renderer.m_device, g_renderer.m_context);
	//Arial Font Names: Bold Italic = ArialBI | Bold = ArialBD | Black = ArialBLK | Italic = ArialI | Regular = Arial
	g_renderer.m_font_config.FontDataOwnedByAtlas = false;
	g_renderer.m_Font = ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 18.f, &g_renderer.m_font_config);
	g_renderer.m_TitleFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\DXLaunch\\Fonts\\TitleFont.ttf", 16.f, &g_renderer.m_font_config);
	g_renderer.m_BarFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\ArialBI.ttf", 20.f, &g_renderer.m_font_config);
}
void renderer::stop_renderer() {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	cleanup_d3d();
	DestroyWindow(g_renderer.m_hwnd);
	UnregisterClass(g_renderer.m_wndclsex.lpszClassName, g_renderer.m_wndclsex.hInstance);
}
bool renderer::create_d3d(HWND hWnd) {
	//Setup the sc
	DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2; sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd; sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0; sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; UINT createDeviceFlags = 0;
	//CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel; const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swapchain, &m_device, &featureLevel, &m_context) != S_OK)
		return false;
	create_target();
	return true;
}
void renderer::cleanup_d3d() {
	cleanup_target();
	if (m_swapchain) { m_swapchain->Release(); m_swapchain = NULL; }
	if (m_device) { m_device->Release(); m_device = NULL; }
	if (m_context) { m_context->Release(); m_context = NULL; }
	DestroyWindow(g_renderer.m_hwnd);
	UnregisterClass(g_renderer.m_wndclsex.lpszClassName, g_renderer.m_wndclsex.hInstance);
}
void renderer::create_target() {
	ID3D11Texture2D* pBackBuffer;
	m_swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	m_device->CreateRenderTargetView(pBackBuffer, NULL, &m_targetview);
	pBackBuffer->Release();
}
void renderer::cleanup_target() {
	if (m_targetview) {
		m_targetview->Release();
		m_targetview = NULL;
	}
}