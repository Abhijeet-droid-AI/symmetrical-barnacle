# Setting Up a NEW Project on the Office Laptop (VS 2022 + Teamcenter 2312)
# and integrating the ClassificationExtraction utility

This guide takes you from a **blank office laptop** to a **working, config-driven
ClassificationExtraction utility**. It assumes nothing exists on the office
machine yet. (If you already have a working copy of the solution there, follow
`OFFICE_PORTING_CHECKLIST.md` instead — this document is the from-zero path.)

Two config files control everything:

- `Config\classification_utilities.cfg` — ALL developer/library settings:
  build ([BUILD]), Teamcenter environment ([ENVIRONMENT] TC_ROOT/TC_DATA),
  logs and the function sections. Only [BUILD] is edited per machine (Step 5).
- `Config\tc_config.txt` — the Teamcenter LOGIN ONLY (TC_USER/TC_PASS/TC_GROUP).
  This is the ONLY file a USER ever edits after you deliver the exes.

No source changes, no project property changes.

---

## Step 0 — Prerequisites checklist (verify before starting)

| # | Requirement | How to verify |
|---|---|---|
| 1 | Visual Studio 2022 with **"Desktop development with C++"** workload installed (any edition — Community/Professional/Enterprise; toolset v143) | Start Menu → Visual Studio 2022 → New Project → "Console App (C++)" template exists |
| 2 | MSBuild available | After VS install, `vcvarsall.bat` exists at:
`C:\Program Files\Microsoft Visual Studio\2022\<Edition>\VC\Auxiliary\Build\vcvarsall.bat` |
| 3 | Teamcenter 2312 client + TC SDK/ITK headers & libs installed (folder containing `include\`, `include_cpp\`, `lib\itk_main.obj`) | e.g. `D:\Teamcenter_2312\tc\include\tc\tc_startup.h` exists |
| 4 | TC_DATA volume reachable (contains `tc_profilevars.bat`) | e.g. `\\server\apps\TCDATA\tc_profilevars.bat` opens/runs |
| 5 | (Only for DB / CSV_DB modes) A 64-bit ODBC DSN that can query the TC database | Windows Start → "ODBC Data Sources (64-bit)" → System DSN → Test Connection |
| 6 | A TC test account (user / password / group) for smoke testing | Try logging into Rich Client / Portal with it |

If any of these fail, fix that first — nothing later will work without them.

---

## Step 1 — Create the project folder skeleton

On the office laptop, create this structure (drive letter can be anything; the
config file is what points everything else at it):

```
D:\ITK_customization\ClassificationUtilities\
│
├── Config\                          <- create empty
├── ClassificationExtraction\        <- create empty
├── x64\                             <- created automatically at build time
```

Naming convention: the solution root folder name is free; the exe ends up in
`<root>\x64\Release\ClassificationExtraction.exe`.

---

## Step 2 — Copy the files from your home laptop

Plug in your USB / share and copy these files, preserving relative placement.

### 2.1 Into the solution root:

```
ClassificationUtilitiesTC13.sln            (copy as-is; name is historical, harmless)
```

> Alternative: if you prefer a clean name, create a fresh solution in Step 3
> instead and skip copying the `.sln`. The bat files fall back to building the
> `.vcxproj` directly when the `.sln` is missing, so both paths work.

### 2.2 Into `Config\`:

```
classification_utilities.cfg       ALL developer settings ([BUILD] edited in Step 5)
tc_config.txt                      TC login - the ONLY file a USER edits
_cfg.bat                           batch helper that reads the config files
Build_All.bat                      builds the solution (ALL / EXTRACTION / IMPORT / DELETE)
Build_Extraction.bat               compat wrapper -> Build_All.bat EXTRACTION
Run.bat                            master dispatcher ([RUN] FUNCTION)
Run_Extraction.bat                 runs the extraction exe
Run_Import.bat                     runs the import exe
Run_Delete.bat                     runs the delete exe
```

(Do NOT copy `TC_Root.props` — it gets generated automatically by the build bat.
Do not copy the `.md` docs unless you want them for reference.)

### 2.3 Into `ClassificationExtraction\` — source files:

```
Source.cpp                         main + unchanged ICS/ITEM/TCTYPE extraction logic
Header.hxx                         structs, ITK macro, declarations
standard_defines.hpp               shared macros
M_Logger.hpp / M_Logger.cpp        logging (config-driven prefixes, safe paths)
ConfigParser.hpp / ConfigParser.cpp        INI config reader
DBConnector.hpp / DBConnector.cpp          read-only ODBC wrapper
InputLoader.hpp / InputLoader.cpp          CSV / DB / CSV_DB ingestion
```

### 2.4 Into `ClassificationExtraction\` — project files:

```
ClassificationExtraction.vcxproj           references $(TC_ROOT), odbc32.lib, new sources
ClassificationExtraction.vcxproj.filters   Solution Explorer organization
ClassificationExtraction.vcxproj.user      debug settings (empty args = config-driven)
```

### 2.5 (Recommended) sample data for the smoke test:

```
ClassificationExtraction\InputFileT.txt    your existing pipe-delimited sample input
```

---

## Step 3 — Create/open the solution in Visual Studio 2022

**Option A (recommended): use the copied solution**

1. Double-click `ClassificationUtilitiesTC13.sln`.
2. If VS offers to retarget platforms/toolset, accept (v143 is already correct for VS2022).
3. You should see project **ClassificationExtraction** with 5 .cpp / 6 headers.
4. Set Configuration = `Release`, Platform = `x64`.

**Option B: create a brand-new solution**

1. VS 2022 → File → New → Project → **Empty Project (C++)** → name it
   `ClassificationExtraction`, location = the root folder from Step 1.
2. In Solution Explorer: add existing items — all `.cpp`/`.hpp`/`.hxx` files from Step 2.3.
3. Project Properties (Release | x64):
   - C/C++ → General → Additional Include Directories:
     `$(TC_ROOT)\include;$(TC_ROOT)\include_cpp`
   - C/C++ → Preprocessor → Preprocessor Definitions: add
     `IPLIB=none;_CRT_SECURE_NO_WARNINGS`
   - Linker → General → Additional Library Directories: `$(TC_ROOT)\lib`
   - Linker → Input → Additional Dependencies:
     `$(TC_ROOT)\lib\*.lib;$(TC_ROOT)\lib\itk_main.obj;odbc32.lib`
   - (The copied `.vcxproj` from Step 2.4 already contains all of this — Option B
     is only for when you want a pristine project file.)
4. Note: the project imports `Config\TC_Root.props` if present. It is generated
   by `Build_Extraction.bat` (Step 6). If you build inside VS before ever running
   the bat, set env var `TC_ROOT` once first (Step 5.3) or run the bat once.

> **Why `$(TC_ROOT)` matters:** the project file contains **no** machine-specific
> Teamcenter paths. VS (or msbuild) resolves `$(TC_ROOT)` from `TC_Root.props`
> (generated from the config) or from the `TC_ROOT` environment variable.

---

## Step 4 — Understand the two config files

`Config\classification_utilities.cfg` (developer-owned — users NEVER edit it):

```ini
[ENVIRONMENT]                        ; Teamcenter library settings (developer)
TC_ROOT  = <TC install used to build, e.g. D:\Teamcenter_2312\tc>
TC_DATA  = <TC data volume, e.g. \\office-tcserver\apps\TCDATA>

[BUILD]                              ; build settings (developer)
VCVARSALL     = <office VS2022 vcvarsall.bat full path>
PLATFORM      = x64
CONFIGURATION = Release

; NO [CREDENTIALS] here any more - the TC login lives in tc_config.txt

[LOGS]
LOG_DIR         = D:\ITK_customization\ClassificationUtilities\Logs\
LOG_PREFIX_OK   = SuccessEPM_
LOG_PREFIX_FAIL = FailEPM_

[EXTRACTION]
INPUT_MODE    = CSV            ; CSV | DB | CSV_DB
CSV_FILE      = ...\InputFileT.txt
CSV_DELIMITER = |
CSV_HAS_HEADER = YES
CSV_DB_FALLBACK_TO_CSV = YES
DB_CONN_STR   = DSN=<office DSN>;UID=<ro user>;PWD=<ro pwd>
DB_QUERY      = SELECT item_id, item_revision_id FROM pitem WHERE ...
CSV_DB_QUERY  = SELECT item_id, item_revision_id FROM pitem WHERE item_id = '%ITEM_ID%'
OUTPUT_FILE   = ...\OutputFile.txt

[RUN]
FUNCTION = EXTRACTION
```

`Config\tc_config.txt` (user-owned — the ONLY file a user edits):

```ini
[CREDENTIALS]
TC_USER  = <office TC test user>
TC_PASS  = <password — plaintext warning applies>
TC_GROUP = <group, usually dba>
```

Editing rules: `;` or `#` starts a comment; paths may omit the trailing `\`;
keys are case-insensitive; `INPUT_MODE` picks the ingestion path.

---

## Step 5 — Configure the office machine specifics

5.1 Edit `[ENVIRONMENT]` → TC_ROOT, TC_DATA only if the office Teamcenter
    paths differ from the values already in the file (developer-owned;
    a USER never touches this section).
5.2 Edit `[BUILD]` → VCVARSALL (office VS2022 path from Step 0.2).
5.3 Edit `Config\tc_config.txt` → office TC test account
    (TC_USER / TC_PASS / TC_GROUP). On a user's machine this is the
    ONLY file they edit.
5.4 (Optional, for building inside the VS IDE) set a machine-wide variable once:
    `setx TC_ROOT "D:\Teamcenter_2312\tc"` then reopen VS. Not needed for bat builds.
5.5 (Only for DB/CSV_DB) create the 64-bit ODBC DSN:
    "ODBC Data Sources (64-bit)" → System DSN → Add → your TC DB driver
    (Oracle/MSSQL) → name it e.g. `TCDB` → test connection with a **read-only**
    DB account. Put `DSN=TCDB;UID=...;PWD=...` into `DB_CONN_STR`.
5.6 Create the log directory if it doesn't exist (the bat also auto-creates it):
    `mkdir D:\ITK_customization\ClassificationUtilities\Logs`

---

## Step 6 — Build

**From the bat (no VS needed):**
```
Double-click  Config\Build_Extraction.bat
```
It will:
- read TC_ROOT + VCVARSALL from the config,
- generate `Config\TC_Root.props` (defines `$(TC_ROOT)` for the vcxproj),
- call `vcvarsall.bat x64` and msbuild,
- leave `x64\Release\ClassificationExtraction.exe`.

**From the VS IDE:**
- Ensure TC_Root.props exists (run the bat once) or TC_ROOT env var is set,
  then Build → Build Solution (Release | x64).
- If you created/changed TC_Root.props while VS was open: close & reopen the
  solution so MSBuild re-evaluates the props import.

**Expected first-build notes:**
- Warnings C4100/C4189 etc. from legacy-style code are fine.
- `link.exe` processes `$(TC_ROOT)\lib\*.lib` — takes a while, that's normal.
- If the linker can't find `itk_main.obj`, TC_ROOT is wrong — re-check Step 5.1.

---

## Step 7 — Run & verify (smoke test sequence)

Run each with `INPUT_MODE=CSV` first (no DB dependency), then the others.

**T1 — Usage screen (no TC login needed)**
```
cd /d D:\ITK_customization\ClassificationUtilities\x64\Release
ClassificationExtraction.exe -h
```
Expected: usage text, exit code 1.

**T2 — CSV end-to-end**
- `[EXTRACTION] INPUT_MODE = CSV`, CSV_FILE = your sample `InputFileT.txt`.
- Double-click `Config\Run_Extraction.bat`.
- Console shows: config path used → TC_ROOT/TC_DATA → "Login Successful" →
  items processed → "EXITED from writeIntoFile" → "finished with exit code 0".
- Check `LOG_DIR` for `SuccessEPM_<timestamp>.log`; first lines print config file
  + input mode. Open OUTPUT_FILE — header + one row per classified object.

**T3 — DB mode** (requires Step 5.5 DSN)
- `INPUT_MODE = DB`, set `DB_QUERY` to return item_id (col 1) and optionally
  item_revision_id (col 2). Re-run. Log shows "Query returned N row(s)".

**T4 — CSV_DB combo mode**
- `INPUT_MODE = CSV_DB`, CSV lists ids, `CSV_DB_QUERY` contains `%ITEM_ID%`
  (and optionally `%ITEM_REV%`).
- Log shows every substituted query: `... WHERE item_id = '1111-5YY0197'`.
- If a CSV id has no DB row: fallback controlled by `CSV_DB_FALLBACK_TO_CSV`.

**T5 — Master dispatcher**
- Keep `[RUN] FUNCTION = EXTRACTION`, double-click `Config\Run.bat` → same as T2.

**Exit codes:** 0 ok · 1 usage · 2 config problem · 3 TC login failed ·
4 input loading failed · 5 output write failed · 6 nothing classified found.

---

## Step 8 — Troubleshooting

| Symptom | Likely cause / fix |
|---|---|
| `ERROR : config file not found` | `Config\` must sit at solution root next to the `.sln`; or pass `-config=<full path>`. |
| `configuration file not found` from the exe (run via VS) | Working dir is `x64\Release`; the exe searches `..\..\Config\` — run via the bat, or pass `-config=`. |
| Build: `tc_startup.h` not found | TC_ROOT wrong in config, or VS opened before `TC_Root.props` existed → rerun build bat / reopen solution. |
| Build: `MSB... toolset v143 not found` | VS2022 C++ workload missing (Step 0.1). |
| Login failed (exit 3) | Wrong credentials, or TC_DATA/tc_profilevars points to another environment. |
| `Failed to open Input file` (exit 4) | CSV_FILE path wrong for office; check the log for the exact resolved path. |
| DB: `SQLSTATE=IM002` | DSN doesn't exist (or created as 32-bit). Create **64-bit** DSN (Step 5.5). |
| DB: `SQLSTATE=28000` | DSN credentials wrong / read-only account locked. |
| Output file empty/missing (exit 6) | No classified objects among inputs — check FailEPM_*.log for "Niether Item Nor Revision is classified". |
| Output file not writable (exit 5) | File open in Excel; or no write permission on folder. |
| Items "not found" though they exist | Item belongs to another org — TC login user lacks access; or rev id text doesn't match exactly. |

---

## Step 9 — Where things live now (mental model)

```
Office laptop
D:\ITK_customization\ClassificationUtilities\
├── ClassificationUtilitiesTC13.sln
├── Config\
│   ├── classification_utilities.cfg   ← ALL developer settings (build, TC env, functions)
│   ├── tc_config.txt                  ← TC login only (the file a USER edits)
│   ├── _cfg.bat / Build_*.bat / Run_*.bat / Run.bat
│   └── TC_Root.props                  ← generated, do not edit
├── ClassificationExtraction\          ← machine-INDEPENDENT sources
└── x64\Release\ClassificationExtraction.exe
Runtime artifacts
├── Logs\SuccessEPM_*.log, FailEPM_*.log
└── OutputFile.txt
```

Machines differ **only** in `classification_utilities.cfg` (developer settings,
[BUILD]/[ENVIRONMENT]) and `tc_config.txt` (the user's TC login). Sources and
project files are identical between home and office — that's what makes the USB
copy in Step 2 a one-time operation. When you deliver to a user, they edit ONLY
`tc_config.txt`.

---

## Step 10 — Later phases (Import / Validation)

When you migrate ClassificationImport and ClassificationValidation with the same
pattern, nothing in this setup changes:

1. Copy their migrated sources into their folders (same recipe as Extraction).
2. Add `Run_Import.bat` / `Run_Delete.bat` next to `Run_Extraction.bat`
   (already present in this project).
3. Fill their existing `[IMPORT]` / `[DELETE]` config sections.
4. Switch utilities by editing `[RUN] FUNCTION` — or run their `Run_*.bat` directly.
