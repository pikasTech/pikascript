from PikaObj import *


class cJSON(TinyObj):
    cJSON_Invalid: int
    cJSON_False: int
    cJSON_True: int
    cJSON_NULL: int
    cJSON_Number: int
    cJSON_String: int
    cJSON_Array: int
    cJSON_Object: int
    cJSON_Raw: int
    def print(self) -> str: ...
    def parse(self, value: str): ...
    def __del__(self): ...
    def __init__(self): ...
    def getObjectItem(self, string: str) -> cJSON: ...
    def getArrayItem(self, index: int) -> cJSON: ...
    def getArraySize(self) -> int: ...
    def getType(self) -> int: ...
    def getNext(self) -> cJSON: ...
    def getPrev(self) -> cJSON: ...
    def getChild(self) -> cJSON: ...
    def getValueString(self) -> str: ...
    def getValueInt(self) -> int: ...
    def getValueDouble(self) -> float: ...
    def getString(self) -> str: ...
    def getValue(self) -> any: ...
    def isInvalid(self) -> int: ...
    def isFalse(self) -> int: ...
    def isTrue(self) -> int: ...
    def isBool(self) -> int: ...
    def isNull(self) -> int: ...
    def isNumber(self) -> int: ...
    def isString(self) -> int: ...
    def isArray(self) -> int: ...
    def isObject(self) -> int: ...
    def isRaw(self) -> int: ...
