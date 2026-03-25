---
phase: quick
plan: 260325-fcl-spdlog-thirdparty
type: execute
wave: 1
depends_on: []
files_modified: [thirdparty/CMakeLists.txt, thirdparty/spdlog/*, .gitignore, CMakeLists.txt]
autonomous: true
requirements: []
---

<objective>
Convert spdlog from online FetchContent download to offline thirdparty integration.

Purpose: Eliminate compile-time network dependency, ensure reproducible builds with pinned spdlog version
Output: spdlog source in thirdparty/, CMake configuration updated to use local spdlog
</objective>

<execution_context>
@$HOME/.claude/get-shit-done/workflows/execute-plan.md
</execution_context>

<context>
@/data/zhongwang2/jtzhuang/projects/OnlyWan/.planning/STATE.md
@/data/zhongwang2/jtzhuang/projects/OnlyWan/CMakeLists.txt
@/data/zhongwang2/jtzhuang/projects/OnlyWan/thirdparty/CMakeLists.txt

Current spdlog usage:
- thirdparty/CMakeLists.txt: FetchContent_Declare spdlog v1.13.0 from GitHub
- CMakeLists.txt line 231: target_link_libraries uses spdlog::spdlog
- src/util.h: includes "spdlog/spdlog.h" and "spdlog/fmt/ostr.h"
- src/util.cpp: includes "spdlog/sinks/stdout_color_sinks.h"
</context>

<tasks>

<task type="auto">
  <name>Task 1: Download and commit spdlog v1.13.0 to thirdparty</name>
  <files>thirdparty/spdlog/*, .gitignore</files>
  <action>
1. Download spdlog v1.13.0 from GitHub:
   - Clone: git clone --depth 1 --branch v1.13.0 https://github.com/gabime/spdlog.git /tmp/spdlog-v1.13.0
   - Copy include/ directory to thirdparty/spdlog/
   - Copy CMakeLists.txt from spdlog repo to thirdparty/spdlog/

2. Update .gitignore to explicitly NOT ignore thirdparty/spdlog/:
   - If .gitignore has "thirdparty/*" or similar wildcard, add exception: "!thirdparty/spdlog/"
   - Commit thirdparty/spdlog/ headers to git

3. Verify structure after copy:
   - thirdparty/spdlog/include/spdlog/spdlog.h exists
   - thirdparty/spdlog/include/spdlog/sinks/stdout_color_sinks.h exists
   - thirdparty/spdlog/include/spdlog/fmt/ostr.h exists
  </action>
  <verify>
    <automated>ls -la /data/zhongwang2/jtzhuang/projects/OnlyWan/thirdparty/spdlog/include/spdlog/{spdlog.h,fmt/ostr.h,sinks/stdout_color_sinks.h}</automated>
  </verify>
  <done>spdlog v1.13.0 source committed to thirdparty/spdlog/, .gitignore updated to track spdlog headers</done>
</task>

<task type="auto">
  <name>Task 2: Replace FetchContent with header-only CMake configuration</name>
  <files>thirdparty/CMakeLists.txt</files>
  <action>
Replace the FetchContent_Declare block with direct include path setup:

Remove these lines (4-10):
```
include(FetchContent)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
)
FetchContent_MakeAvailable(spdlog)
```

Add header-only spdlog interface library:
```
# spdlog - header-only, already in thirdparty/spdlog/
if(NOT TARGET spdlog::spdlog)
    add_library(spdlog INTERFACE)
    target_include_directories(spdlog INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
    )
    add_library(spdlog::spdlog ALIAS spdlog)
endif()
```

Keep the rest of thirdparty/CMakeLists.txt unchanged (zip library section).
  </action>
  <verify>
    <automated>grep -A2 "# spdlog - header-only" /data/zhongwang2/jtzhuang/projects/OnlyWan/thirdparty/CMakeLists.txt && ! grep -q "FetchContent_Declare" /data/zhongwang2/jtzhuang/projects/OnlyWan/thirdparty/CMakeLists.txt</automated>
  </verify>
  <done>FetchContent removed, local spdlog interface library created, CMake build still links spdlog::spdlog without network dependency</done>
</task>

<task type="auto">
  <name>Task 3: Verify build with local spdlog</name>
  <files></files>
  <action>
Build the project to confirm spdlog is found locally:

1. Create fresh build directory:
   cd /data/zhongwang2/jtzhuang/projects/OnlyWan
   mkdir -p build_test
   cd build_test

2. Configure and build:
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . -j$(nproc)

3. Check for network access during build:
   - Should NOT see "Cloning into" or FetchContent download messages for spdlog
   - Should see "Building C object thirdparty/..." messages
   - Final link should reference spdlog headers from thirdparty/spdlog/include/

4. Clean up test build:
   cd /data/zhongwang2/jtzhuang/projects/OnlyWan
   rm -rf build_test
  </action>
  <verify>
    <automated>grep -q "spdlog::spdlog" /data/zhongwang2/jtzhuang/projects/OnlyWan/CMakeLists.txt && ls /data/zhongwang2/jtzhuang/projects/OnlyWan/thirdparty/spdlog/include/spdlog/spdlog.h</automated>
  </verify>
  <done>Project builds successfully with spdlog from local thirdparty/, no FetchContent network calls</done>
</task>

</tasks>

<verification>
Confirm:
- spdlog v1.13.0 source files exist in thirdparty/spdlog/
- .gitignore allows thirdparty/spdlog/ to be tracked
- thirdparty/CMakeLists.txt creates spdlog::spdlog target from local headers
- No FetchContent references in CMakeLists.txt
- Build succeeds with local spdlog
</verification>

<success_criteria>
- All spdlog headers present in thirdparty/spdlog/include/
- FetchContent removed from thirdparty/CMakeLists.txt
- spdlog::spdlog interface library created pointing to local headers
- Clean build succeeds without any network requests
- .gitignore updated to track spdlog files
</success_criteria>

<output>
After completion, create `.planning/quick/260325-fcl-spdlog-thirdparty/260325-fcl-SUMMARY.md`
</output>
