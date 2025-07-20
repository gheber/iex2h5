from pathlib import Path

SOURCE_ROOT = Path(__file__).resolve().parents[1]  # assumes script lives in scripts/
LICENSE_DIR = SOURCE_ROOT / "docs/licenses"
OUTPUT_FILE = SOURCE_ROOT / "include/licenses.hpp"

def has_spdx_tag(lines):
    return lines and lines[0].strip().startswith("SPDX-License-Identifier:")

def extract_spdx(lines):
    if has_spdx_tag(lines):
        return lines[0].strip().split(":", 1)[-1].strip()
    return None

def extract_license_content_without_spdx(path: Path) -> str:
    lines = path.read_text(encoding="utf-8").splitlines()
    if has_spdx_tag(lines):
        return "\n".join(lines[1:]).strip()
    return "\n".join(lines).strip()

def validate_and_collect():
    licenses = []
    for file in sorted(LICENSE_DIR.iterdir()):
        if not file.is_file():
            continue

        lines = file.read_text(encoding="utf-8").splitlines()
        name = file.name
        if has_spdx_tag(lines):
            spdx = extract_spdx(lines)
            content = extract_license_content_without_spdx(file)
            licenses.append((name, spdx, content))
            print(f"✓ {name} has SPDX: {spdx}")
        else:
            print(f"✗  {name} is missing SPDX-License-Identifier")

    return licenses

def write_cpp_header(licenses):
    OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write("""#pragma once
#include <string>
#include <vector>
#include <tuple>

/**
 * @file third_party_licenses.hpp
 * @brief List of third-party license metadata used in this project.
 *
 * Each entry is a tuple of:
 *   - name (e.g. "hdf5")
 *   - SPDX identifier (e.g. "BSD-3-Clause")
 *   - full license text (multi-line, raw string)
 *
 * This file is auto-generated. Do not edit manually.
 */

namespace licenses {
const std::vector<std::tuple<std::string, std::string, std::string>> thirdparty = {
""")
        for i, (name, spdx, content) in enumerate(licenses):
            comma = "," if i < len(licenses) - 1 else ""
            separator = f'\n\n/*== {name.upper()} {spdx.upper()} =========================================================*/'
            out.write(f'{separator} \nstd::make_tuple("{name}", "{spdx}", R"({content})"){comma}\n')

        out.write("};\n} // namespace licenses\n")

    print(f"\n✓ Header generated: {OUTPUT_FILE}")

def main():
    licenses = validate_and_collect()
    write_cpp_header(licenses)

if __name__ == "__main__":
    main()
