#include <dxgi1_6.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

/* ---------- 返却用構造体 ---------- */
struct OutputInfo {
    int         adapter_idx;
    int         output_idx;
    std::string device_name;      // "\\\\.\\DISPLAY1"
    int         width;            // ピクセル
    int         height;
    bool        primary;          // プライマリか
};

/* ---------- 列挙処理 ---------- */
static std::vector<OutputInfo> enumerate_outputs()
{
    std::vector<OutputInfo> list;

    /* COM 初期化 (多重呼び出しOK) */
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool need_uninit = SUCCEEDED(hr);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        throw std::runtime_error("CoInitializeEx failed");

    IDXGIFactory1* factory = nullptr;
    hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                            reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || !factory)
        throw std::runtime_error("CreateDXGIFactory1 failed");

    for (UINT ai = 0;; ++ai) {
        IDXGIAdapter1* adapter = nullptr;
        if (factory->EnumAdapters1(ai, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        for (UINT oi = 0;; ++oi) {
            IDXGIOutput* out_raw = nullptr;
            if (adapter->EnumOutputs(oi, &out_raw) == DXGI_ERROR_NOT_FOUND)
                break;

            IDXGIOutput6* output = nullptr;
            out_raw->QueryInterface(__uuidof(IDXGIOutput6),
                                    reinterpret_cast<void**>(&output));
            out_raw->Release();
            if (!output) continue;

            DXGI_OUTPUT_DESC1 desc{};
            output->GetDesc1(&desc);

            OutputInfo info;
            info.adapter_idx = static_cast<int>(ai);
            info.output_idx  = static_cast<int>(oi);
            info.device_name = std::wstring(desc.DeviceName).begin(),
            info.device_name = std::string(desc.DeviceName,
                                    desc.DeviceName + wcslen(desc.DeviceName));

            RECT rc = desc.DesktopCoordinates;
            info.width  = rc.right  - rc.left;
            info.height = rc.bottom - rc.top;
            info.primary = (desc.Flags & DXGI_OUTPUT_DESC1_FLAG_PRIMARY) != 0;

            list.emplace_back(std::move(info));
            output->Release();
        }
        adapter->Release();
    }
    factory->Release();
    if (need_uninit) CoUninitialize();
    return list;
}

/* ---------- pybind11 バインディング ---------- */
PYBIND11_MODULE(_dxgi_probe, m)
{
    m.doc() = "DXGI adapter/output enumeration (native)";

    py::class_<OutputInfo>(m, "OutputInfo")
        .def_readonly("adapter_idx", &OutputInfo::adapter_idx)
        .def_readonly("output_idx",  &OutputInfo::output_idx)
        .def_readonly("device_name", &OutputInfo::device_name)
        .def_readonly("width",       &OutputInfo::width)
        .def_readonly("height",      &OutputInfo::height)
        .def_readonly("primary",     &OutputInfo::primary);

    m.def("enumerate", &enumerate_outputs,
          "Return a list of OutputInfo for all active outputs");
}
