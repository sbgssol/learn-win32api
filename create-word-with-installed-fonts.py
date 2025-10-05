import win32com.client
import os
import sys

# --- Configuration ---
OUTPUT_FILE_PATH = os.path.join(
    os.path.expanduser("~"), "Desktop", "Installed_Fonts_Preview_Python_FINAL.docx"
)
SAMPLE_TEXT = """abcdefghijklmnopqrstuvwxyz  12345 ABCDEFGHIJKLMNOPQRSTUVWXYZ  67890 {}[]()<>$*-+=/#_%^@\&|~?'"`!,.;: the quick brown fox jumps over the lazy dog THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG float Fox.quick(h){ is_brown && it_jumps_over(doges.lazy) } != := -> => || == !== && www *** 0xFF /* <!-- >=<= </> #! i++ 9:0 /* /// |>"""
FONT_SIZE = 14
HEADING_FONT_SIZE = 18
# --- Word COM Constants ---
wdPageBreak = 7
# ---------------------


def release_com_object(obj):
    """
    Safely releases the COM object reference using the internal pywin32 method.
    This replaces pythoncom.Release() which caused the error.
    """
    if obj:
        try:
            # Force the release of the Python COM wrapper's reference
            obj.CLSID.z_delref()
        except Exception:
            pass


def list_installed_fonts(word_app):
    """Lists installed fonts using the Word Application COM object."""
    print("-------------------------------------")
    print("1-> Searching for installed fonts...")

    # Open a temporary document so Word.FontNames works correctly
    temp_doc = word_app.Documents.Add()

    installed_fonts = sorted(f for f in word_app.FontNames if not f.startswith("@"))

    temp_doc.Close(SaveChanges=False)

    print(f"1-> Found {len(installed_fonts)} unique fonts.")
    # installed_fonts = installed_fonts[:10]  # Limit to first 10 fonts for performance
    return installed_fonts


def create_font_document(installed_fonts):
    """Creates and populates the Word document."""
    word = None
    doc = None

    try:
        # 2. Initialize Word
        print("2-> Initializing Microsoft Word...")
        word = win32com.client.Dispatch("Word.Application")
        word.Visible = False

        # Assign doc using the stable return value
        doc = word.Documents.Add()

        selection = word.Selection

        print(f"3-> Starting document generation...")

        # 3 & 4. Populate Document with Font Samples
        rng = doc.Range(0, 0)
        for font_name in installed_fonts:
            try:
                rng.InsertAfter(f"{font_name}\n")
                rng.Font.Size = HEADING_FONT_SIZE
                rng.Font.Bold = True
                rng.Font.Name = "Calibri"

                rng.Collapse(0)  # collapse end
                rng.InsertAfter(SAMPLE_TEXT + "\n")
                rng.Font.Size = FONT_SIZE
                rng.Font.Bold = False
                rng.Font.Name = font_name

                rng.Collapse(0)
                rng.InsertBreak(wdPageBreak)

                print(f"Applied font: {font_name}")

            except Exception as e:
                print(f"WARNING: Font '{font_name}' failed: {e}")
                rng.InsertAfter(f"--- ERROR with {font_name} ---\n")
                rng.Collapse(0)
                rng.InsertBreak(wdPageBreak)

        # --- Save and Clean Up (Executed after the loop completes) ---
        print("-------------------------------------")
        print("Saving document...")

        # Save the document using the stable doc reference
        doc.SaveAs2(OUTPUT_FILE_PATH, FileFormat=12)  # wdFormatXMLDocument

        # Close the document and quit Word
        doc.Close(SaveChanges=False)
        word.Quit()

        print(f"✅ Document successfully created at '{OUTPUT_FILE_PATH}'")
        os.startfile(OUTPUT_FILE_PATH)

        # Explicitly release the COM objects to prevent corruption
        release_com_object(doc)
        release_com_object(word)

    except Exception as main_e:
        print(f"FATAL SCRIPT ERROR (outside loop): {main_e}")
        # Final cleanup attempt
        if doc:
            try:
                doc.Close(SaveChanges=False)
                release_com_object(doc)
            except:
                pass
        if word:
            try:
                word.Quit()
                release_com_object(word)
            except:
                pass


if __name__ == "__main__":
    # Initialize Word once to get the reliable font list, then close it
    temp_word = None
    try:
        temp_word = win32com.client.Dispatch("Word.Application")
        fonts = list_installed_fonts(temp_word)
        temp_word.Quit()
        release_com_object(temp_word)
    except Exception as e:
        print(f"Could not initialize Word to list fonts. Error: {e}")
        sys.exit(1)

    create_font_document(fonts)
