"""
Generates 'Classification Utilities - Build Fix Guide.docx' in the project root.
Follows the exact step-by-step structure of Office_Setup_Guide.html:
title page, issue summary table, per-fix sections (symptom -> cause -> fix),
each fix split into Step N with numbered actions, code in Courier New tables,
expected result, and a final verification checklist.

Run:  python Config/generate_build_fix_doc.py
"""

import os

from docx import Document
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Pt, RGBColor, Inches

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_PATH = os.path.join(
    PROJECT_ROOT, "Classification Utilities - Build Fix Guide.docx")

MONO = "Courier New"
ACCENT = RGBColor(0x1F, 0x4E, 0x79)
GREEN = RGBColor(0x00, 0x66, 0x00)

doc = Document()


# ----------------------------------------------------------------- helpers
def shade(cell, hex_fill):
    el = OxmlElement("w:shd")
    el.set(qn("w:val"), "clear")
    el.set(qn("w:fill"), hex_fill)
    cell._tc.get_or_add_tcPr().append(el)


def add_code(text, font_size=8.5):
    """Code block rendered as a one-cell table with a light background."""
    tbl = doc.add_table(rows=1, cols=1)
    tbl.alignment = WD_TABLE_ALIGNMENT.CENTER
    cell = tbl.rows[0].cells[0]
    shade(cell, "F2F2F2")
    cell.paragraphs[0].text = ""
    lines = text.strip("\n").split("\n")
    for idx, line in enumerate(lines):
        p = cell.paragraphs[0] if idx == 0 else cell.add_paragraph()
        p.paragraph_format.space_after = Pt(0)
        run = p.add_run(line)
        run.font.name = MONO
        run.font.size = Pt(font_size)
    doc.add_paragraph().paragraph_format.space_after = Pt(2)


def h1(text):
    p = doc.add_heading(text, level=1)
    for r in p.runs:
        r.font.color.rgb = ACCENT


def note(text):
    tbl = doc.add_table(rows=1, cols=1)
    cell = tbl.rows[0].cells[0]
    shade(cell, "EAF1FB")
    cell.paragraphs[0].text = ""
    run = cell.paragraphs[0].add_run(text)
    run.font.size = Pt(9.5)
    doc.add_paragraph().paragraph_format.space_after = Pt(2)


def expected(text):
    tbl = doc.add_table(rows=1, cols=1)
    cell = tbl.rows[0].cells[0]
    shade(cell, "E9F0E4")
    cell.paragraphs[0].text = ""
    run = cell.paragraphs[0].add_run("Expected result: " + text)
    run.font.size = Pt(9.5)
    run.font.color.rgb = GREEN
    doc.add_paragraph().paragraph_format.space_after = Pt(2)


# ------------------------------------------------------------- title page
t = doc.add_paragraph()
t.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = t.add_run("Classification Utilities TC13")
r.bold = True
r.font.size = Pt(26)

t2 = doc.add_paragraph()
t2.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = t2.add_run("Build Fix Guide")
r.bold = True
r.font.size = Pt(20)
r.font.color.rgb = ACCENT

t3 = doc.add_paragraph()
t3.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = t3.add_run("Resolving the 3 build errors reported by Build_All.bat\n"
               "(ClassificationImport - ClassificationExtraction - "
               "ClassificationDelete)")
r.font.size = Pt(11)

t4 = doc.add_paragraph()
t4.alignment = WD_ALIGN_PARAGRAPH.CENTER
r = t4.add_run("Target environment: Teamcenter 2312 (TC13) - Visual Studio "
               "2022 (v143) - Windows x64 - Release")
r.font.size = Pt(9.5)

doc.add_paragraph()
note("HOW TO USE THIS DOCUMENT: work through Fix 1, Fix 2 and Fix 3 in order "
     "- the numbering matches the folders ClassificationImport, "
     "ClassificationExtraction and ClassificationDelete. Every change is "
     "copy-paste ready. After each fix, the 'Expected result' section tells "
     "you what the next build must show for that project. The summary table "
     "in section 3 is the acceptance checklist.")

