# Office Porting Checklist — ClassificationExtraction (config-driven refactor)

You cannot sync the repo, so this document is the complete change list. Every
change is self-contained; apply it to your office laptop's copy of the
`ClassificationUtilitiesTC2312` solution.

- Office: VS 2022 (toolset `v143` — same as home) + Teamcenter 2312 → **no toolset or TC version differences to worry about**.
- No new external libraries: ODBC is part of Windows (`odbc32.lib` is already on
  the Import/Validation link line today; the Extraction project now links it too).
- Library calls and classification logic are unchanged — ICS_*, ITEM_*, TCTYPE_*
  appear with identical arguments as before. Only wiring around them changed.

---

## 1. What changed in the code (file by file)

### New files (create on the office laptop, copy from home)

| File | Purpose |
|---|---|
| `Config\classification_utilities.cfg` | **The one config file.** TC_ROOT, TC_DATA, build paths, credentials, log dir/prefixes, EXTRACTION input mode + paths + queries, output file, [RUN] FUNCTION. |
| `Config\_cfg.bat` | Batch helper the bats use to read values out of the config. |
| `Config\Build_Extraction.bat` | Sets up VC env from `[BUILD] VCVARSALL`, generates `Config\TC_Root.props`, runs msbuild. |
| `Config\Run_Extraction.bat` | Calls `tc_profilevars.bat`, creates log dir, runs the exe with `-config=`. |
| `Config\Run.bat` | Master dispatcher: reads `[RUN] FUNCTION` → runs `Run_Extraction.bat` (later Import/Validation). |
| `ClassificationExtraction\ConfigParser.hpp/.cpp` | INI parser (`getString/getBool/getDirectory/getPath`, `%ENV_VAR%` expansion, relative-path resolution). |
| `ClassificationExtraction\DBConnector.hpp/.cpp` | ODBC wrapper (`connect`, read-only `executeQuery`, `disconnect`). |
| `ClassificationExtraction\InputLoader.hpp/.cpp` | Ingestion: `CSV` / `DB` / `CSV_DB` modes → same `vector<classifiedObjs_t>`. |

### Modified files

| File | Change |
|---|---|
| `ClassificationExtraction\Source.cpp` | **Replaced.** main = CLI → config → login → logger → InputLoader → unchanged extraction functions → output. Real exit codes. `find_item`/`find_rev` no longer `static`. Old `readInputFile` moved into `InputLoader::loadFromCsv`. |
| `ClassificationExtraction\Header.hxx` | `setAttributeNames`/`setAttributevalues` now `extern` (defined in Source.cpp); `icoAttrValues_t.theAttributeValue` is `std::string` (was `char*`); find_item/find_rev non-static; signatures updated (`const vector<>&`, `writeIntoFile` returns `bool`, takes `const string&`). |
| `ClassificationExtraction\M_Logger.hpp/.cpp` | New `init(dir, prefixOk, prefixFail)`; directory without trailing `\` now works; null-safe writes; configurable prefixes. Old `M_Logger(string)` ctor replaced. |
| `ClassificationExtraction\standard_defines.hpp` | Removed the duplicate `ITK` macro (C4005 clash with Header.hxx); fixed `#endif#pragma once` junk line. |
| `ClassificationExtraction\ClassificationExtraction.vcxproj` | All `D:\Teamcenter_2312` → `$(TC_ROOT)`; Debug|x64 now buildable too; `odbc32.lib` added; new sources registered; imports `Config\TC_Root.props` when present. |
| `ClassificationExtraction\ClassificationExtraction.vcxproj.user` | Debug args cleared — exe finds the config on its own. |
| `ClassificationExtraction\ClassificationExtraction.vcxproj.filters` | New files added under an "Ingestion" filter. |

### Fixed logic flaws (behavior-neutral otherwise)

1. **Use-after-free**: ICO attribute values were stored as `char*` into TC shared memory, then `SAFE_SM_FREE`'d — the output file read freed memory. Values are now copied into `std::string` in `icoAttrValues_t`.
2. **argv[1] crash** when double-clicking the exe with no args.
3. **`-log=` needed a trailing `\`** — now normalized in `M_Logger::init`.
4. **Output file never closed** → possible truncated output; now flushed+closed and failure is reported.
5. **`-output=` was never validated** (UB on `ofstream::open(NULL)`); now checked.
6. **Duplicate ITK macros** (Header.hxx `iStatus` vs standard_defines.hpp `status`) — the standard_defines one silently did nothing useful; now only one definition exists.
7. **Globals defined in header** — now `extern` + single definition.
8. **`static` function parameter** (`getAllClassificationAttributeValues(static vector...)`) — meaningless construct; now `const&` (no full copy per call).
9. **Always returned 0** — the bat now sees a real exit code (0 ok, 1 usage, 2 config, 3 login, 4 input, 5 output, 6 nothing extracted).

---

## 2. Applying it on the office laptop

### Option A — copy the changed files (recommended)

Copy from home → USB → office (files only, no repo needed):

```
Config\  (all files: classification_utilities.cfg, _cfg.bat, Build_Extraction.bat,
          Run_Extraction.bat, Run.bat, OFFICE_PORTING_CHECKLIST.md)
