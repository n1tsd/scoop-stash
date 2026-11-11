/*
 * recshim - native command entries for the reclaude Scoop package.
 *
 * One file, hardlinked as reclaude.exe / rec.exe / recodex.exe / recx.exe in
 * the app directory, next to the real reclaude-windows-<arch>.exe.
 *
 *  - Forwards to the real binary passing the ORIGINAL command line through
 *    untouched with lpApplicationName pointed at the real exe, so the child
 *    still sees this entry's own path as argv[0]. The filename-based
 *    reclaude/recodex dispatch behaves exactly like the official entries
 *    (official setup also just copies the binary under entry names).
 *  - Blocks the `update`, `setup` and `uninstall` subcommands: they would
 *    bypass Scoop and overwrite/copy managed files. `recodex app update`
 *    (ChatGPT desktop app updater) passes, since its first arg is `app`.
 *  - Ignores console Ctrl+C itself while waiting on the child, which handles
 *    the key natively - no wrapper dies early, no "Terminate batch job".
 *
 * Build (same flags locally and in CI, see shim/build.ps1):
 *   zig cc -target x86_64-windows-gnu -Os -lkernel32 -lshell32
 */

#include <windows.h>
#include <shellapi.h>

static const char BLOCKED_MSG[] =
    "reclaude: 'update', 'setup' and 'uninstall' are disabled: this copy is managed by Scoop.\r\n"
    "reclaude: use: scoop update reclaude  /  scoop uninstall reclaude\r\n";

static const char NO_TARGET_MSG[] =
    "recshim: reclaude-windows-*.exe not found next to this entry\r\n";

static void write_out(const char *s)
{
    DWORD n = (DWORD)lstrlenA(s);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), s, n, &n, NULL);
}

static int is_blocked_subcommand(const WCHAR *s)
{
    return !lstrcmpiW(s, L"update") || !lstrcmpiW(s, L"setup") || !lstrcmpiW(s, L"uninstall");
}

int main(void)
{
    /* static = zero-initialized in BSS: no init code needed */
    static WCHAR self[MAX_PATH];
    static WCHAR cmdline[32768];
    static WCHAR target[MAX_PATH];
    static STARTUPINFOW si;
    static PROCESS_INFORMATION pi;
    WIN32_FIND_DATAW fd;
    HANDLE h;
    WCHAR *slash;
    LPWSTR *argv_list;
    int argc;
    DWORD dirlen;
    DWORD code = 1;

    /* entry identity = our own module path; it is what the child will see
       as argv[0] because the command line below is forwarded verbatim */
    if (!GetModuleFileNameW(NULL, self, MAX_PATH))
        return 1;

    /* block Scoop-bypassing subcommands (first arg only) */
    argv_list = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv_list) {
        int blocked = (argc >= 2) && is_blocked_subcommand(argv_list[1]);
        LocalFree(argv_list);
        if (blocked) {
            write_out(BLOCKED_MSG);
            return 1;
        }
    }

    /* directory of self */
    slash = self;
    for (WCHAR *p = self; *p; p++)
        if (*p == L'\\' || *p == L'/')
            slash = p + 1;
    dirlen = (DWORD)(slash - self);

    /* locate reclaude-windows-*.exe next to this entry */
    lstrcpynW(target, self, (int)dirlen + 1);
    lstrcpynW(target + dirlen, L"reclaude-windows-*.exe", MAX_PATH - (int)dirlen);
    h = FindFirstFileW(target, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        write_out(NO_TARGET_MSG);
        return 1;
    }
    FindClose(h);
    lstrcpynW(target + dirlen, fd.cFileName, MAX_PATH - (int)dirlen);

    /* forward the original command line byte-for-byte: lpApplicationName
       decides what runs, the command line decides argv[0] and all args */
    lstrcpynW(cmdline, GetCommandLineW(), (int)(sizeof(cmdline) / sizeof(WCHAR)));

    /* never die on Ctrl+C before the child is done; the child handles it */
    SetConsoleCtrlHandler(NULL, TRUE);

    si.cb = sizeof(si);
    if (CreateProcessW(target, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return (int)code;
}