# ------------------------------------------------------------- 1. summary
h1("1. Summary of the Build Errors")
p = doc.add_paragraph(
    "The Build_All.bat run failed with 12 errors across the 3 utility "
    "projects. They are 3 independent root causes:")
tbl = doc.add_table(rows=5, cols=4)
tbl.style = "Table Grid"
hdr = ["#", "Project", "Error(s)", "Root cause"]
for i, htxt in enumerate(hdr):
    c = tbl.rows[0].cells[i]
    c.paragraphs[0].text = ""
    run = c.paragraphs[0].add_run(htxt)
    run.bold = True
    shade(c, "D9E2F3")
rows = [
    ("Fix 1", "ClassificationImport",
     "LNK2001 unresolved external symbol ?getObject@@YAHPEBD0PEAI@Z\n"
     "LNK1120: 1 unresolved externals",
     "getObject() is declared (Source.cpp, line 26) and called (line 239) "
     "but the function body does not exist in this project - it was only "
     "ever implemented in ClassificationDelete\\Source.cpp (line 804). "
     "The linker therefore cannot resolve the symbol."),
    ("Fix 2", "ClassificationExtraction",
     "9 x C2065: 'iStatus': undeclared identifier\n"
     "(Source.cpp lines 438, 439, 450-453, 463, 465, 467)",
     "The ITK(x) macro defined in Header.hxx expands to "
     "iStatus = (x). The function writeIntoFile() uses ITK() calls but "
     "never declares the iStatus variable - every other function in the "
     "file declares 'int iStatus = ITK_ok;' at its top."),
    ("Fix 3", "ClassificationDelete",
     "C1189: #error: EXPORT is incompatibly defined; use AE_EXPORT and "
     "re-arrange includes (ae\\datasettype.h line 55, while compiling "
     "DBConnector.cpp)",
     "Include-order problem. For DBConnector.cpp the include chain is "
     "DBConnector.hpp -> standard_defines.hpp (TC headers first) -> "
     "M_Logger.hpp -> Header.hxx, so ae/datasettype.h is only reached at "
     "the END - after other TC headers have already defined EXPORT. "
     "datasettype.h detects the clash and raises #error. The fix is to "
     "include the AE headers FIRST."),
]
for ri, (fx, prj, err, cause) in enumerate(rows, start=1):
    vals = (fx, prj, err, cause)
    for ci, val in enumerate(vals):
        c = tbl.rows[ri].cells[ci]
        c.paragraphs[0].text = ""
        for li, line in enumerate(str(val).split("\n")):
            p = c.paragraphs[0] if li == 0 else c.add_paragraph()
            run = p.add_run(line)
            run.font.size = Pt(8.5)
            if ci == 0:
                run.bold = True
    shade(tbl.rows[ri].cells[0], "EAF1FB")
for w, col in zip((Inches(0.6), Inches(1.35), Inches(2.05), Inches(2.5)),
                  range(4)):
    for cell in tbl.columns[col].cells:
        cell.width = w

doc.add_page_break()

# ------------------------------------------------------------- 2. prelim
h1("2. Before You Start")
for line in (
    "Log in with a user account that has write access to "
    "D:\\ClassificationUtilities.",
    "Close any open Visual Studio instance that has the solution "
    "ClassificationUtilitiesTC13.sln loaded (files must not be locked).",
    "Keep this path ready - it is used throughout: "
    "D:\\ClassificationUtilities",
    "Do NOT change anything under D:\\Teamcenter_2312 (Teamcenter headers "
    "are never modified - all fixes are in the utility project sources).",
):
    doc.add_paragraph(line, style="List Bullet")

# ------------------------------------------------------------- fix 1
h1("3. Fix 1 - ClassificationImport: unresolved external symbol getObject")
doc.add_heading("Symptom (from the failed Build_All.bat run)", level=2)
add_code(
    "Source.obj : error LNK2001: unresolved external symbol\n"
    '  "int __cdecl getObject(char const *,char const *,unsigned int *)"\n'
    "  (?getObject@@YAHPEBD0PEAI@Z)\n"
    "D:\\ClassificationUtilities\\x64\\Release\\ClassificationImport.exe :\n"
    "  fatal error LNK1120: 1 unresolved externals")
