from __future__ import annotations
from dataclasses import dataclass
from typing import List, Tuple
import importlib

# ネイティブ拡張の読み込み
_core = importlib.import_module("._dxgi_probe", package=__name__)

@dataclass(frozen=True)
class OutputInfo:
    adapter_idx: int
    output_idx:  int
    device_name: str
    width:       int
    height:      int
    primary:     bool

def enumerate_outputs() -> List[OutputInfo]:
    """全モニター情報を列挙して返す。"""
    return [OutputInfo(o.adapter_idx, o.output_idx, o.device_name,
                       o.width, o.height, o.primary)
            for o in _core.enumerate()]

def display_name_to_index(name: str) -> Tuple[int, int]:
    """
    '\\\\.\\DISPLAYn' → (adapter_idx, output_idx) へ変換。
    見つからなければ LookupError。
    """
    lname = name.lower()
    for o in _core.enumerate():
        if o.device_name.lower() == lname:
            return o.adapter_idx, o.output_idx
    raise LookupError(f"{name} not found in DXGI enumeration")
