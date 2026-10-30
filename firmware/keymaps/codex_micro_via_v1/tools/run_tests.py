import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET


KEYMAP = Path(__file__).resolve().parents[1]
REPO = KEYMAP.parents[2]
QMK = Path(os.environ.get("QMK_HOME", Path.home() / "qmk_firmware"))


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 1
    cursor = opening + 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def compile_and_run(directory, name, sources, defines=()):
    binary = directory / name
    subprocess.run([
        "clang", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-DCODEX_MICRO_HOST_TEST",
        *defines,
        '-DQMK_KEYBOARD_H="test_qmk.h"', "-DPOC_DUAL_USB", "-DPROTOCOL_CHIBIOS",
        "-I", str(directory), "-I", str(KEYMAP), "-I", str(KEYMAP / "tools"),
        "-I", str(QMK / "quantum"), "-I", str(QMK / "quantum/keymap_extras"),
        "-I", str(QMK / "quantum/sequencer"),
        "-I", str(QMK / "drivers/oled"), "-I", str(QMK / "platforms"),
        *map(str, sources), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)


def check_via_layout():
    definition = json.loads((REPO / "firmware/via/kb16_codex_via.json").read_text())
    positions = {}
    vertical = 0
    for row in definition["layouts"]["keymap"]:
        horizontal = 0
        for item in row:
            if isinstance(item, dict):
                horizontal += item.get("x", 0)
                vertical += item.get("y", 0)
            else:
                positions[item.split("\n")[0]] = (horizontal, vertical)
                horizontal += 1
        vertical += 1
    for row in range(4):
        for col in range(4):
            assert positions[f"{row},{col}"] == (col, row)
    assert positions["0,4"] == (4.75, 0)
    assert positions["1,4"] == (6, 0)
    assert positions["2,4"] == (5.375, 2.5)
    assert len(positions) == 19
    print("PASS: VIA KLE relative offsets produce the physical 16 keys and three knobs", flush=True)
    return positions


def check_codex_layout_artwork(positions):
    source = (KEYMAP / "keymap.c").read_text()
    body = re.search(r"\[_CODEX\]\s*=\s*LAYOUT\((.*?)\),", source, re.S).group(1)
    defaults = [entry.strip() for entry in re.sub(r"/\*.*?\*/", "", body, flags=re.S).split(",") if entry.strip()]
    assert len(defaults) == 19
    encoder_source = source[source.index("const uint16_t PROGMEM encoder_map"):]
    body = re.search(r"\[_CODEX\]\s*=\s*\{(.*?)\},", encoder_source, re.S).group(1)
    turns = re.findall(r"ENCODER_CCW_CW\((\w+),\s*(\w+)\)", body)
    custom = {"0,0": "1", "0,3": "2", "0,4": "3", "1,4": "4", "3,0": "5"}
    namespace = {"svg": "http://www.w3.org/2000/svg"}
    for language in ("zh", "en"):
        artwork = ET.parse(REPO / f"docs/assets/kb16-codex-layout-{language}.svg").getroot()
        keys = artwork.findall('svg:g[@data-control="key"]', namespace)
        knobs = artwork.findall('svg:g[@data-control="encoder"]', namespace)
        assert len(keys) == 16 and len(knobs) == 3
        for group in keys + knobs:
            matrix = group.attrib["data-matrix"]
            row, column = map(int, matrix.split(","))
            expected = defaults[row * 5 + column]
            assert group.attrib["data-keycode"] == expected
            assert group.attrib.get("data-via-slot") == custom.get(matrix)
            horizontal, vertical = positions[matrix]
            if group in keys:
                rect = group.find("svg:rect", namespace)
                assert float(rect.attrib["x"]) == 24 + horizontal * 64
                assert float(rect.attrib["y"]) == 90 + vertical * 64
                assert int(group.attrib["data-led"]) == row * 4 + column
            else:
                circle = group.find("svg:circle", namespace)
                assert float(circle.attrib["cx"]) == 24 + horizontal * 64 + 30
                assert float(circle.attrib["cy"]) == 90 + vertical * 64 + 30
                index = int(group.attrib["data-index"])
                assert (group.attrib["data-ccw"], group.attrib["data-cw"]) == turns[index]
    print("PASS: both SVG layouts match all five direct VIA keys, native controls, LED indexes and VIA physical coordinates", flush=True)


def check_power_config():
    for timeout in (0, 120000):
        subprocess.run([
            "clang", "-x", "c", "-fsyntax-only", "-include", str(KEYMAP / "config.h"),
            "-include", str(KEYMAP / "poc_config.h"),
            f"-DRGB_MATRIX_TIMEOUT={timeout}", "-",
        ], input="""
#ifndef RGB_MATRIX_SLEEP
#error USB suspend must turn off keyboard lighting as well as the OLED
#endif
_Static_assert(OLED_TIMEOUT == 0 && RGB_MATRIX_TIMEOUT == 0, "native idle timers must not bypass shared host-aware power policy");
_Static_assert(POC_IDLE_SLEEP_MS == 3600000UL, "shared idle fallback must be one hour");
_Static_assert(POC_VIA_WINDOW_MS == 10000, "VIA startup window must be ten seconds");
_Static_assert(POC_CODEX_LAYER == 0 && POC_NORMAL_FIRST_LAYER == 1 && POC_VIA_LAYER_COUNT == 4, "Codex must be the first of four VIA pages");
#ifdef POC_LAYER_KEY_HOLD_MS
#error No fixed long-hold exit is allowed
#endif
""", text=True, check=True)
    print("PASS: shared one-hour policy owns OLED/RGB timeouts, without native timers bypassing heartbeat renewal; USB suspend remains enabled", flush=True)


with tempfile.TemporaryDirectory(prefix="kb16-tests-") as scratch:
    directory = Path(scratch)
    source = (KEYMAP / "keymap.c").read_text()
    enum = re.search(r"enum poc_keycodes \{.*?\n\};", source, re.S).group()
    extracted = [enum, "#define POC_LAYER_COUNT POC_VIA_LAYER_COUNT"]
    for stamp in ("POC_LAYER_STAMP", "POC_FOUR_LAYER_STAMP", "POC_CUSTOM_KEYS_STAMP", "POC_CODEX_L0_STAMP"):
        extracted.append(re.search(rf"#define {stamp} .*", source).group())
    for signature in (
        "static bool poc_codex_custom_position(", "static uint16_t poc_codex_custom_keycode(",
        "uint8_t poc_keymap_base_layer(", "uint16_t keymap_key_to_keycode(",
        "static void codex_send_action(", "bool process_record_user(", "void matrix_scan_user(",
        "static void poc_check_layout_stamp(",
        "static uint16_t poc_remap_legacy_codex_keycode(", "static void poc_migrate_four_layer_layout(",
        "static void poc_migrate_codex_custom_keys(", "static uint16_t poc_remap_shifted_layer_keycode(",
        "static void poc_migrate_codex_l0_layout(",
    ):
        extracted.append(function(source, signature))
    extracted.append(source[source.index("static bool codex_rgb_active;"):source.index("static void codex_rgb_init(void) {")])
    extracted.append(function(source, "static void codex_rgb_init(void) {"))
    extracted.append(function(source, "static void update_codex_rgb("))
    extracted.append(function(source, "layer_state_t layer_state_set_user("))
    extracted.append(function(source, "layer_state_t default_layer_state_set_user("))
    extracted.append(function(source, "bool rgb_matrix_indicators_advanced_user("))
    extracted.append(function(source, "void keyboard_post_init_user("))
    (directory / "keymap_under_test.h").write_text("\n".join(extracted))
    layer_source = (REPO / "firmware/qmk-core-patches/quantum/action_layer.c").read_text()
    layers = [function(layer_source, signature) for signature in (
        "void layer_move(", "void layer_on(", "void layer_off(", "void layer_invert(", "void layer_xor(",
        "void update_source_layers_cache_impl(", "uint8_t read_source_layers_cache_impl(",
        "void update_source_layers_cache(", "uint8_t read_source_layers_cache(", "uint8_t layer_switch_get_layer(",
    )]
    layers.append(function((QMK / "quantum/quantum.c").read_text(), "uint16_t get_event_keycode("))
    (directory / "qmk_layers_under_test.h").write_text("\n".join(layers))
    board_source = (QMK / "keyboards/doio/kb16/kb16.c").read_text()
    mode_source = (KEYMAP / "poc_mode.c").read_text()
    (directory / "startup_under_test.h").write_text("\n".join([
        function(board_source, "oled_rotation_t oled_init_kb("),
        function(board_source, "bool oled_task_kb("),
        *[function(mode_source, signature) for signature in (
            "void poc_mode_start_window(", "void poc_mode_note_via_command(",
            "void poc_mode_task(", "bool poc_mode_via_waiting(",
        )],
    ]))

    via_source = (QMK / "quantum/via.h").read_text()
    via_enum = re.search(r"enum via_command_id \{.*?\n\};", via_source, re.S).group()
    (directory / "via.h").write_text("#pragma once\n" + via_enum + "\nbool via_command_kb(uint8_t *data, uint8_t length);\n")
    (directory / "qmk_via_key_under_test.h").write_text(function((QMK / "quantum/via.c").read_text(), "bool process_record_via("))

    usb_source = (REPO / "firmware/qmk-core-patches/tmk_core/protocol/chibios/usb_main.c").read_text()
    (directory / "usb_input_under_test.h").write_text("\n".join(function(usb_source, signature) for signature in (
        "void send_keyboard(", "void send_nkro(", "void send_mouse(", "void send_extra(",
    )))
    (directory / "usb_task_under_test.h").write_text(function(usb_source, "void raw_hid_task("))
    setup_start = usb_source.index("case RAW_INTERFACE: {", usb_source.index("case HID_REQ_SetReport:"))
    setup_end = usb_source.index("#endif", setup_start)
    (directory / "usb_setup_under_test.h").write_text(usb_source[setup_start:setup_end])

    descriptors = (REPO / "firmware/qmk-core-patches/tmk_core/protocol/usb_descriptor.c").read_text()
    hid_start = descriptors.index("case HID_DTYPE_HID:")
    raw_start = descriptors.index("case RAW_INTERFACE:", hid_start)
    raw_end = descriptors.index("break;", descriptors.index("Address = &ConfigurationDescriptor.Raw_HID;", raw_start)) + len("break;")
    (directory / "hid_descriptor_under_test.h").write_text(descriptors[raw_start:raw_end])

    compile_and_run(directory, "lighting", [KEYMAP / "codex_micro_protocol.c", KEYMAP / "poc_power.c", KEYMAP / "tools/lighting_test.c"])
    compile_and_run(directory, "reliability", [
        KEYMAP / "codex_micro_protocol.c", KEYMAP / "poc_power.c", KEYMAP / "poc_usb.c", KEYMAP / "poc_via.c",
        KEYMAP / "oled_icons.c", KEYMAP / "oled_input.c", KEYMAP / "tools/reliability_test.c",
    ])
    compile_and_run(directory, "oled", [
        KEYMAP / "codex_micro_protocol.c", KEYMAP / "poc_power.c", KEYMAP / "oled_icons.c", KEYMAP / "oled_input.c",
        KEYMAP / "tools/oled_test.c",
    ])
    compile_and_run(directory, "storage_layout", [KEYMAP / "tools/storage_layout_test.c"])
    compile_and_run(directory, "power", [KEYMAP / "poc_power.c", KEYMAP / "tools/power_test.c"])
    compile_and_run(directory, "startup", [KEYMAP / "tools/startup_test.c"])
    positions = check_via_layout()
    check_codex_layout_artwork(positions)
    check_power_config()