doc.add_heading("Why it happens", level=2)
p = doc.add_paragraph(
    "In ClassificationImport\\Source.cpp the function is only DECLARED "
    "(line 26) and CALLED (line 239) - the implementation was never added "
    "to this project. The identical function exists in "
    "ClassificationDelete\\Source.cpp (line 804), so we copy the proven "
    "implementation into Import (one adaptation: Import's logger object is "
    "named logger, Delete's is named loggers).")
doc.add_heading("The Fix", level=2)

doc.add_heading("Step 1.1 - Open the file", level=3)
doc.add_paragraph("Open ClassificationImport\\Source.cpp in Visual Studio "
                  "(or any text editor).")

doc.add_heading("Step 1.2 - Go to the very end of the file", level=3)
doc.add_paragraph(
    "The file ends with the function getItemOrRevToValidate(), whose last "
    "lines look like this:")
add_code(
    "/*sWrite << \"INFO: Object latest revision is \" << rev_id.getString()"
    " << endl;\n"
    "loggers.write(sWrite.str());\n"
    "sWrite.str(\"\");*/\n\n"
    "return obj_rev;\n"
    "}")
doc.add_paragraph(
    "Place the cursor on the LAST line of the file (after that closing "
    "brace) and paste the following complete new function:")
add_code(
    "/*\n"
    "  FIX (LNK2001 / LNK1120): getObject() was declared at the top of this"
    " file and\n"
    "  called in the classification loop, but never implemented in this"
    " project -\n"
    "  the definition only existed in ClassificationDelete\\Source.cpp. This"
    " is the\n"
    "  same implementation (uses Import's \"logger\" instead of Delete's"
    " \"loggers\").\n"
    "  Finds an object of the given item_id + object_type in Teamcenter.\n"
    "*/\n"
    "int getObject(const char* itemId, const char* pObjType, tag_t* tObj)\n"
    "{\n"
    "    int nObjs = 0;\n"
    "    int iStatus = ITK_ok;\n"
    "    tag_t* tObjs = NULL;\n"
    "    char* cObjType = NULL;\n"
    "\n"
    "    const char\n"
    "        * names[2]  = { \"item_id\" , \"object_type\" },\n"
    "        * values[2] = { itemId , pObjType };\n"
    "\n"
    "    ITK( ITEM_find_items_by_key_attributes( 2, names, values, &nObjs,"
    " &tObjs ) );\n"
    "\n"
    "    if ( nObjs > 0 )\n"
    "    {\n"
    "        for ( int ii = 0; ii < nObjs; ii++ )\n"
    "        {\n"
    "            ITK( AOM_ask_value_string( tObjs[ii], \"object_type\","
    " &cObjType ) );\n"
    "\n"
    "            if ( tc_strcmp( pObjType, cObjType ) == 0 )\n"
    "            {\n"
    "                *tObj = tObjs[ii];\n"
    "            }\n"
    "            SAFE_MEM_FREE( cObjType );\n"
    "        }\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        logger.writefaillog( \"Object -> \" + (string)itemId +\n"
    "                             \" not found in Teamcenter\" );\n"
    "    }\n"
    "\n"
    "    SAFE_MEM_FREE( tObjs );\n"
    "\n"
    "    return 0;\n"
    "}")
note("IMPORTANT: use SAFE_MEM_FREE here (Import only defines SAFE_MEM_FREE "
     "in Header.hpp) - NOT the SAFE_SM_FREE name used in the Delete "
     "project. Also note the code uses tabs in the original file - either "
     "is fine as long as braces and semicolons match exactly.")
doc.add_heading("Step 1.3 - Save the file", level=3)
expected("ClassificationImport links successfully; the LNK2001/LNK1120 "
         "errors are gone from the next build output.")
doc.add_page_break()

