# Phase 13: Document wan-convert Sub-model Scope - Research

**Researched:** 2026-03-17
**Domain:** Documentation / CLI help text / requirements traceability
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- `print_usage()` in `examples/convert/main.cpp`: add inline annotation per `--type` line
  - dit-t2v/dit-i2v/dit-ti2v: append `(loadable by wan_load_model)`
  - vae/t5/clip: append `(reserved: future multi-file loading)`
  - Neutral tone, no warning language
- New file `examples/convert/README.md` (do NOT modify `examples/README.md`)
  - Contents: usage examples, type table (loadable vs reserved), limitations paragraph
- `REQUIREMENTS.md` SAFE-03: keep `[ ]` unchecked, add inline comment explaining partial satisfaction

### Claude's Discretion
- Specific Markdown structure and wording of `examples/convert/README.md`
- Column design of the type table
- Exact wording of the limitations paragraph

### Deferred Ideas (OUT OF SCOPE)
- Implement actual `wan_load_model` multi-file loading for vae/t5/clip
- Add `--dry-run` or validation mode to wan-convert
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| SAFE-03 | 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP） | dit-* types fully satisfy loadability; vae/t5/clip produce valid GGUF but wan_load_model only accepts DiT architectures — document partial satisfaction with inline REQUIREMENTS.md comment |
</phase_requirements>

## Summary

Phase 13 is a pure documentation phase. No new C++ logic is introduced. The three deliverables are: (1) annotated `--help` output in `print_usage()`, (2) a new `examples/convert/README.md`, and (3) an inline traceability comment on SAFE-03 in `REQUIREMENTS.md`.

The key technical fact driving all documentation: `is_wan_gguf()` in `src/api/wan_loader.cpp` accepts a GGUF file only when `general.model_type == "wan"` AND `general.architecture` contains a T2V/I2V/TI2V substring. The `SUBMODEL_META` map in `examples/convert/main.cpp` writes `WAN-VAE`, `WAN-T5`, `WAN-CLIP` as architecture values — none of which match those substrings. Therefore vae/t5/clip conversions produce structurally valid GGUF files that `wan_load_model` will reject at runtime.

All research is grounded in direct source inspection. No external library research is needed.

**Primary recommendation:** Make the loadability boundary explicit in every user-facing surface (help text, README, requirements) without changing any conversion logic.

## Standard Stack

This phase touches only text files and one C++ string literal. No new libraries or build changes.

| File | Type | Change |
|------|------|--------|
| `examples/convert/main.cpp` | C++ | Edit string literal in `print_usage()` |
| `examples/convert/README.md` | Markdown | Create new file |
| `.planning/REQUIREMENTS.md` | Markdown | Edit SAFE-03 line |

## Architecture Patterns

### Existing print_usage() Structure

Current layout (lines 27-45 of `examples/convert/main.cpp`):

```
"  --type   <submodel> Sub-model type (required):\n"
"                        dit-t2v   WAN2.1 DiT text-to-video\n"
"                        dit-i2v   WAN2.2 DiT image-to-video\n"
"                        dit-ti2v  WAN2.2 DiT text+image-to-video\n"
"                        vae       VAE encoder/decoder\n"
"                        t5        T5 text encoder\n"
"                        clip      CLIP image encoder\n"
```

Each type line is a separate string literal fragment. Annotations append to the end of each line before `\n`. Column alignment should be preserved — the existing description text starts at column ~26 from the left margin of the type name.

### Recommended Annotation Style

```c
"                        dit-t2v   WAN2.1 DiT text-to-video          (loadable by wan_load_model)\n"
"                        dit-i2v   WAN2.2 DiT image-to-video         (loadable by wan_load_model)\n"
"                        dit-ti2v  WAN2.2 DiT text+image-to-video    (loadable by wan_load_model)\n"
"                        vae       VAE encoder/decoder                (reserved: future multi-file loading)\n"
"                        t5        T5 text encoder                    (reserved: future multi-file loading)\n"
"                        clip      CLIP image encoder                 (reserved: future multi-file loading)\n"
```

### examples/convert/README.md Structure

