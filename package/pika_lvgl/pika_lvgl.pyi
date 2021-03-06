from PikaObj import *


def __init__(): ...

class EVENT(TinyObj):
    ALL: int
    PRESSED: int
    PRESSING: int
    PRESS_LOST: int
    SHORT_CLICKED: int
    LONG_PRESSED: int
    LONG_PRESSED_REPEAT: int
    CLICKED: int
    RELEASED: int
    SCROLL_BEGIN: int
    SCROLL_END: int
    SCROLL: int
    GESTURE: int
    KEY: int
    FOCUSED: int
    DEFOCUSED: int
    LEAVE: int
    HIT_TEST: int
    COVER_CHECK: int
    REFR_EXT_DRAW_SIZE: int
    DRAW_MAIN_BEGIN: int
    DRAW_MAIN: int
    DRAW_MAIN_END: int
    DRAW_POST_BEGIN: int
    DRAW_POST: int
    DRAW_POST_END: int
    DRAW_PART_BEGIN: int
    DRAW_PART_END: int
    VALUE_CHANGED: int
    INSERT: int
    REFRESH: int
    READY: int
    CANCEL: int
    DELETE: int
    CHILD_CHANGED: int
    CHILD_CREATED: int
    CHILD_DELETED: int
    SCREEN_UNLOAD_START: int
    SCREEN_LOAD_START: int
    SCREEN_LOADED: int
    SCREEN_UNLOADED: int
    SIZE_CHANGED: int
    STYLE_CHANGED: int
    LAYOUT_CHANGED: int
    GET_SELF_SIZE: int
    PREPROCESS: int
    def __init__(self): ...


class ALIGN(TinyObj):
    DEFAULT: int
    TOP_LEFT: int
    TOP_MID: int
    TOP_RIGHT: int
    BOTTOM_LEFT: int
    BOTTOM_MID: int
    BOTTOM_RIGHT: int
    LEFT_MID: int
    RIGHT_MID: int
    CENTER: int
    OUT_TOP_LEFT: int
    OUT_TOP_MID: int
    OUT_TOP_RIGHT: int
    OUT_BOTTOM_LEFT: int
    OUT_BOTTOM_MID: int
    OUT_BOTTOM_RIGHT: int
    OUT_LEFT_TOP: int
    OUT_LEFT_MID: int
    OUT_LEFT_BOTTOM: int
    OUT_RIGHT_TOP: int
    OUT_RIGHT_MID: int
    OUT_RIGHT_BOTTOM: int
    def __init__(self): ...


class PALETTE(TinyObj):
    RED: int
    PINK: int
    PURPLE: int
    DEEP_PURPLE: int
    INDIGO: int
    BLUE: int
    LIGHT_BLUE: int
    CYAN: int
    TEAL: int
    GREEN: int
    LIGHT_GREEN: int
    LIME: int
    YELLOW: int
    AMBER: int
    ORANGE: int
    DEEP_ORANGE: int
    BROWN: int
    BLUE_GREY: int
    GREY: int
    NONE: int
    def __init__(self): ...


class OPA(TinyObj):
    TRANSP: int
    COVER: int
    def __init__(self): ...


class ANIM(TinyObj):
    OFF: int
    ON: int
    def __init__(self): ...

class STATE(TinyObj):
    def __init__(self): ...


class lv_event(TinyObj):
    def get_code(self) -> int: ...


class lv_color_t(TinyObj):
    ...


def palette_lighten(p: int, lvl: int) -> lv_color_t: ...
def palette_main(p: int) -> lv_color_t: ...


class style_t(TinyObj):
    def __init__(self): ...
    def init(self): ...
    def set_radius(self, radius: int): ...
    def set_bg_opa(self, opa: int): ...
    def set_bg_color(self, color: lv_color_t): ...
    def set_outline_width(self, w: int): ...
    def set_outline_color(self, color: lv_color_t): ...
    def set_outline_pad(self, pad: int): ...


class lv_obj(TinyObj):
    def center(self): ...
    def set_size(self, size_x: int, size_y: int): ...
    def align(self, align: int, x_ofs: int, y_ofs: int): ...
    def set_hight(self, h: int): ...
    def update_layout(self): ...
    def set_width(self, w: int): ...
    def add_state(self, state: int): ...
    def add_event_cb(self, event_cb: any, filter: int, user_data: pointer): ...
    def add_style(self, style: style_t, selector: int): ...


def obj(parent: lv_obj) -> lv_obj: ...


class arc(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_end_angle(self, angle: int): ...
    def set_bg_angles(self, start: int, end: int): ...
    def set_angles(self, start: int, end: int): ...


class bar(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_value(self, value: int, anim: int): ...


class btn(lv_obj):
    def __init__(self, parent: lv_obj): ...


class checkbox(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_text(self, txt: str): ...


class dropdown(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_options(self, options: str): ...


class label(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_text(self, txt: str): ...
    def set_long_mode(self, mode: int): ...
    def set_recolor(self, en: int): ...
    def set_style_text_align(self, value: int, selector: int): ...


class roller(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_options(self, options: str, mode: int): ...
    def set_visible_row_count(self, row_cnt: int): ...


class slider(lv_obj):
    def __init__(self, parent: lv_obj): ...


class switch(lv_obj):
    def __init__(self, parent: lv_obj): ...


class table(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_cell_value(self, row: int, col: int, txt: str): ...


class textarea(lv_obj):
    def __init__(self, parent: lv_obj): ...
    def set_one_line(en: int): ...


def scr_act() -> lv_obj: ...