# ------------------------------------------------------------- fix 2
h1("4. Fix 2 - ClassificationExtraction: 'iStatus': undeclared identifier")
doc.add_heading("Symptom (from the failed Build_All.bat run)", level=2)
add_code(
    "D:\\ClassificationUtilities\\ClassificationExtraction\\Source.cpp"
    "(438,17): error C2065: 'iStatus': undeclared identifier\n"
    "(...same error for lines 439, 450, 451, 452, 453, 463, 465, 467)")
doc.add_heading("Why it happens", level=2)
p = doc.add_paragraph(
    "The ITK() macro in ClassificationExtraction\\Header.hxx (line 111) "
    "expands to iStatus = (x). The function writeIntoFile() (Source.cpp, "
    "line ~403) calls ITK() several times but never declares an iStatus "
    "variable - unlike every other function in this file, which starts "
    "with 'int iStatus = ITK_ok;'.")
doc.add_heading("The Fix", level=2)

doc.add_heading("Step 2.1 - Open the file and locate the function", level=3)
doc.add_paragraph(
    "Open ClassificationExtraction\\Source.cpp and search (Ctrl+F) for:")
add_code("bool writeIntoFile( map< tag_t, map< string, icoAttrValues_t > >"
         " mapClassifiedObjects,")
doc.add_paragraph(
    "You are now at the top of the function. It currently begins:")
add_code(
    "bool writeIntoFile( map< tag_t, map< string, icoAttrValues_t > >"
    " mapClassifiedObjects,\n"
    "                    const string& sOutputFile )\n"
    "{\n"
    "    ofstream fpOutputFile;\n"
    "    fpOutputFile.open( sOutputFile.c_str() );")

doc.add_heading("Step 2.2 - Declare iStatus", level=3)
doc.add_paragraph(
    "Add ONE line right after 'ofstream fpOutputFile;' so the opening of "
    "the function looks exactly like this (the new line is marked):")
add_code(
    "bool writeIntoFile( map< tag_t, map< string, icoAttrValues_t > >"
    " mapClassifiedObjects,\n"
    "                    const string& sOutputFile )\n"
    "{\n"
    "    ofstream fpOutputFile;\n"
    "\n"
    "    /* FIX (C2065 'iStatus' undeclared): the ITK() macro defined in"
    " Header.hxx\n"
    "       expands to \"iStatus = (x)\" - this function used ITK() but"
    " never declared\n"
    "       iStatus, unlike every other function in this file. */\n"
    "    int iStatus = ITK_ok;          <-- ADD THIS LINE\n"
    "\n"
    "    fpOutputFile.open( sOutputFile.c_str() );")
note("Type the actual statement exactly as: int iStatus = ITK_ok;  The "
     "'<-- ADD THIS LINE' marker above is only an annotation - do NOT "
     "paste that part into the code.")

doc.add_heading("Step 2.3 - Save the file", level=3)
expected("ClassificationExtraction compiles; all 9 C2065 errors on lines "
         "438-467 disappear. (The C4251 IFail.hxx warnings may remain - "
         "they are harmless and can be ignored.)")
doc.add_page_break()

# ------------------------------------------------------------- fix 3
h1("5. Fix 3 - ClassificationDelete: EXPORT is incompatibly defined "
   "(C1189)")
doc.add_heading("Symptom (from the failed Build_All.bat run)", level=2)
add_code(
    "D:\\Teamcenter_2312\\tc\\include\\ae\\datasettype.h(55,1):\n"
    "  error C1189: #error:  EXPORT is incompatibly defined; use AE_EXPORT\n"
    "  and re-arrange includes\n"
    "(compiling source file 'DBConnector.cpp')")
doc.add_heading("Why it happens", level=2)
p = doc.add_paragraph(
    "When DBConnector.cpp is compiled, its include chain is:")
add_code(
    "DBConnector.cpp\n"
    "  -> DBConnector.hpp\n"
    "       -> standard_defines.hpp        (includes many TC headers:"
    " tcinit, emh, aom, grm, tc_util, ics...)\n"
    "       -> M_Logger.hpp\n"
    "            -> Header.hxx             (only HERE is"
    " <ae/datasettype.h> included)")