Match the style of `examples/README.md`: standard Markdown, `\`\`\`bash` code blocks, pipe tables.

Recommended sections:
1. Brief intro sentence
2. Build note (reference `WAN_BUILD_EXAMPLES`)
3. Usage example (bash block)
4. Type reference table
5. Limitations paragraph

### Type Table Column Design (Claude's Discretion)

Suggested columns matching CONTEXT.md specifics section:

| `--type` value | Sub-model | Status | Notes |
|----------------|-----------|--------|-------|
| dit-t2v | WAN2.1 DiT | Loadable | Accepted by `wan_load_model` |
| dit-i2v | WAN2.2 DiT | Loadable | Accepted by `wan_load_model` |
| dit-ti2v | WAN2.2 DiT | Loadable | Accepted by `wan_load_model` |
| vae | VAE encoder/decoder | Reserved | Future multi-file loading |
| t5 | T5 text encoder | Reserved | Future multi-file loading |
| clip | CLIP image encoder | Reserved | Future multi-file loading |

### Limitations Paragraph Wording (Claude's Discretion)

Verbatim suggestion from CONTEXT.md specifics:

> `vae/t5/clip types produce valid GGUF files but wan_load_model currently expects a single DiT checkpoint. Multi-file loading is planned for a future release.`

This is accurate per `is_wan_gguf()` source inspection and is neutral in tone.

### SAFE-03 Traceability Comment

Current REQUIREMENTS.md line 59:
```
- [ ] **SAFE-03**: 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP）
```

Target (keep `[ ]`, append parenthetical):
```
- [ ] **SAFE-03**: 转换工具支持 WAN2.1/2.2 所有子模型（DiT、VAE、T5、CLIP）（部分满足：dit-* 类型可加载，vae/t5/clip 待 Phase 13 文档说明）
```

Note: REQUIREMENTS.md line 59 already contains this parenthetical from a prior edit. Verify before writing to avoid duplication.

## Don't Hand-Roll

Not applicable — this phase has no algorithmic implementation.

## Common Pitfalls

### Pitfall 1: Line length blowout in print_usage()
**What goes wrong:** Appending long annotation strings pushes lines past terminal width (80 cols), wrapping awkwardly.
**How to avoid:** Use spacing to align annotations at a consistent column. Keep annotation text short. The suggested wording fits within ~100 chars per line.

### Pitfall 2: Duplicating the SAFE-03 parenthetical
**What goes wrong:** REQUIREMENTS.md line 59 already has `（部分满足：dit-* 类型可加载，vae/t5/clip 待 Phase 13 文档说明）` appended. Writing it again creates a doubled comment.
**How to avoid:** Read the current SAFE-03 line before editing. Only add the comment if it is not already present.

### Pitfall 3: Modifying examples/README.md instead of creating examples/convert/README.md
**What goes wrong:** CONTEXT.md explicitly locks the new file to `examples/convert/README.md`. Editing the parent README violates the locked decision.
**How to avoid:** Create a new file; do not touch `examples/README.md`.

### Pitfall 4: Misrepresenting vae/t5/clip as broken
**What goes wrong:** Using warning language ("WARNING: not supported") implies the conversion itself fails. It succeeds — only loading fails.
**How to avoid:** Use neutral language: "reserved", "future multi-file loading". The conversion produces a valid GGUF; the limitation is on the loader side.

## Code Examples

### Current print_usage() — exact lines to edit
```c
// Source: examples/convert/main.cpp lines 34-40
"                        dit-t2v   WAN2.1 DiT text-to-video\n"
"                        dit-i2v   WAN2.2 DiT image-to-video\n"
"                        dit-ti2v  WAN2.2 DiT text+image-to-video\n"
"                        vae       VAE encoder/decoder\n"
"                        t5        T5 text encoder\n"
"                        clip      CLIP image encoder\n"
```

