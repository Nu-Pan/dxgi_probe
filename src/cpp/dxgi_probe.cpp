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

// hr �����s�n�R�[�h�������ꍇ�A��O�𓊂���
inline void throw_if_failed(HRESULT hr, std::string_view ctx)
{
    if(FAILED(hr))
    {
        throw runtime_error(
            format("{} failed: 0x{:08X}", ctx, static_cast<unsigned>(hr))
        );
    }    
}

// DXGI ouptut ���\����
struct OutputInfo {
    int         adapter_index;
    int         output_index;
    string      device_name;      // "\\\\.\\DISPLAY1"
    int         width;            // �s�N�Z��
    int         height;
    bool        primary;          // �v���C�}����
};

// CoInitializeEz �� RAII �ɂ��邽�߂̃N���X
struct CoInitializeExRAII
{

    // �R���X�g���N�^
    CoInitializeExRAII()
    {
        // COM �� 2 ��ڈȍ~ RPC_E_CHANGED_MODE ��Ԃ����Ƃ�����̂œ��ʈ���
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(hr != RPC_E_CHANGED_MODE)
        {
            throw_if_failed(hr, "CoInitializeEx");
        }
    }

    // �f�X�g���N�^
    ~CoInitializeExRAII()
    {
        CoUninitialize();
    }

};

// DXGI output ����񋓂���
static vector<OutputInfo> enumerate_outputs()
{

    // COM ������ (���d�Ăяo��OK)
    CoInitializeExRAII co_initialize_ex_raii;

    // �t�@�N�g���[�𐶐�
    ComPtr<IDXGIFactory1> p_factory;
    {
        const HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&p_factory));
        throw_if_failed(hr, "CreateDXGIFactory1");
    }

    // Adapter �𑖍�
    vector<OutputInfo> enumerated_outputs;
    for (UINT adapter_index = 0; /*nop*/; ++adapter_index) {

        // Adapter ���擾
        ComPtr<IDXGIAdapter1> p_adapter;
        {
            const HRESULT hr = p_factory->EnumAdapters1(adapter_index, &p_adapter);
            if(hr == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            throw_if_failed(hr, "EnumAdapters1");
        }

        // Output �𑖍�
        for (UINT output_index = 0; /*nop*/; ++output_index) {

            // Output ���擾
            ComPtr<IDXGIOutput> p_output;
            {
                const HRESULT hr = p_adapter->EnumOutputs(output_index, &p_output);
                if(hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }
                throw_if_failed(hr, "EnumOutputs");
            }

            // Desc ���擾            
            DXGI_OUTPUT_DESC output_desc{};
            {
                p_output->GetDesc(&output_desc);
            }

            // �f�o�C�X���� string �ɕϊ�
            const wstring device_name_wide(output_desc.DeviceName);
            const string device_name(
                device_name_wide.begin(), device_name_wide.end()
            );

            // ���j�^�[�����擾
            const RECT rc = output_desc.DesktopCoordinates;
            MONITORINFO mi{sizeof(MONITORINFO)};
            GetMonitorInfo(output_desc.Monitor, &mi);

            // �ԋp�p�I�u�W�F�N�g���\�z
            const OutputInfo info = {
                static_cast<int>(adapter_index),
                static_cast<int>(output_index),
                device_name,
                rc.right  - rc.left,
                rc.bottom - rc.top,
                (mi.dwFlags & MONITORINFOF_PRIMARY) != 0
            };

            // �ԋp�p���X�g�ɒǉ�
            enumerated_outputs.push_back(info);

        }
    }

    // ����I��
    return enumerated_outputs;

}

/* ---------- pybind11 �o�C���f�B���O ---------- */
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