p = doc.add_paragraph(
    "By the time ae/datasettype.h is reached, other Teamcenter headers "
    "have already defined the EXPORT macro in an incompatible way, so the "
    "#error guard inside datasettype.h fires. Extraction does not hit this "
    "because its standard_defines.hpp includes ae/datasettype.h EARLY. The "
    "Teamcenter error message itself says what to do: 'use AE_EXPORT and "
    "re-arrange includes'. We re-arrange the includes: AE headers first, "
    "before every other Teamcenter header.")
doc.add_heading("The Fix", level=2)

doc.add_heading("Step 3.1 - Open the file", level=3)
doc.add_paragraph("Open ClassificationDelete\\standard_defines.hpp and find "
                  "this block (after the Windows includes):")
add_code(
    "#include <windows.h>\n"
    "\n"
    "// Teamcenter / ITK\n"
    "#include <tcinit/tcinit.h>")

doc.add_heading("Step 3.2 - Insert the AE headers FIRST", level=3)
doc.add_paragraph(
    "Insert the three AE include lines BETWEEN '#include <windows.h>' and "
    "'// Teamcenter / ITK', so the block reads exactly:")
add_code(
    "#include <windows.h>\n"
    "\n"
    "/*\n"
    "  FIX (C1189 \"EXPORT is incompatibly defined; use AE_EXPORT and"
    " re-arrange\n"
    "  includes\"): when DBConnector.cpp compiles, its include chain is\n"
    "  DBConnector.hpp -> this file -> ... -> M_Logger.hpp -> Header.hxx,"
    " and\n"
    "  ae/datasettype.h only arrived at the very END (via Header.hxx) -"
    " after the\n"
    "  TC headers below had already (transitively) defined EXPORT, so\n"
    "  datasettype.h's own #error check fired.\n"
    "\n"
    "  Including the AE headers FIRST, before every other Teamcenter"
    " header, is\n"
    "  the arrangement the TC error message itself recommends (\"re-arrange\n"
    "  includes\") and mirrors the proven-good order already used in"
    " Header.hxx.\n"
    "*/\n"
    "#include <ae/ae.h>\n"
    "#include <ae/datasettype.h>\n"
    "#include <ae/dataset.h>\n"
    "\n"
    "// Teamcenter / ITK\n"
    "#include <tcinit/tcinit.h>")
note("The 3 required lines are: #include <ae/ae.h> , "
     "#include <ae/datasettype.h> , #include <ae/dataset.h>  - the comment "
     "block documents WHY and can be shortened or omitted. What matters is "
     "that the AE headers come before tcinit/tc_startup/emh/aom/grm/"
     "tc_util/ics.")

doc.add_heading("Step 3.3 - Save the file", level=3)
doc.add_paragraph(
    "Because Header.hxx still contains the same includes later on, nothing "
    "else changes - include guards make the later includes no-ops, but the "
    "AE definitions now exist BEFORE any other TC header is processed.")
expected("ClassificationDelete compiles; the C1189 error in "
         "ae\\datasettype.h disappears while compiling DBConnector.cpp "
         "(and any other source of this project).")
doc.add_page_break()

# ------------------------------------------------------------- 6. rebuild
h1("6. Rebuild and Verify")
doc.add_heading("Step 4.1 - Clean rebuild of the whole solution", level=3)
doc.add_paragraph("Open a Command Prompt and run:")
add_code(
    "D:\\ClassificationUtilities\\Config> Build_All.bat ALL clean")
note("'clean' removes old .obj/.exe output first - recommended so stale "
     "objects cannot mask the fixes. Individual projects can also be "
     "rebuilt alone: Build_All.bat IMPORT clean, Build_All.bat EXTRACTION "
     "clean, Build_All.bat DELETE clean.")

doc.add_heading("Step 4.2 - Expected build output (acceptance checklist)",
                level=3)
