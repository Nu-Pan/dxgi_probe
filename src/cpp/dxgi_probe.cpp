// std
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>
#include <format>

// windows
#include <dxgi.h>
#include <wrl/client.h>
#include <windows.h>

// pybind11
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// namespace
using namespace std;
using Microsoft::WRL::ComPtr;
namespace py = pybind11;

// hr が失敗系コードだった場合、例外を投げる
inline void throw_if_failed(HRESULT hr, std::string_view ctx)
{
    if(FAILED(hr))
    {
        throw runtime_error(
            format("{} failed: 0x{:08X}", ctx, static_cast<unsigned>(hr))
        );
    }    
}

// DXGI ouptut 情報構造体
struct OutputInfo {
    int         adapter_index;
    int         output_index;
    string      device_name;      // "\\\\.\\DISPLAY1"
    int         width;            // ピクセル
    int         height;
    bool        primary;          // プライマリか
};

// CoInitializeEz を RAII にするためのクラス
struct CoInitializeExRAII
{

    // コンストラクタ
    CoInitializeExRAII()
    {
        // COM は 2 回目以降 RPC_E_CHANGED_MODE を返すことがあるので特別扱い
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(hr != RPC_E_CHANGED_MODE)
        {
            throw_if_failed(hr, "CoInitializeEx");
        }
    }

    // デストラクタ
    ~CoInitializeExRAII()
    {
        CoUninitialize();
    }

};

// DXGI output 情報を列挙する
static vector<OutputInfo> enumerate_outputs()
{

    // COM 初期化 (多重呼び出しOK)
    CoInitializeExRAII co_initialize_ex_raii;

    // ファクトリーを生成
    ComPtr<IDXGIFactory1> p_factory;
    {
        const HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&p_factory));
        throw_if_failed(hr, "CreateDXGIFactory1");
    }

    // Adapter を走査
    vector<OutputInfo> enumerated_outputs;
    for (UINT adapter_index = 0; /*nop*/; ++adapter_index) {

        // Adapter を取得
        ComPtr<IDXGIAdapter1> p_adapter;
        {
            const HRESULT hr = p_factory->EnumAdapters1(adapter_index, &p_adapter);
            if(hr == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            throw_if_failed(hr, "EnumAdapters1");
        }

        // Output を走査
        for (UINT output_index = 0; /*nop*/; ++output_index) {

            // Output を取得
            ComPtr<IDXGIOutput> p_output;
            {
                const HRESULT hr = p_adapter->EnumOutputs(output_index, &p_output);
                if(hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }
                throw_if_failed(hr, "EnumOutputs");
            }

            // Desc を取得            
            DXGI_OUTPUT_DESC output_desc{};
            {
                p_output->GetDesc(&output_desc);
            }

            // デバイス名を string に変換
            const wstring device_name_wide(output_desc.DeviceName);
            const string device_name(
                device_name_wide.begin(), device_name_wide.end()
            );

            // モニター情報を取得
            const RECT rc = output_desc.DesktopCoordinates;
            MONITORINFO mi{sizeof(MONITORINFO)};
            GetMonitorInfo(output_desc.Monitor, &mi);

            // 返却用オブジェクトを構築
            const OutputInfo info = {
                static_cast<int>(adapter_index),
                static_cast<int>(output_index),
                device_name,
                rc.right  - rc.left,
                rc.bottom - rc.top,
                (mi.dwFlags & MONITORINFOF_PRIMARY) != 0
            };

            // 返却用リストに追加
            enumerated_outputs.push_back(info);

        }
    }

    // 正常終了
    return enumerated_outputs;

}

/* ---------- pybind11 バインディング ---------- */
PYBIND11_MODULE(_dxgi_probe, m)
{
    m.doc() = "DXGI adapter/output enumeration (native)";

    py::class_<OutputInfo>(m, "OutputInfo")
        .def_readonly("adapter_index",  &OutputInfo::adapter_index)
        .def_readonly("output_index",   &OutputInfo::output_index)
        .def_readonly("device_name",    &OutputInfo::device_name)
        .def_readonly("width",          &OutputInfo::width)
        .def_readonly("height",         &OutputInfo::height)
        .def_readonly("primary",        &OutputInfo::primary);

    m.def(
        "enumerate",
        &enumerate_outputs,
        "Return a list of OutputInfo for all active outputs"
    );
}
