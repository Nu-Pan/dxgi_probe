from __future__ import annotations
from dataclasses import dataclass
from typing import List, Tuple
import importlib

# ネイティブ拡張の読み込み
_core = importlib.import_module("._dxgi_probe", package=__name__)

@dataclass(frozen=True)
class OutputInfo:
    adapter_index: int
    output_index:  int
    device_name: str
    width:       int
    height:      int
    primary:     bool

def enumerate_outputs() -> List[OutputInfo]:
    """全モニター情報を列挙して返す。"""
    return [OutputInfo(o.adapter_index, o.output_index, o.device_name,
                       o.width, o.height, o.primary)
            for o in _core.enumerate()]