tbl = doc.add_table(rows=4, cols=3)
tbl.style = "Table Grid"
hdr = ["Project", "Expected output", "Must NOT appear anymore"]
for i, htxt in enumerate(hdr):
    c = tbl.rows[0].cells[i]
    c.paragraphs[0].text = ""
    run = c.paragraphs[0].add_run(htxt)
    run.bold = True
    shade(c, "D9E2F3")
rows = [
    ("ClassificationImport",
     "'ClassificationImport.vcxproj -> "
     "D:\\ClassificationUtilities\\x64\\Release\\ClassificationImport.exe' "
     "and 'Done Building Project ... ClassificationImport.vcxproj'",
     "LNK2001 (?getObject@@YAHPEBD0PEAI@Z) / LNK1120"),
    ("ClassificationExtraction",
     "'ClassificationExtraction.vcxproj -> ... "
     "ClassificationExtraction.exe' with no errors",
     "C2065 'iStatus': undeclared identifier (lines 438-467)"),
    ("ClassificationDelete",
     "'ClassificationValidation.vcxproj -> ... ClassificationDelete.exe' "
     "with no errors",
     "C1189 'EXPORT is incompatibly defined' in ae\\datasettype.h"),
]
for ri, vals in enumerate(rows, start=1):
    for ci, val in enumerate(vals):
        c = tbl.rows[ri].cells[ci]
        c.paragraphs[0].text = ""
        run = c.paragraphs[0].add_run(val)
        run.font.size = Pt(8.5)
for w, col in zip((Inches(1.4), Inches(2.9), Inches(2.2)), range(3)):
    for cell in tbl.columns[col].cells:
        cell.width = w

doc.add_paragraph()
p = doc.add_paragraph("The overall summary must end with:")
add_code("Build succeeded.\n    0 Warning(s)   <- C4251 IFail warnings may"
         " remain; they are harmless\n    0 Error(s)")

doc.add_heading("Step 4.3 - Smoke test after a successful build", level=3)
doc.add_paragraph("Run each utility with -h to confirm the binaries work:")
add_code(
    "D:\\ClassificationUtilities\\x64\\Release\\ClassificationImport.exe -h\n"
    "D:\\ClassificationUtilities\\x64\\Release\\ClassificationExtraction.exe"
    " -h\n"
    "D:\\ClassificationUtilities\\x64\\Release\\ClassificationDelete.exe -h")
expected("Each exe prints its usage/help text. (Running ITK functions "
         "requires a TC login - use Run.bat / Run_Extraction.bat / "
         "Run_Delete.bat from Config for the real test.)")

# ------------------------------------------------------------- 7. rollback
h1("7. Rollback / Troubleshooting")
for line in (
    "Before editing, copy each of the 3 files to <name>.orig.cpp / "
    ".orig.hpp - then a rollback is a simple file copy.",
    "If LNK2001 getObject still appears after Fix 1: confirm the new "
    "function was pasted at the END of Source.cpp (not inside another "
    "function or inside the ITK_user_main body) and that Source.cpp was "
    "saved. 'All outputs are up-to-date' in the build log means MSBuild "
    "did not recompile - run with 'clean'.",
    "If C2065 iStatus still appears after Fix 2: the declaration must be "
    "inside writeIntoFile(), before the first ITK( call. Check that you "
    "edited ClassificationExtraction\\Source.cpp (not ClassificationImport).",
    "If C1189 EXPORT still appears after Fix 3: verify the 3 AE includes "
    "are in ClassificationDelete\\standard_defines.hpp ABOVE the "
    "'Teamcenter / ITK' block. If a different TC header then complains, "
    "keep the AE-first order and report the new error - the AE-first "
    "arrangement is the supported one per the TC error text.",
    "The C4251 warning on IFail.hxx (m_message std::string / dll-interface) "
    "comes from the Teamcenter headers themselves - it exists in Extraction "
    "already and is safe to ignore.",
):
    doc.add_paragraph(line, style="List Bullet")

doc.save(OUT_PATH)
print("Written:", OUT_PATH)
