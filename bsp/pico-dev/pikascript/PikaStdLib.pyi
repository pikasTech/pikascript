from PikaObj import *


class MemChecker(TinyObj):
    def max(self): ...
    def now(self): ...
    def getMax(self) -> float: ...
    def getNow(self) -> float: ...
    def resetMax(self): ...


class SysObj(BaseObj):
    def type(self, arg: any): ...
    def remove(self, argPath: str): ...
    def int(self, arg: any) -> int: ...
    def float(self, arg: any) -> float: ...
    def str(self, arg: any) -> str: ...
    def iter(self, arg: any) -> any: ...
    def range(self, a1: int, a2: int) -> any: ...
    def print(self, *val): ...
    def printNoEnd(self, val: any): ...
    def __set__(self, obj: any, key: any, val: any, obj_str: str): ...
    def __get__(self, obj: any, key: any) -> any: ...
    def __slice__(self, obj: any, start: any, end: any, step: int) -> any: ...
    def len(self, arg: any) -> int: ...
    def list(self) -> any: ...
    def dict(self) -> any: ...
    def hex(self, val: int) -> str: ...
    def ord(self, val: str) -> int: ...
    def chr(self, val: int) -> str: ...
    def bytes(self, val: any) -> bytes: ...
    def cformat(self, fmt: str, *var) -> str: ...
    def id(self, obj: any) -> int: ...


class RangeObj(TinyObj):
    def __next__(self) -> any: ...


class StringObj(TinyObj):
    def __next__(self) -> any: ...


class PikaObj(TinyObj):
    ...
