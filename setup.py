from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "dxgi_probe._dxgi_probe",
        ["src/cpp/dxgi_probe.cpp"],
        libraries=["dxgi"],   # DXGI.lib は Windows 標準
        cxx_std=17,
    )
]

setup(
    name="dxgi-probe",
    version="0.1.0",
    packages=["dxgi_probe"],
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},  # pybind11 が include パス解決
    zip_safe=False,
)
