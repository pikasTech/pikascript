# api
from PikaObj import *


class MemChecker(TinyObj):
    def max(self): ...
    def now(): ...
    def getMax() -> float: ...
    def getNow() -> float: ...
    def resetMax(): ...


class SysObj(BaseObj):
    def type(arg: any): ...
    def remove(argPath: str): ...
    def int(arg: any) -> int: ...
    def float(arg: any) -> float: ...
    def str(arg: any) -> str: ...
    def iter(arg: any) -> any: ...
    def range(a1: int, a2: int) -> any: ...
    def __set__(obj: any, key: any, val: any, obj_str: str): ...
    def __get__(obj: any, key: any) -> any: ...
    def print(self, *val): ...


class RangeObj(TinyObj):
    def __next__() -> any:
        pass


class StringObj(TinyObj):
    def __next__() -> any:
        pass


class PikaObj(TinyObj):
    pass
