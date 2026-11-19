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

    std::puts("project.json tests passed");
    return 0;
}
