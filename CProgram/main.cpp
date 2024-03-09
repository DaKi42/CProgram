#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include <CommCtrl.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime> 
using namespace std;

wstring targetWord;
HWND progressBarHandle;

int timerDuration;
bool isTargetWordFound;
bool isOperationActive = true;

void ResetSummaryFile()
{
    ofstream outputFile("res_sum.txt", ios::out | ios::trunc);
    if (outputFile.is_open()) {
        outputFile.close();
        isOperationActive = false;
    }
    else {
        MessageBox(NULL, L"Unable to reset the file 'res_sum.txt'!", L"Operation Failed", MB_OK | MB_ICONERROR);
    }
}

wstring CensorWord(const wstring& text, const wstring& searchWord, int& censorCount)
{
    wstring modifiedText = text;
    size_t currentPosition = 0;
    while ((currentPosition = modifiedText.find(searchWord, currentPosition)) != wstring::npos) {
        modifiedText.replace(currentPosition, searchWord.length(), searchWord.length(), L'*');
        currentPosition += searchWord.length();
        censorCount++;
    }
    return modifiedText;
}

void DisplaySearchResult(HWND windowHandle, bool found, const wstring& searchTerm)
{
    if (found) {
        wstring alertMessage = L"Found '" + searchTerm + L"' in files";
        MessageBox(windowHandle, alertMessage.c_str(), L"Search Result", MB_OK | MB_ICONINFORMATION);
    }
    else {
        wstring alertMessage = L"No '" + searchTerm + L"' found in files";
        MessageBox(windowHandle, alertMessage.c_str(), L"Search Result", MB_OK | MB_ICONINFORMATION);
    }
}

void ProcessAndSearchFile(const wstring& filePath, const wstring& searchTerm, wofstream& outputFile, int& censorCount, bool& found)
{
    ifstream inputFile(filePath, ios::binary | ios::ate);
    if (inputFile.is_open()) {
        streampos fileSize = inputFile.tellg();
        inputFile.seekg(0, ios::beg);
        string inputLine;
        while (getline(inputFile, inputLine)) {
            wstring wideLine(inputLine.begin(), inputLine.end());
            if (wideLine.find(searchTerm) != wstring::npos) {
                wideLine = CensorWord(wideLine, searchTerm, censorCount);
                found = true;

                outputFile << wideLine << endl;
            }
        }
        if (censorCount > 0) {
            outputFile << "######################################" << endl;
            outputFile << "File: " << filePath << " - (" << fileSize << " bytes)" << endl;
            outputFile << "Words censored: " << censorCount << endl;
            outputFile << "######################################" << endl << endl;
        }
        inputFile.close();
    }
}

void TraverseDirectoryForFiles(const wstring& directoryPath, const wstring& searchTerm)
{
    bool foundFlag = false;
    int totalCensorCount = 0;
    wofstream summaryFile("res_sum.txt", ios::out | ios::trunc);
    if (summaryFile.is_open()) {
        WIN32_FIND_DATA fileData;
        HANDLE findHandle = FindFirstFile((directoryPath + L"\\*.txt").c_str(), &fileData);
        if (findHandle != INVALID_HANDLE_VALUE) {
            do {
                wstring currentFile = fileData.cFileName;
                int fileCensorCount = 0;
                ProcessAndSearchFile(currentFile, searchTerm, summaryFile, fileCensorCount, foundFlag);
                totalCensorCount += fileCensorCount;
                isTargetWordFound = foundFlag;
            } while (FindNextFile(findHandle, &fileData) != 0);
            FindClose(findHandle);
        }
        summaryFile.close();
    }
    else {
        MessageBox(NULL, L"Failed to open the summary file 'res_sum.txt'!", L"Operation Failed", MB_OK | MB_ICONERROR);
    }
}

DWORD WINAPI ThreadSearchFunction(LPVOID param)
{
    HWND textInputHandle = GetDlgItem((HWND)param, IDC_EDIT1);
    if (textInputHandle != NULL) {
        int inputLength = GetWindowTextLength(textInputHandle);
        if (inputLength > 0) {
            wchar_t* buffer = new wchar_t[inputLength + 1];
            GetWindowText(textInputHandle, buffer, inputLength + 1);
            targetWord = buffer;
            delete[] buffer;
            TraverseDirectoryForFiles(L".", targetWord);
        }
        if (inputLength > 0) {
            srand(time(0));
            timerDuration = rand() % 2 + 2;
            SendMessage(progressBarHandle, PBM_SETRANGE, 0, MAKELPARAM(0, timerDuration));
            SetTimer((HWND)param, 1, 1000, NULL);
        }
    }
    return 0;
}

void InitiateSearchThread(HWND windowHandle)
{
    HANDLE searchThread = CreateThread(NULL, 0, ThreadSearchFunction, windowHandle, 0, NULL);
    if (searchThread == NULL) {
        MessageBox(NULL, L"Thread creation failed!", L"Operation Failed", MB_OK | MB_ICONERROR);
    }
    else {
        CloseHandle(searchThread);
    }
}

int CALLBACK DialogProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        srand(time(0));
        progressBarHandle = GetDlgItem(windowHandle, IDC_PROGRESS1);
        SendMessage(progressBarHandle, PBM_SETRANGE, 0, MAKELPARAM(0, 2));
        SendMessage(progressBarHandle, PBM_SETSTEP, 1, 0);
        SendMessage(progressBarHandle, PBM_SETPOS, 0, 0);
        SendMessage(progressBarHandle, PBM_SETBKCOLOR, 0, LPARAM(RGB(255, 255, 255)));
        SendMessage(progressBarHandle, PBM_SETBARCOLOR, 0, LPARAM(RGB(122, 122, 122)));
    }
    break;

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {

        case IDC_BUTTON1: {
            InitiateSearchThread(windowHandle);
        }
        break;

        case IDC_BUTTON2: {
            ResetSummaryFile();
            KillTimer(windowHandle, 1);
            SendMessage(progressBarHandle, PBM_SETPOS, 0, 0);
            isOperationActive = true;
        }
        break;

        }
        break;
    }
    case WM_TIMER: {
        int progressBarPosition = SendMessage(progressBarHandle, PBM_GETPOS, 0, 0);
        if (progressBarPosition < timerDuration) {
            SendMessage(progressBarHandle, PBM_STEPIT, 0, 0);
        }
        else {
            KillTimer(windowHandle, 1);
            SendMessage(progressBarHandle, PBM_SETBKCOLOR, 0, LPARAM(RGB(255, 255, 255)));
            SendMessage(progressBarHandle, PBM_SETPOS, 0, 0);

            if (isOperationActive && targetWord.size() > 0) {
                DisplaySearchResult(windowHandle, isTargetWordFound, targetWord);
            }
        }
        break;
    }

    case WM_CLOSE: {
        EndDialog(windowHandle, 0);
        return TRUE;
    }

    }
    return FALSE;
}

int WINAPI _tWinMain(HINSTANCE instanceHandle, HINSTANCE prevInstanceHandle, LPTSTR commandLine, int commandShow) {
    return DialogBox(instanceHandle, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DialogProc);
}
