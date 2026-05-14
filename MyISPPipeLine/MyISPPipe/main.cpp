#include<iostream>
#include <objbase.h> // 必须包含
#include <windows.h>
#include <commdlg.h> // 必须包含这个头文件
#include "../Pipe/mangepipe.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <ShellScalingApi.h>
#include <fstream>
#include <sstream>
#include <vector>
#include< algorithm >
#define GUI 1
extern char current_raw_path[260];
extern stISPParams gISPparamHDRcombine;
extern stISPParams gISPparamLinear;
extern stRawInfo  grawinfo;
extern eISPMode g_current_mode;
#if !GUI
int main()
{
	int ret = 0;
	ret=PipeInit();
	ret=PipeRun();

	return 0;
}
#endif

# if GUI


// 辅助函数：从 TXT 加载 PWL 数据
void LoadPWLFromTxt(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    // 临时存储，用于排序
    struct Point { u32 x, y; };
    std::vector<Point> pts;

    std::string line;
    while (std::getline(file, line) && pts.size() < 34) {
        std::stringstream ss(line);
        u32 tx, ty;
        // 尝试读取，支持空格或逗号分隔
        if (ss >> tx >> ty) {
            pts.push_back({ tx, ty });
        }
    }

    // 关键：按照 X 从小到大排序，防止硬件逻辑死锁
    std::sort(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
        return a.x < b.x;
        });

    // 填充回全局结构体
    gISPparamHDRcombine.depwl_param.neepoint = (u32)pts.size();
    for (size_t i = 0; i < pts.size(); i++) {
        gISPparamHDRcombine.depwl_param.x[i] = pts[i].x;
        gISPparamHDRcombine.depwl_param.y[i] = pts[i].y;
    }
    file.close();
}
std::string OpenFileDialog(HWND hwnd) {
    char szFile[260] = { 0 };
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd; // 关键：传入主窗口句柄，让对话框成为子窗口
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Raw Files (*.raw)\0*.raw\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    return "";
}
// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderISPParameterWindow() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    if (!ImGui::Begin("ISP Technical Tuning Terminal", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
    {
        ImGui::End();
        return;
    }
    ImGui::Text("Select Processing Mode:");
    
    if (ImGui::RadioButton("Linear Mode", g_current_mode ==  ISPLinear)) {
        g_current_mode = ISPLinear;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("HDR Mode ", g_current_mode == ISPCombined)) {
        g_current_mode = ISPCombined;
    }
    ImGui::Separator();


    //ImGui::Begin("ISP Technical Tuning Terminal", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // ---  RAW Global Information ---
    if (ImGui::CollapsingHeader(" RAW path", ImGuiTreeNodeFlags_DefaultOpen)) {

        // 显示当前路径（只读输入框）
        ImGui::InputText("choose the Path", current_raw_path, sizeof(current_raw_path), ImGuiInputTextFlags_ReadOnly);
        ImGui::SameLine();

        // 点击按钮打开 Windows 选择框
        if (ImGui::Button("Browse...")) {
            std::string path = OpenFileDialog(NULL);
            if (!path.empty()) {
                strcpy_s(current_raw_path, path.c_str());
                // 可选：选中文件后自动执行一次 Init
             
            }
        }

        // ... 宽度、高度、Bayer 等其他 Input 控件 ...
    }
    if (g_current_mode == ISPCombined)
    {
    // --- 0. RAW Global Info (With Enums) ---
    if (ImGui::CollapsingHeader("0. RAW Information", ImGuiTreeNodeFlags_DefaultOpen)) {

        int w = (int)gISPparamHDRcombine.rawinfo.W;
        int h = (int)gISPparamHDRcombine.rawinfo.H;
        int b = (int)gISPparamHDRcombine.rawinfo.rawbit;

        if (ImGui::InputInt("Width", &w))  gISPparamHDRcombine.rawinfo.W = (u16)w;
        if (ImGui::InputInt("Height", &h)) gISPparamHDRcombine.rawinfo.H = (u16)h;
        const char* bit_depths[] = { "12", "16" };
        int current_selection = (gISPparamHDRcombine.rawinfo.rawbit == 16) ? 1 : 0; // 映射索引

        if (ImGui::Combo("Bit Depth", &current_selection, bit_depths, IM_ARRAYSIZE(bit_depths))) {
            gISPparamHDRcombine.rawinfo.rawbit = (current_selection == 1) ? 16 : 12;
        }
        // Bayer Pattern Enum Selection
        const char* bayer_types[] = { "RGGB", "BGGR", "GBRG", "GRBG" };
        int current_bayer = (int)gISPparamHDRcombine.rawinfo.BayerType;
        if (ImGui::Combo("Bayer Pattern", &current_bayer, bayer_types, IM_ARRAYSIZE(bayer_types))) {
            gISPparamHDRcombine.rawinfo.BayerType =(eBayerPattern) current_bayer;
        }

    }

        // --- 1. DEPWL (With Dynamic Neepoint) ---
    if (ImGui::CollapsingHeader("1. DEPWL - De-companding")) {

        // --- 新增：批量加载按钮 ---
        if (ImGui::Button("Load PWL Table from TXT")) {
            // 调用之前定义的现代文件打开对话框 (注意传入 main_hwnd)
            std::string txtPath = OpenFileDialog(NULL);
            if (!txtPath.empty()) {
                LoadPWLFromTxt(txtPath);
            }
        }

        // --- 原有的 Neepoint 配置 ---
        int n_point = (int)gISPparamHDRcombine.depwl_param.neepoint;

        if (ImGui::InputInt("Knee Points (Neepoint)", &n_point)) {

            if (n_point < 2) n_point = 2;   // 至少两点成线

            if (n_point > 34) n_point = 34; // 这里的最大值根据你数组分配的大小决定

            gISPparamHDRcombine.depwl_param.neepoint = (u32)n_point;

        }
        ImGui::Text("PWL Table (X: Input, Y: Output)");
        for (int i = 0; i < (int)gISPparamHDRcombine.depwl_param.neepoint; i++) {
            char label[32];
            sprintf_s(label, sizeof(label), "P%d", i);
            int x_val = (int)gISPparamHDRcombine.depwl_param.x[i];
            int y_val = (int)gISPparamHDRcombine.depwl_param.y[i];
            ImGui::PushID(i); // 防止同名控件冲突
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("X", &x_val, 0, 0)) gISPparamHDRcombine.depwl_param.x[i] = (u32)x_val;
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            if (ImGui::InputInt("Y", &y_val, 0, 0)) gISPparamHDRcombine.depwl_param.y[i] = (u32)y_val;
            ImGui::SameLine();
            ImGui::Text("%s", label);
            ImGui::PopID();
        }

    }
        //--3.dpc
        if (ImGui::CollapsingHeader("2. DPC - Dead Pixel Correction")) {
            bool en = (gISPparamHDRcombine.dpc_Param.enable != 0);
            if (ImGui::Checkbox("Enable DPC", &en)) gISPparamHDRcombine.dpc_Param.enable = en;

            // 这里的 threshold 数值通常很大，InputInt 比滑块好用得多
            ImGui::InputInt("Threshold", (int*)&gISPparamHDRcombine.dpc_Param.threshold, 100, 1000);
        }

        // --- 2. BLC ---
        if (ImGui::CollapsingHeader("3. BLC - Black Level")) {
            bool en = (gISPparamHDRcombine.blc_Param.enable != 0);
            if (ImGui::Checkbox("Enable BLC", &en)) gISPparamHDRcombine.blc_Param.enable = en;
            int r = gISPparamHDRcombine.blc_Param.blc_r, g = gISPparamHDRcombine.blc_Param.blc_g, b = gISPparamHDRcombine.blc_Param.blc_b;
            int blcbit = gISPparamHDRcombine.blc_Param.BlcBit;
            if (ImGui::InputInt("BLC R", &r)) gISPparamHDRcombine.blc_Param.blc_r = (u16)r;
            if (ImGui::InputInt("BLC G", &g)) gISPparamHDRcombine.blc_Param.blc_g = (u16)g;
            if (ImGui::InputInt("BLC B", &b)) gISPparamHDRcombine.blc_Param.blc_b = (u16)b;

            if (ImGui::InputInt("BLC bit", &blcbit)) gISPparamHDRcombine.blc_Param.BlcBit = (u16)blcbit;
        }
        // --- 4. AWB ---
        if (ImGui::CollapsingHeader("4. AWB - White Balance")) {
            bool en = (gISPparamHDRcombine.awb_Param.enable != 0);
            if (ImGui::Checkbox("Enable AWB", &en)) gISPparamHDRcombine.awb_Param.enable = en;
            ImGui::InputInt("R Gain", &gISPparamHDRcombine.awb_Param.r_gain);
            ImGui::InputInt("B Gain", &gISPparamHDRcombine.awb_Param.b_gain);
        }

        // --- 5. Tonemap ---
        if (ImGui::CollapsingHeader("5. Tonemap - Detailed Tuning", ImGuiTreeNodeFlags_DefaultOpen)) {
            float lamda = (float)gISPparamHDRcombine.tonemap_Param.lamda;
            if (ImGui::InputFloat("Lambda", &lamda, 0.01f, 0.1f, "%.4f")) gISPparamHDRcombine.tonemap_Param.lamda = (double)lamda;

            float tauR = (float)gISPparamHDRcombine.tonemap_Param.tauR;
            if (ImGui::InputFloat("TauR", &tauR)) gISPparamHDRcombine.tonemap_Param.tauR = (double)tauR;

            float p_val = (float)gISPparamHDRcombine.tonemap_Param.P;
            if (ImGui::InputFloat("P (Exposure)", &p_val, 0.00001f, 0.0001f, "%.7f")) gISPparamHDRcombine.tonemap_Param.P = (double)p_val;

            float etaF = (float)gISPparamHDRcombine.tonemap_Param.etaF;
            if (ImGui::InputFloat("EtaF", &etaF)) gISPparamHDRcombine.tonemap_Param.etaF = (double)etaF;

            float etaC = (float)gISPparamHDRcombine.tonemap_Param.etaC;
            if (ImGui::InputFloat("EtaC", &etaC)) gISPparamHDRcombine.tonemap_Param.etaC = (double)etaC;

            float bpcMax = (float)gISPparamHDRcombine.tonemap_Param.BPCmax;
            if (ImGui::InputFloat("BPC Max", &bpcMax)) gISPparamHDRcombine.tonemap_Param.BPCmax = (double)bpcMax;

            float bpcMin = (float)gISPparamHDRcombine.tonemap_Param.BPCmin;
            if (ImGui::InputFloat("BPC Min", &bpcMin)) gISPparamHDRcombine.tonemap_Param.BPCmin = (double)bpcMin;

            float n_sigma = (float)gISPparamHDRcombine.tonemap_Param.noise_sigma;
            if (ImGui::InputFloat("Noise Sigma", &n_sigma)) gISPparamHDRcombine.tonemap_Param.noise_sigma = (double)n_sigma;

            ImGui::InputFloat("Shadow Boost", &gISPparamHDRcombine.tonemap_Param.shadow_boost);
            ImGui::InputFloat("Brightness Ratio", &gISPparamHDRcombine.tonemap_Param.brightness_ratio);
            ImGui::InputFloat("Contrast K", &gISPparamHDRcombine.tonemap_Param.contrast_k);
            ImGui::InputFloat("Light Compress", &gISPparamHDRcombine.tonemap_Param.light_compress);
            ImGui::InputInt("TMP bit", &gISPparamHDRcombine.tonemap_Param.TMPBit);
        }

        // --- 6. RNR (Raw Noise Reduction) ---
        if (ImGui::CollapsingHeader("6. RNR - Raw Denoise")) {
            bool en = (gISPparamHDRcombine.rnr_Param.enable != 0);
            if (ImGui::Checkbox("Enable RNR", &en)) gISPparamHDRcombine.rnr_Param.enable = en;

            // 使用 InputFloat 替代滑块，步进设为 0.1
            ImGui::InputFloat("H_R (Red)", &gISPparamHDRcombine.rnr_Param.h_r, 0.1f, 1.0f, "%.2f");
            ImGui::InputFloat("H_G (Green)", &gISPparamHDRcombine.rnr_Param.h_g, 0.1f, 1.0f, "%.2f");
            ImGui::InputFloat("H_B (Blue)", &gISPparamHDRcombine.rnr_Param.h_b, 0.1f, 1.0f, "%.2f");
        }
        //--demosaic
        if (ImGui::CollapsingHeader("7. Demosaic")) {
            float m_alpha = (float)gISPparamHDRcombine.demosaic_Param.m_alpha;
            if (ImGui::InputFloat("Alpha", &m_alpha)) gISPparamHDRcombine.demosaic_Param.m_alpha = (double)m_alpha;
            ImGui::InputInt("HV Gap", &gISPparamHDRcombine.demosaic_Param.m_HVgap);
            int tc = gISPparamHDRcombine.demosaic_Param.thr_c, ty = gISPparamHDRcombine.demosaic_Param.thr_y;
            if (ImGui::InputInt("Thr C", &tc)) gISPparamHDRcombine.demosaic_Param.thr_c = (u16)tc;
            if (ImGui::InputInt("Thr Y", &ty)) gISPparamHDRcombine.demosaic_Param.thr_y = (u16)ty;
        }

        // --- 7. CCM ---
        if (ImGui::CollapsingHeader("8. CCM - Color Matrix")) {
            bool en = (gISPparamHDRcombine.ccm_Param.enable != 0);
            if (ImGui::Checkbox("Enable Gamma", &en)) gISPparamHDRcombine.ccm_Param.enable = en;
            for (int i = 0; i < 3; i++) {
                float row[3] = { (float)gISPparamHDRcombine.ccm_Param.sccm[i][0], (float)gISPparamHDRcombine.ccm_Param.sccm[i][1], (float)gISPparamHDRcombine.ccm_Param.sccm[i][2] };
                std::string lbl = "Row " + std::to_string(i);
                if (ImGui::InputFloat3(lbl.c_str(), row, "%.4f")) {
                    gISPparamHDRcombine.ccm_Param.sccm[i][0] = (double)row[0];
                    gISPparamHDRcombine.ccm_Param.sccm[i][1] = (double)row[1];
                    gISPparamHDRcombine.ccm_Param.sccm[i][2] = (double)row[2];
                }
            }
        }
        // --- 7. Gamma Correction ---
        if (ImGui::CollapsingHeader("9. Gamma Correction")) {

            ImGui::InputFloat("Gamma Power", &gISPparamHDRcombine.gamma_Param.gamma_power, 0.01f, 0.1f, "%.2f");
        }
        // --- 8. contrast ---
        if (ImGui::CollapsingHeader("10. contrast")) {

            bool en = (gISPparamHDRcombine.contrast_Param.enable != 0);
            if (ImGui::Checkbox("Enable contrast", &en)) gISPparamHDRcombine.contrast_Param.enable = en;
            int contrastbit = gISPparamHDRcombine.contrast_Param.contrastbit;
            int contraststrength = gISPparamHDRcombine.contrast_Param.ContrastStrength;
            ImGui::InputInt("contrastbit", &contrastbit);
            ImGui::InputInt("contraststrength", &contraststrength);
            gISPparamHDRcombine.contrast_Param.ContrastStrength = contraststrength;
            gISPparamHDRcombine.contrast_Param.contrastbit=contrastbit;

        }

        // --- 9. Sharpen ---
        if (ImGui::CollapsingHeader("11. Sharpening")) {
            bool en = (gISPparamHDRcombine.sharpen_Param.enable != 0);
            if (ImGui::Checkbox("Enable Sharpen", &en)) gISPparamHDRcombine.sharpen_Param.enable = en;
            ImGui::InputFloat("Gain Fine", &gISPparamHDRcombine.sharpen_Param.gain_fine);
            ImGui::InputFloat("Gain mid", &gISPparamHDRcombine.sharpen_Param.gain_mid);
            ImGui::InputFloat("coring_high", &gISPparamHDRcombine.sharpen_Param.coring_high);
            ImGui::InputInt("UnderShot Limit", &gISPparamHDRcombine.sharpen_Param.under_shot_limit);
            ImGui::InputInt("OverShot Limit", &gISPparamHDRcombine.sharpen_Param.over_shot_limit);
            int sharpenbit = gISPparamHDRcombine.sharpen_Param.sharpenbit;
            ImGui::InputInt("sharpen bit", &sharpenbit);
            gISPparamHDRcombine.sharpen_Param.sharpenbit = sharpenbit;
        }
        ImGui::Separator();
    }
    if (g_current_mode == ISPLinear)
    {
        // --- 0. RAW Global Info (With Enums) ---
        if (ImGui::CollapsingHeader("0. RAW Information", ImGuiTreeNodeFlags_DefaultOpen)) {

            int w = (int)gISPparamLinear.rawinfo.W;
            int h = (int)gISPparamLinear.rawinfo.H;
            int b = (int)gISPparamLinear.rawinfo.rawbit;

            if (ImGui::InputInt("Width", &w))  gISPparamLinear.rawinfo.W = (u16)w;
            if (ImGui::InputInt("Height", &h)) gISPparamLinear.rawinfo.H = (u16)h;
            const char* bit_depths[] = { "12", "16" };
            int current_selection = (gISPparamLinear.rawinfo.rawbit == 16) ? 1 : 0; // 映射索引

            if (ImGui::Combo("Bit Depth", &current_selection, bit_depths, IM_ARRAYSIZE(bit_depths))) {
                gISPparamLinear.rawinfo.rawbit = (current_selection == 1) ? 16 : 12;
            }
            // Bayer Pattern Enum Selection
            const char* bayer_types[] = { "RGGB", "BGGR", "GBRG", "GRBG" };
            int current_bayer = (int)gISPparamLinear.rawinfo.BayerType;
            if (ImGui::Combo("Bayer Pattern", &current_bayer, bayer_types, IM_ARRAYSIZE(bayer_types))) {
                gISPparamLinear.rawinfo.BayerType = (eBayerPattern)current_bayer;
            }

        }

        //--1.dpc
        if (ImGui::CollapsingHeader("1. DPC - Dead Pixel Correction")) {
            bool en = (gISPparamLinear.dpc_Param.enable != 0);
            if (ImGui::Checkbox("Enable DPC", &en)) gISPparamLinear.dpc_Param.enable = en;

            // 这里的 threshold 数值通常很大，InputInt 比滑块好用得多
            ImGui::InputInt("Threshold", (int*)&gISPparamLinear.dpc_Param.threshold, 100, 1000);
        }
        // --- 2. BLC ---
        if (ImGui::CollapsingHeader("2. BLC - Black Level")) {
            bool en = (gISPparamLinear.blc_Param.enable != 0);
            if (ImGui::Checkbox("Enable BLC", &en)) gISPparamLinear.blc_Param.enable = en;
            int r = gISPparamLinear.blc_Param.blc_r, g = gISPparamLinear.blc_Param.blc_g, b = gISPparamLinear.blc_Param.blc_b;
            int blcbit = gISPparamLinear.blc_Param.BlcBit;
            if (ImGui::InputInt("BLC R", &r)) gISPparamLinear.blc_Param.blc_r = (u16)r;
            if (ImGui::InputInt("BLC G", &g)) gISPparamLinear.blc_Param.blc_g = (u16)g;
            if (ImGui::InputInt("BLC B", &b)) gISPparamLinear.blc_Param.blc_b = (u16)b;

            if (ImGui::InputInt("BLC bit", &blcbit)) gISPparamLinear.blc_Param.BlcBit = (u16)blcbit;
        }

        // --- 4. AWB ---
        if (ImGui::CollapsingHeader("3. AWB - White Balance")) {
            bool en = (gISPparamLinear.awb_Param.enable != 0);
            if (ImGui::Checkbox("Enable AWB", &en)) gISPparamLinear.awb_Param.enable = en;
            ImGui::InputInt("R Gain", &gISPparamLinear.awb_Param.r_gain);
            ImGui::InputInt("B Gain", &gISPparamLinear.awb_Param.b_gain);
        }

        // --- 5. Tonemap ---
        if (ImGui::CollapsingHeader("4. Tonemap - Detailed Tuning", ImGuiTreeNodeFlags_DefaultOpen)) {
            float lamda = (float)gISPparamLinear.tonemap_Param.lamda;
            if (ImGui::InputFloat("Lambda", &lamda, 0.01f, 0.1f, "%.4f")) gISPparamLinear.tonemap_Param.lamda = (double)lamda;

            float tauR = (float)gISPparamLinear.tonemap_Param.tauR;
            if (ImGui::InputFloat("TauR", &tauR)) gISPparamLinear.tonemap_Param.tauR = (double)tauR;

            float p_val = (float)gISPparamLinear.tonemap_Param.P;
            if (ImGui::InputFloat("P (Exposure)", &p_val, 0.00001f, 0.0001f, "%.7f")) gISPparamLinear.tonemap_Param.P = (double)p_val;

            float etaF = (float)gISPparamLinear.tonemap_Param.etaF;
            if (ImGui::InputFloat("EtaF", &etaF)) gISPparamLinear.tonemap_Param.etaF = (double)etaF;

            float etaC = (float)gISPparamLinear.tonemap_Param.etaC;
            if (ImGui::InputFloat("EtaC", &etaC)) gISPparamLinear.tonemap_Param.etaC = (double)etaC;

            float bpcMax = (float)gISPparamLinear.tonemap_Param.BPCmax;
            if (ImGui::InputFloat("BPC Max", &bpcMax)) gISPparamLinear.tonemap_Param.BPCmax = (double)bpcMax;

            float bpcMin = (float)gISPparamLinear.tonemap_Param.BPCmin;
            if (ImGui::InputFloat("BPC Min", &bpcMin)) gISPparamLinear.tonemap_Param.BPCmin = (double)bpcMin;

            float n_sigma = (float)gISPparamLinear.tonemap_Param.noise_sigma;
            if (ImGui::InputFloat("Noise Sigma", &n_sigma)) gISPparamLinear.tonemap_Param.noise_sigma = (double)n_sigma;

            ImGui::InputFloat("Shadow Boost", &gISPparamLinear.tonemap_Param.shadow_boost);
            ImGui::InputFloat("Brightness Ratio", &gISPparamLinear.tonemap_Param.brightness_ratio);
            ImGui::InputFloat("Contrast K", &gISPparamLinear.tonemap_Param.contrast_k);
            ImGui::InputFloat("Light Compress", &gISPparamLinear.tonemap_Param.light_compress);
            ImGui::InputInt("TMP bit", &gISPparamLinear.tonemap_Param.TMPBit);
        }

        // --- 6. RNR (Raw Noise Reduction) ---
        if (ImGui::CollapsingHeader("5. RNR - Raw Denoise")) {
            bool en = (gISPparamLinear.rnr_Param.enable != 0);
            if (ImGui::Checkbox("Enable RNR", &en)) gISPparamLinear.rnr_Param.enable = en;

            // 使用 InputFloat 替代滑块，步进设为 0.1
            ImGui::InputFloat("H_R (Red)", &gISPparamLinear.rnr_Param.h_r, 0.1f, 1.0f, "%.2f");
            ImGui::InputFloat("H_G (Green)", &gISPparamLinear.rnr_Param.h_g, 0.1f, 1.0f, "%.2f");
            ImGui::InputFloat("H_B (Blue)", &gISPparamLinear.rnr_Param.h_b, 0.1f, 1.0f, "%.2f");
        }
        //--demosaic
        if (ImGui::CollapsingHeader("6. Demosaic")) {
            float m_alpha = (float)gISPparamLinear.demosaic_Param.m_alpha;
            if (ImGui::InputFloat("Alpha", &m_alpha)) gISPparamLinear.demosaic_Param.m_alpha = (double)m_alpha;
            ImGui::InputInt("HV Gap", &gISPparamLinear.demosaic_Param.m_HVgap);
            int tc = gISPparamLinear.demosaic_Param.thr_c, ty = gISPparamLinear.demosaic_Param.thr_y;
            if (ImGui::InputInt("Thr C", &tc)) gISPparamLinear.demosaic_Param.thr_c = (u16)tc;
            if (ImGui::InputInt("Thr Y", &ty)) gISPparamLinear.demosaic_Param.thr_y = (u16)ty;
        }

        // --- 7. CCM ---
        if (ImGui::CollapsingHeader("7. CCM - Color Matrix")) {
            bool en = (gISPparamLinear.ccm_Param.enable != 0);
            if (ImGui::Checkbox("Enable Gamma", &en)) gISPparamLinear.ccm_Param.enable = en;
            for (int i = 0; i < 3; i++) {
                float row[3] = { (float)gISPparamLinear.ccm_Param.sccm[i][0], (float)gISPparamLinear.ccm_Param.sccm[i][1], (float)gISPparamLinear.ccm_Param.sccm[i][2] };
                std::string lbl = "Row " + std::to_string(i);
                if (ImGui::InputFloat3(lbl.c_str(), row, "%.4f")) {
                    gISPparamLinear.ccm_Param.sccm[i][0] = (double)row[0];
                    gISPparamLinear.ccm_Param.sccm[i][1] = (double)row[1];
                    gISPparamLinear.ccm_Param.sccm[i][2] = (double)row[2];
                }
            }
        }
        // --- 7. Gamma Correction ---
        if (ImGui::CollapsingHeader("8. Gamma Correction")) {

            ImGui::InputFloat("Gamma Power", &gISPparamLinear.gamma_Param.gamma_power, 0.01f, 0.1f, "%.2f");
        }
        // --- 8. contrast ---
        if (ImGui::CollapsingHeader("9. contrast")) {

            bool en = (gISPparamLinear.contrast_Param.enable != 0);
            if (ImGui::Checkbox("Enable contrast", &en)) gISPparamLinear.contrast_Param.enable = en;
            int contrastbit = gISPparamLinear.contrast_Param.contrastbit;
            int contraststrength = gISPparamLinear.contrast_Param.ContrastStrength;
            ImGui::InputInt("contrastbit", &contrastbit);
            ImGui::InputInt("contraststrength", &contraststrength);

            gISPparamLinear.contrast_Param.contrastbit=contrastbit;
            gISPparamLinear.contrast_Param.ContrastStrength=contraststrength;
        }
        // --- 8. Sharpen ---
        if (ImGui::CollapsingHeader("10. Sharpening")) {
            bool en = (gISPparamLinear.sharpen_Param.enable != 0);
            if (ImGui::Checkbox("Enable Sharpen", &en)) gISPparamLinear.sharpen_Param.enable = en;
            ImGui::InputFloat("Gain Fine", &gISPparamLinear.sharpen_Param.gain_fine);
            ImGui::InputFloat("Gain mid", &gISPparamLinear.sharpen_Param.gain_mid);
            ImGui::InputFloat("coring_high", &gISPparamLinear.sharpen_Param.coring_high);
            ImGui::InputInt("UnderShot Limit", &gISPparamLinear.sharpen_Param.under_shot_limit);
            ImGui::InputInt("OverShot Limit", &gISPparamLinear.sharpen_Param.over_shot_limit);
            int sharpenbit = gISPparamLinear.sharpen_Param.sharpenbit;
            ImGui::InputInt("sharpen bit", &sharpenbit);
            gISPparamLinear.sharpen_Param.sharpenbit = sharpenbit;

        }
        ImGui::Separator();
    }

  

    // --- Buttons ---
    static std::string status = "Ready";
    //if (ImGui::Button("INIT PIPE", ImVec2(120, 40))) {
    //    status = (PipeInit() == 0) ? "Init Success" : "Init Failed";
   // }
    ImGui::SameLine();
    if (ImGui::Button("RUN PIPE", ImVec2(120, 40))) {
        gISPparamHDRcombine.rawinfo.ispmode = g_current_mode;
        gISPparamLinear.rawinfo.ispmode = g_current_mode;
        status = "processing";
        PipeInit();
        status = (PipeRun() == 0) ? "Run Success" : "Run Failed";
    }
    ImGui::Text("Status: %s", status.c_str());

    ImGui::End();
}

// Main code
int main(int, char**)
{
    printf("****************************************Raw 2 JPG**************************************************\n");
    printf("****************************************By fyj 20260402********************************************\n");
    SetProcessDPIAware();
   
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return -1;

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ISP_Class", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"RAW2JPG  ^.^", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // 1. 同步尺寸
        RECT rect;
        ::GetClientRect(hwnd, &rect);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

        // 2. 开始新帧
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 3. 开启全屏 UI 容器窗口
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);


        // 4. 调用业务 UI 逻辑
        // 【注意】：请确保 RenderISPParameterWindow() 内部 不要 再写 ImGui::Begin 和 End 了！
        // 只需要写里面的按钮、滑块、CollapsingHeader 即可。
        RenderISPParameterWindow();

        // 5. 在这里补上窗口的结束
      

        // 6. 渲染绘制
        ImGui::Render();
        const float clear_color[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // 释放资源
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
            // 1. 销毁旧的 RTV
            if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }

            // 2. 调整交换链（0 表示自动跟随窗口大小）
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);

            // 3. 重新创建 RTV
            ID3D11Texture2D* pBackBuffer;
            g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
#endif