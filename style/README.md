# Application styles

The Application Style menu enumerates `.qss` files in the installed resource
root's style directory. With the supplied project metadata:

- Linux: `<PREFIX>/share/NotQuiteRSS/style` (normally `/usr/local/share/NotQuiteRSS/style`).
- Windows, including portable mode: `style` beside `notquiterss.exe`.
- macOS: `NotQuiteRSS.app/Contents/Resources/style`.

All three platforms deploy this directory through qmake's normal install/bundle
rules. Windows CI runs `mingw32-make install` and verifies each source QSS file
against its deployed copy before creating the release artifact. Article `.css`
files also remain deployed, but do not appear in Application Style.

Add a UTF-8 `.qss` file, then open View > Application Style. The menu refreshes
on each opening, so adding/removing files requires no restart or rebuild.
Selecting a style loads its current contents immediately. Editing an already
selected file requires selecting it again. Use absolute or `:/` resource URLs
for images; relative URLs retain Qt's normal working-directory semantics.

An optional header at the beginning of the file supplies metadata:

```css
/* ApplicationStyle
Name=My dark style
Id=my-dark-style
Colors=dark
Default=false
*/
QWidget {
    background-color: #464546;
    color: #e1e0e1;
}
```

Keys and values are case-sensitive. Each key may appear once; unknown keys,
empty values, and invalid Colors/Default values reject the file with a warning.
With no header (or omitted fields), Name and Id default to the filename without
`.qss`, Colors to `standard`, and Default to `false`. IDs must be unique and
nonempty. Keep an explicit Id stable if you rename a file. Duplicate IDs are
skipped deterministically in filename order. Names use the existing MainWindow
translation context where a translation is available; otherwise they display
as written.

`Default=true` selects a file only when no style preference has been saved.
If multiple files declare it, the first in filename order wins with a warning.
The bundled Green file retains the previous fresh-install default. Bundled IDs
are preserved in their headers so existing selections continue to resolve.

The always-available **System default** entry uses the embedded system QSS and
has an empty saved ID. Missing, unreadable or rejected selected files fall back
to this entry and normalize `Settings/styleApplication`. A missing style does
not silently select an unrelated external file. User color preferences remain
independent and are not overwritten during startup or automatic fallback.

# Appearance responsibilities

- The native Qt widget style / QProxyStyle is initialized once, as before.
  Selecting QSS does not replace it or select Fusion/another Qt widget style.
- The QSS loader owns directory discovery, metadata and file reading. It does
  not access MainWindow widgets or settings.
- Explicit style selection still resets application-specific article, list and
  notification colors, matching the old behavior. `Colors=dark` chooses the
  existing dark profile and loading animation; `standard` chooses the existing
  standard profile. MainWindow applies those colors separately from QSS.
  Startup preserves saved/customized colors.

# Diagnostics and verification

Missing directories, file read failures, malformed metadata, duplicate IDs,
and empty/unbalanced QSS produce warnings. The applied file path is logged.
Qt's own QSS syntax/property warnings continue through the application logger.
The structural check handles comments, quoted strings and brace balance; it
is not a complete QSS parser. Qt's public `setStyleSheet()` API returns no
success status, so arbitrary grammar/property errors cannot reliably trigger
an automatic fallback without using private Qt APIs or intercepting logging.
No private Qt API or process-wide message-handler replacement is used.

The optional `tests/applicationstyle/applicationstyle.pro` Qt Test project
supports both Qt5 and Qt6. It covers discovery, refresh after adding/removing
files, metadata, duplicate IDs and malformed input. It is separate from the
application build and requires the matching Qt Test development package.

Manual checks for each Qt major version/platform:

1. Build and install to a clean destination. Launch from a different working
   directory and select every bundled style; check immediate visible changes.
2. Add a new QSS file with a distinctive widget color. Reopen the menu, select
   it, restart, and verify its selection persists.
3. Remove the selected file, then reopen the menu or restart. Verify System
   default is selected; also test an absent style directory and malformed QSS.
4. Switch Dark/standard profiles and check article/list/notification colors,
   loading animation, and customized colors surviving an ordinary restart.
5. Reopen the menu several times and select once: one application-QSS log entry
   should appear, with no duplicate dispatch or crash on shutdown.

The patch was checked statically; application compilation/runtime tests and
these Qt Test executables were not run during patch preparation.
