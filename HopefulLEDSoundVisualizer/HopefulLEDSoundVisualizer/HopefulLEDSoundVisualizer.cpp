#include <stdio.h>
#include <stdlib.h>
#include <atlbase.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <math.h>

#define LOG(format, ...) wprintf_s(format L"\n", __VA_ARGS__)

HANDLE InitializeSerialCommunication() {

    DCB dcbSerialParams;
    COMMTIMEOUTS timeouts;
    char SerialPortIn[8];
    char SerialPortStr[16];

    printf_s("Enter the COM Port: ");
    scanf_s("%s", SerialPortIn, (unsigned)_countof(SerialPortIn));
    sprintf_s(SerialPortStr, sizeof(SerialPortStr), "\\\\.\\%s", SerialPortIn);

    HANDLE hComm = CreateFileA(SerialPortStr, GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
    if (hComm == INVALID_HANDLE_VALUE)
    {
        printf_s("ERROR: Failed to open communication to port %s.\n", SerialPortIn);
        return hComm;
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (GetCommState(hComm, &dcbSerialParams) == FALSE)
    {
        printf_s("ERROR: Unable to retrieve communication state data.\n");
        CloseHandle(hComm);
        return INVALID_HANDLE_VALUE;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (SetCommState(hComm, &dcbSerialParams) == FALSE)
    {
        printf_s("ERROR: Unable to set communication state data.\n");
        CloseHandle(hComm);
        return INVALID_HANDLE_VALUE;
    }

    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (SetCommTimeouts(hComm, &timeouts) == FALSE)
    {
        printf_s("ERROR: Unable to set communication timeouts.\n");
        CloseHandle(hComm);
        return INVALID_HANDLE_VALUE;
    }

    return hComm;
}

int main()
{
    LOG(L"Initializing Serial Communication...");
    HANDLE hComm = InitializeSerialCommunication();
    if (hComm == INVALID_HANDLE_VALUE)
    {
        LOG(L"Serial Communication Initialization Error. Exiting.");
        return -__LINE__;
    }
    LOG(L"Serial Communication Initialized.\n");

    HRESULT hr = S_OK;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        LOG(L"CoInitialize failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

    CComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;
    hr = pMMDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
    if (FAILED(hr))
    {
        LOG(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

    CComPtr<IMMDevice> pIMMDevice;
    hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pIMMDevice);
    if (FAILED(hr))
    {
        LOG(L"IMMDeviceEnumerator::GetDefaultAudioEndpoint failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

    CComPtr<IAudioMeterInformation> pAudioMeterInformationEndpoint;
    hr = pIMMDevice->Activate(__uuidof(IAudioMeterInformation), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&pAudioMeterInformationEndpoint));
    if (FAILED(hr))
    {
        LOG(L"IMMDevice::Activate(IAudioMeterInformation) failed: hr = 0x%08x", hr);
        return -__LINE__;
    }

    int LastPeakLevel = 0;
    while (true)
    {
        float peak_endpoint = 0.0f;
        hr = pAudioMeterInformationEndpoint->GetPeakValue(&peak_endpoint);
        if (FAILED(hr))
        {
            LOG(L"IAudioMeterInformation::GetPeakValue() failed: hr = 0x%08x", hr);
            return -__LINE__;
        }
        int CurrentPeakLevel = (int)(peak_endpoint * 100);
        printf_s("Current Peak Level: %03d%%\r", CurrentPeakLevel);

        if (LastPeakLevel == CurrentPeakLevel)
        {
            Sleep(10);
            continue;
        }
        LastPeakLevel = CurrentPeakLevel;
        char peakLevel[1] = { (char)CurrentPeakLevel };

        if (WriteFile(hComm, peakLevel, sizeof(peakLevel), nullptr, NULL) == FALSE)
        {
            LOG(L"Failed to write data to communication port.");
            break;
        }

        Sleep(10);
    }

    CloseHandle(hComm);
    CoUninitialize();
}