### is_wan_gguf() loadability gate — why dit-* works, vae/t5/clip does not
```cpp
// Source: src/api/wan_loader.cpp lines 75-83
if (arch.find("TI2V") != std::string::npos || arch.find("ti2v") != std::string::npos) {
    model_type = "ti2v";
} else if (arch.find("I2V") != std::string::npos || arch.find("i2v") != std::string::npos) {
    model_type = "i2v";
} else if (arch.find("T2V") != std::string::npos || arch.find("t2v") != std::string::npos) {
    model_type = "t2v";
} else {
    model_type = "t2v";  // Default to T2V
}
```

SUBMODEL_META arch values for vae/t5/clip are `WAN-VAE`, `WAN-T5`, `WAN-CLIP` — none contain T2V/I2V/TI2V substrings, so they fall through to the default `t2v` assignment. However `is_wan_gguf()` returns `true` for any file with `general.model_type == "wan"`, meaning the loader will attempt to use a VAE checkpoint as a DiT model and fail at tensor lookup time, not at the GGUF validation gate. This nuance is worth noting in the README limitations paragraph.

### SUBMODEL_META — source of truth for table data
```cpp
// Source: examples/convert/main.cpp lines 18-25
static const std::map<std::string, SubModelInfo> SUBMODEL_META = {
    {"dit-t2v",  {"WAN-T2V",  "WAN2.1"}},
    {"dit-i2v",  {"WAN-I2V",  "WAN2.2"}},
    {"dit-ti2v", {"WAN-TI2V", "WAN2.2"}},
    {"vae",      {"WAN-VAE",  "WAN2.1"}},
    {"t5",       {"WAN-T5",   "WAN2.1"}},
    {"clip",     {"WAN-CLIP", "WAN2.2"}},
};
```

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None detected — documentation-only phase |
| Config file | none |
| Quick run command | `./build/bin/wan-convert --help` (manual visual check) |
| Full suite command | n/a |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SAFE-03 | `--help` output shows loadability annotations | manual-only | `./build/bin/wan-convert --help \| grep loadable` | n/a — no test file needed |
| SAFE-03 | `examples/convert/README.md` exists with type table | manual-only | `ls examples/convert/README.md` | ❌ Wave 0 (create in plan) |
| SAFE-03 | REQUIREMENTS.md SAFE-03 has traceability comment | manual-only | `grep "SAFE-03" .planning/REQUIREMENTS.md` | ✅ (verify no duplication) |

### Sampling Rate
- **Per task commit:** `./build/bin/wan-convert --help` — visual verify annotations present
- **Per wave merge:** same
- **Phase gate:** All three deliverables present before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `examples/convert/README.md` — covers SAFE-03 documentation requirement (created as part of plan task, not a test file)

## Open Questions

1. **SAFE-03 parenthetical already present?**
   - What we know: REQUIREMENTS.md line 59 already contains `（部分满足：dit-* 类型可加载，vae/t5/clip 待 Phase 13 文档说明）`
   - What's unclear: Whether this was added by a prior agent or is the target state
   - Recommendation: Planner should instruct implementer to read the line first and only append if the comment is absent

2. **is_wan_gguf() default fallthrough behavior**
   - What we know: vae/t5/clip arch strings don't match T2V/I2V/TI2V — they fall to default `t2v` assignment, but `is_wan_gguf()` still returns `true`
   - What's unclear: Whether the README should explain this subtlety or keep it simple
   - Recommendation: Keep README simple ("wan_load_model currently expects a single DiT checkpoint"); the subtlety is an implementation detail users don't need

## Sources

### Primary (HIGH confidence)
- `examples/convert/main.cpp` — direct inspection of `print_usage()` and `SUBMODEL_META`
- `src/api/wan_loader.cpp` — direct inspection of `is_wan_gguf()` loadability logic
- `.planning/REQUIREMENTS.md` — direct inspection of SAFE-03 current state
- `examples/README.md` — direct inspection of existing documentation style
- `.planning/phases/13-document-wan-convert-sub-model-scope/13-CONTEXT.md` — locked decisions

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — pure documentation, no library research needed
- Architecture: HIGH — all patterns derived from direct source inspection
- Pitfalls: HIGH — identified from concrete source facts (duplicate comment, line length, file targeting)

**Research date:** 2026-03-17
**Valid until:** Stable indefinitely (no external dependencies)
