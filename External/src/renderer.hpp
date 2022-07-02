#pragma once
#include "common.hpp"
#include <d3d11.h>
#include "imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class renderer {
public:
	renderer() = default;
	~renderer() = default;
public:
	void start_renderer();
	void stop_renderer();
public:
	bool create_d3d(HWND hWnd);
	void cleanup_d3d();
	void create_target();
	void cleanup_target();
public:
	bool present_hk();
public:
	bool isWndOpen = true;
	void window();
public:
	static LRESULT WINAPI wndproc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
public:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	IDXGISwapChain* m_swapchain;
	ID3D11RenderTargetView* m_targetview;
	DXGI_SWAP_CHAIN_DESC m_desc;
	HWND m_hwnd;
	WNDCLASSEX m_wndclsex;
public:
	ImFont* m_Font;
	ImFont* m_DefaultFont;
	ImFont* m_TitleFont;
	ImFont* m_BarFont;
private:
	ImFontConfig m_font_config;
};
inline renderer g_renderer;