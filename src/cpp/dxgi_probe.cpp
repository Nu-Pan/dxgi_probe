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

// DXGI ouptut ���\����
struct OutputInfo {
    int         adapter_index;
    int         output_index;
    string device_name;      // "\\\\.\\DISPLAY1"
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
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(FAILED(hr))
        {
            throw runtime_error("CoInitializeEx Failed");
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
    CoInitializeExRAII coInitializeExRAII;

    // �t�@�N�g���[�𐶐�
    ComPtr<IDXGIFactory1> pFactory;
    {
        const HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
        if(FAILED(hr))
        {
            throw runtime_error("CreateDXGIFactory1 Failed");
        }
    }

    // Adapter �𑖍�
    vector<OutputInfo> enumeratedOutputs;
    for (UINT adapterIndex = 0; /*nop*/; ++adapterIndex) {

        // Adapter ���擾
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

        // Output �𑖍�
        for (UINT outputIndex = 0; /*nop*/; ++outputIndex) {

            // Output ���擾
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

            // Desc ���擾            
            DXGI_OUTPUT_DESC outputDesc{};
            {
                pOutput->GetDesc(&outputDesc);
            }

            // �f�o�C�X���� string �ɕϊ�
            const wstring device_name_wide(outputDesc.DeviceName);
            const string device_name(
                device_name_wide.begin(), device_name_wide.end()
            );

            // ���j�^�[�����擾
            const RECT rc = outputDesc.DesktopCoordinates;
            MONITORINFO mi{sizeof(MONITORINFO)};
            GetMonitorInfo(outputDesc.Monitor, &mi);

            // �ԋp�p�I�u�W�F�N�g���\�z
            const OutputInfo info = {
                static_cast<int>(adapterIndex),
                static_cast<int>(outputIndex),
                device_name,
                rc.right  - rc.left,
                rc.bottom - rc.top,
                (mi.dwFlags & MONITORINFOF_PRIMARY) != 0
            };

            // �ԋp�p���X�g�ɒǉ�
            enumeratedOutputs.push_back(info);

        }
    }

    // ����I��
    return enumeratedOutputs;

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
