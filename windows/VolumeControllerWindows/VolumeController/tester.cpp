#include <iostream>
#include <Windows.h>


#include "volume_manager.h"

using namespace std;

void makeConsole() {
    AllocConsole();

    // std::cout, std::clog, std::cerr, std::cin
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    // std::wcout, std::wclog, std::wcerr, std::wcin
    HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
}


void scanOutputDevices(VolumeManager& vol) {
    vector<wstring> outputDevices = vol.scanOutputDevices();
    for (size_t i = 0; i < outputDevices.size(); i++) {
        wcout << L"  " << i << L": " << outputDevices[i] << endl;
    }
    cout << " --total " << outputDevices.size() << " devices" << endl;
}

void scanSessions(VolumeManager& vol) {
    vector<AudioSession> sessions = vol.discoverSessions();
    cout << "sessions:" << endl;
    for (int i = 0; i < sessions.size(); i++) {
        AudioSession& session = sessions[i];
        float lvl;
        session.volume->GetMasterVolume(&lvl);
        wcout << L"  " << i << L": " << session.isSystemSounds << L" " << session.filename
            << " pid: " << session.pid << L" vol: " << lvl * 100 << endl;

    }
    cout << " --total " << sessions.size() << " sessions" << endl;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    makeConsole();

    VolumeManager vol;

    vol.initialize();
    scanOutputDevices(vol);
    vol.useDefaultOutputDevice();
    //vol.useOutputDevice(L"Headphones (CR1178)");
    scanSessions(vol);

    do {
        cout << "> ";
        string cmd;
        cin >> cmd;
        if (cmd == "exit") break;
        else if (cmd == "scan") {
            scanOutputDevices(vol);
        }
        else if (cmd == "vol") {
            wstring channel;
            string cmd2;
            wcin >> channel;
            cin >> cmd2;
            if (cmd2 == "set") {
                float level;
                cin >> level;
                level /= 100;
                if (channel == L"master")
                    vol.setOutputDeviceVolume(level);
                else if (channel == L"system")
                    vol.setSessionVolume(L"system", level);
                else
                    vol.setSessionVolume(channel, level);
            }
            else if (cmd2 == "get") {
                if (channel == L"master")
                    cout << " -master: " << floor(vol.getOutputDeviceVolume()*100) << "% " << (vol.getOutputDeviceMute() ? "muted" : "unmuted") << endl;
                else if (channel == L"system")
                    cout << " -system: " << floor(vol.getSessionVolume(L"system") * 100) << "% " << (vol.getSessionMute(L"system") ? "muted" : "unmuted") << endl;
                else {
                    float level = vol.getSessionVolume(channel);
                    if (level == -1)
                        cout << "session does not exist" << endl;
                    else
                        wcout << L" -" << channel << L": " << floor(vol.getSessionVolume(channel)*100) << L"% " << (vol.getSessionMute(channel) ? L"muted" : L"unmuted") << endl;
                }
                    
            }
            else if (cmd2 == "mute") {
                if (channel == L"master")
                    vol.setOutputDeviceMute(true);
                else if (channel == L"system")
                    vol.setSessionMute(L"system", true);
                else
                    vol.setSessionMute(channel, true);
            }
            else if (cmd2 == "unmute") {
                if (channel == L"master")
                    vol.setOutputDeviceMute(false);
                else if (channel == L"system")
                    vol.setSessionMute(L"system", false);
                else
                    vol.setSessionMute(channel, false);
            }
        }
        else if (cmd == "sessions") {
            scanSessions(vol);
        }
        else {
            cout << "unknown command" << endl;
        }
    } while (1);


	return 0;
}