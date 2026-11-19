#include <cassert>
#include <cstdio>

#include "io/ProjectJson.h"

using namespace nyaa::domain::settings;

int main()
{
    const std::string json = R"({
      "title": "test",
      "general": { "properties": {
        "bloom":   { "type": "bool",   "text": "Bloom", "value": true, "order": 2 },
        "fov":     { "type": "slider", "text": "FOV",   "value": 0.42, "min": 0.1, "max": 1.5, "step": 0.05, "order": 1 },
        "tint":    { "type": "color",  "text": "Tint",  "value": "#FF8800", "order": 3 },
        "mode":    { "type": "combo",  "text": "Mode",  "value": 1,
                     "options": [ {"value": 0, "label": "day"}, {"value": 1, "label": "night"} ], "order": 4 },
        "caption": { "text": "A <b>label</b>", "order": 5 },
        "script":  { "type": "weird",  "text": "ignored", "order": 6 }
      } }
    })";

    const auto descs = nyaa::io::parseProperties(json);
    assert(descs.size() == 5); // the unknown type is skipped

    assert(descs[0].key == "fov");     // sorted by order
    assert(descs[0].kind == Kind::Slider);
    assert(descs[0].number == 0.42);
    assert(descs[0].range.max == 1.5);

    assert(descs[1].key == "bloom");
    assert(descs[1].kind == Kind::Bool);
    assert(descs[1].boolean);

    assert(descs[2].kind == Kind::Color);
    assert(descs[2].color.r == 1.0);
    assert(descs[2].color.g > 0.52 && descs[2].color.g < 0.54);

    assert(descs[3].kind == Kind::Combo);
    assert(descs[3].options.size() == 2);
    assert(descs[3].options[1].second == "night");

    assert(descs[4].kind == Kind::Text);
    assert(descs[4].label == "A label");  // HTML stripped

    // ---- Java PropertyParser parity (docs/TODO.md A1-A5) -------------------
    const std::string loose = R"({
      "general": { "properties": {
        "b_str":  { "type": "bool",   "value": "true", "order": 1 },
        "b_one":  { "type": "bool",   "value": "1",    "order": 2 },
        "b_num":  { "type": "bool",   "value": 1,      "order": 3 },
        "s_bare": { "type": "slider", "order": 4 },
        "s_str":  { "type": "slider", "value": "12.5", "min": "0", "max": "50", "step": "0.5", "order": 5 },
        "c_str":  { "type": "combo",  "value": "2", "order": 6,
                    "options": [ {"label": "Original ads", "value": "1"},
                                 {"label": "Expanded ads", "value": "2"},
                                 {"text":  "No ads",       "value": 3} ] },
        "t_num":  { "type": "textinput", "value": 42,   "order": 7 },
        "t_str":  { "type": "textinput", "value": "hi", "order": 8 }
      } }
    })";

    const auto d = nyaa::io::parseProperties(loose);
    assert(d.size() == 8);

    // A1: bool accepts "true", "1" and numbers, like parseBoolean()
    assert(d[0].boolean);
    assert(d[1].boolean);
    assert(d[2].boolean);

    // A2: slider defaults 0 / 0 / 100 / 1, and numbers may be strings
    assert(d[3].number == 0.0);
    assert(d[3].range.min == 0.0);
    assert(d[3].range.max == 100.0);
    assert(d[3].range.step == 1.0);
    assert(d[4].number == 12.5);
    assert(d[4].range.max == 50.0);
    assert(d[4].range.step == 0.5);

    // A3 + A4: combo value and option values may be numeric strings
    assert(d[5].number == 2.0);
    assert(d[5].options.size() == 3);
    assert(d[5].options[0].first == 1 && d[5].options[0].second == "Original ads");
    assert(d[5].options[1].first == 2);
    assert(d[5].options[2].first == 3 && d[5].options[2].second == "No ads");

    // A5: textinput keeps any value as text
    assert(d[6].text == "42");
    assert(d[7].text == "hi");

    // Java skips a combo whose option values are not numbers ("en", "dark1"):
    // Integer.parseInt throws and parseFromWppr drops the property.
    const auto t = nyaa::io::parseProperties(R"({ "general": { "properties": {
        "language": { "type": "combo", "value": "en",
                      "options": [ {"label": "English", "value": "en"}, {"label": "Deutsch", "value": "de"} ] },
        "keep":     { "type": "bool", "value": true }
    } } })");
    assert(t.size() == 1 && t[0].key == "keep");

    std::puts("project.json tests passed");
    return 0;
}
