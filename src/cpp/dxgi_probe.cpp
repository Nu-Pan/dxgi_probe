// std
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

// windows
#include <dxgi.h>
#include <wrl/client.h>
#include <Windows.h>

// pybind11
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// namespace
using namespace std;
using Microsoft::WRL::ComPtr;
namespace py = pybind11;

// DXGI ouptut 情報構造体
struct OutputInfo {
    int         adapter_index;
    int         output_index;
    string device_name;      // "\\\\.\\DISPLAY1"
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
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(FAILED(hr))
        {
            throw runtime_error("CoInitializeEx Failed");
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
    CoInitializeExRAII coInitializeExRAII;

    // ファクトリーを生成
    ComPtr<IDXGIFactory1> pFactory;
    {
        const HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
        if(FAILED(hr))
        {
            throw runtime_error("CreateDXGIFactory1 Failed");
        }
    }

    // Adapter を走査
    vector<OutputInfo> enumeratedOutputs;
    for (UINT adapterIndex = 0; /*nop*/; ++adapterIndex) {

        // Adapter を取得
        ComPtr<IDXGIAdapter1> pAdapter;
        {
            const HRESULT hr = pFactory->EnumAdapters1(adapterIndex, &pAdapter);
            if(hr == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            else if(FAILED(hr))
            {
                throw runtime_error("EnumAdapters1 Failed");
            }
        }

        // Output を走査
        for (UINT outputIndex = 0; /*nop*/; ++outputIndex) {

            // Output を取得
            ComPtr<IDXGIOutput> pOutput;
            {
                const HRESULT hr = pAdapter->EnumOutputs(outputIndex, &pOutput);
                if(hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }
                else if(FAILED(hr))
                {
                    throw runtime_error("EnumOutputs Failed");
                }
            }

            // Desc を取得            
            DXGI_OUTPUT_DESC outputDesc{};
            {
                pOutput->GetDesc(&outputDesc);
            }

            // デバイス名を string に変換
            const wstring device_name_wide(outputDesc.DeviceName);
            const string device_name(
                device_name_wide.begin(), device_name_wide.end()
            );

            // モニター情報を取得
            const RECT rc = outputDesc.DesktopCoordinates;
            MONITORINFO mi{sizeof(MONITORINFO)};
            GetMonitorInfo(outputDesc.Monitor, &mi);

            // 返却用オブジェクトを構築
            const OutputInfo info = {
                static_cast<int>(adapterIndex),
                static_cast<int>(outputIndex),
                device_name,
                rc.right  - rc.left,
                rc.bottom - rc.top,
                (mi.dwFlags & MONITORINFOF_PRIMARY) != 0
            };

            // 返却用リストに追加
            enumeratedOutputs.push_back(info);

        }
    }

    // 正常終了
    return enumeratedOutputs;

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