ClassificationExtraction\ConfigParser.hpp/.cpp
ClassificationExtraction\DBConnector.hpp/.cpp
ClassificationExtraction\InputLoader.hpp/.cpp
ClassificationExtraction\Source.cpp
ClassificationExtraction\Header.hxx
ClassificationExtraction\M_Logger.hpp/.cpp
ClassificationExtraction\standard_defines.hpp
ClassificationExtraction\ClassificationExtraction.vcxproj
ClassificationExtraction\ClassificationExtraction.vcxproj.user
ClassificationExtraction\ClassificationExtraction.vcxproj.filters
```

On the office laptop:

1. Put `Config\` at the **solution root** (next to `ClassificationUtilitiesTC2312.sln`).
2. Overwrite the listed `ClassificationExtraction\*` files.
3. Edit `Config\classification_utilities.cfg` **for that machine**:
   - `[ENVIRONMENT] TC_ROOT`  → office TC 2312 path (e.g. `D:\Teamcenter_2312\tc`)
   - `[ENVIRONMENT] TC_DATA`  → office TC data volume
   - `[BUILD] VCVARSALL`      → office VS2022 path (edition may differ, e.g. `...\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat`)
   - `[CREDENTIALS]`          → office TC user/password/group
   - `[LOGS] LOG_DIR`         → any office-writable directory
   - `[EXTRACTION]`           → office input/output paths + the DB settings if you use DB modes.
4. Build: double-click `Config\Build_Extraction.bat` (or open VS normally — it reads
   `TC_Root.props`; if VS was open while the props file was created/changed, close and reopen the solution once so MSBuild re-evaluates it).
5. Run: double-click `Config\Run_Extraction.bat` (or `Run.bat` with `[RUN] FUNCTION=EXTRACTION`).

### Option B — if USB/share transfer is not allowed

Everything is documented above; the only hand-written pieces are:

- `ConfigParser.cpp` (~200 lines), `DBConnector.cpp` (~220 lines), `InputLoader.cpp` (~330 lines) — plain C++, no dependencies.
- The bat files and config file — plain text, copied verbatim.
- The vcxproj diff is just: `$(TC_ROOT)` instead of the two hardcoded paths (in **both** x64 configs), `odbc32.lib` appended to `AdditionalDependencies`, and the 5 new `<ClInclude>/<ClCompile>` entries.

### First-run sanity checks

1. `ClassificationExtraction.exe -h` from `x64\Release` → prints usage, exit code 1.
2. `Run_Extraction.bat` → prints the TC_ROOT/TC_DATA/user it resolved, then runs.
3. Check `LOG_DIR` for `SuccessEPM_<timestamp>.log`; the log's first lines print the
   config file used and the selected input mode.
4. Mode `DB`/`CSV_DB` additionally needs a working ODBC DSN on that machine
   (Windows "ODBC Data Sources (64-bit)" → the DSN name in `DB_CONN_STR` must exist).
   ODBC Administrator → System DSN → check "Test Connection" works with the
   TC database read-only account.
5. `CSV_DB` smoke test: set `INPUT_MODE=CSV_DB` with a one-line CSV (`item_id` only)
   and `CSV_DB_QUERY = SELECT item_id, item_revision_id FROM pitem WHERE item_id = '%ITEM_ID%'`
   — the log lists every substituted query it ran.

---

## 3. Structure after this phase

```
ClassificationUtilitiesTC2312\
├── ClassificationUtilitiesTC13.sln
├── Config\                          ← the ONLY place to configure
│   ├── classification_utilities.cfg
│   ├── _cfg.bat
│   ├── Build_Extraction.bat
│   ├── Run_Extraction.bat
│   ├── Run.bat                      ← later: dispatches Import/Validation too
│   └── TC_Root.props                ← generated at build time (do not edit)
├── ClassificationExtraction\        ← migrated (this phase)
├── ClassificationImport\            ← next phase: same pattern
└── ClassificationValidation\        ← next phase: same pattern
```

**Next phases** (same recipe, minimal diffs): copy `ConfigParser/DBConnector/InputLoader`
into Import and Validation, add the same `[IMPORT]` / `[VALIDATION]` config consumption
to their mains, and give each a `Run_Import.bat` / `Run_Validation.bat` so `Run.bat`
dispatches all three. The config file already contains those sections, so it never
needs to change shape again.
