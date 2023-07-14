#include <openssl/evp.h>
#include <stdio.h>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

static BOOL PerformWmiQuery(LPCWCH szClass, LPCWCH szProperty, LPWCH lpResult, DWORD nResult) {
    HRESULT hres;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject *pEnumerator = NULL;
    BOOL bSuccess = FALSE;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return FALSE;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) goto cleanup;

    hres = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                            (LPVOID *)&pLoc);
    if (FAILED(hres)) goto cleanup;

    hres =
        pLoc->lpVtbl->ConnectServer(pLoc, L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);

    if (FAILED(hres)) goto cleanup;

    hres = CoSetProxyBlanket((IUnknown *)pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                             RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) goto cleanup;

    WCHAR szQuery[128];
    swprintf_s(szQuery, _countof(szQuery), L"SELECT * FROM %ls", szClass);

    hres = pSvc->lpVtbl->ExecQuery(pSvc, L"WQL", szQuery,
                                   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL,
                                   &pEnumerator);
    if (FAILED(hres)) goto cleanup;

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (!uReturn) break;

        VARIANT vtProp;
        hr = pclsObj->lpVtbl->Get(pclsObj, szProperty, 0, &vtProp, 0, 0);

        if (SUCCEEDED(hr) && vtProp.vt != VT_NULL) {
            wcscpy_s(lpResult, nResult, vtProp.bstrVal);
            bSuccess = TRUE;
        }

        VariantClear(&vtProp);
        pclsObj->lpVtbl->Release(pclsObj);
    }

    pEnumerator->lpVtbl->Release(pEnumerator);

cleanup:
    if (pSvc) pSvc->lpVtbl->Release(pSvc);
    if (pLoc) pLoc->lpVtbl->Release(pLoc);
    CoUninitialize();
    return bSuccess;
}

inline static void alphaNum(CHAR *lpAddr, BYTE val) {
    // I and O are disallowed, hence the extra logic
    val %= (26 - 2) + 10;
    if (val < 10)
        lpAddr[0] = '0' + val;
    else if (val < 18)
        lpAddr[0] = 'A' + (val - 10);
    else if (val < 23)
        lpAddr[0] = 'J' + (val - 18);
    else
        lpAddr[0] = 'P' + (val - 23);
}
inline static void numeric(CHAR *lpAddr, BYTE val) {
    val %= 100;
    lpAddr[0] = '0' + (val / 10);
    lpAddr[1] = '0' + (val % 10);
}

extern BOOL g_bIsInDllMain;
BOOL GetSerialNumbers(LPSTR szMainId, LPSTR szKeyId) {
    // Making WMI queries from within DllMain is not happening. Ever. :D
    if (g_bIsInDllMain) return FALSE;

    WCHAR szMbSerial[128];
    if (!PerformWmiQuery(L"Win32_BaseBoard", L"SerialNumber", szMbSerial, _countof(szMbSerial)))
        return FALSE;
    szMbSerial[127] = L'\0';
    WCHAR szCpuId[128];
    if (!PerformWmiQuery(L"Win32_Processor", L"ProcessorId", szCpuId, _countof(szCpuId)))
        return FALSE;
    szCpuId[127] = L'\0';

    BYTE sumOut[20];
    unsigned int outlen;

    EVP_MD_CTX *ctx;
    ctx = EVP_MD_CTX_create();
    EVP_DigestInit(ctx, EVP_sha1());
    EVP_DigestUpdate(ctx, szMbSerial, wcslen(szMbSerial));
    EVP_DigestUpdate(ctx, szCpuId, wcslen(szCpuId));
    EVP_DigestFinal_ex(ctx, sumOut, &outlen);
    EVP_MD_CTX_destroy(ctx);

    if (szMainId) {
        szMainId[0] = 'M';
        alphaNum(&szMainId[1], sumOut[0]);
        alphaNum(&szMainId[2], sumOut[1]);
        alphaNum(&szMainId[3], sumOut[2]);
        szMainId[4] = '-';
        alphaNum(&szMainId[5], sumOut[3]);
        alphaNum(&szMainId[6], sumOut[4]);
        alphaNum(&szMainId[7], sumOut[5]);
        numeric(&szMainId[8], sumOut[6]);
        numeric(&szMainId[10], sumOut[7]);
        numeric(&szMainId[12], sumOut[8]);
        numeric(&szMainId[14], sumOut[9]);
        szMainId[16] = '\0';
    }

    if (szKeyId) {
        szKeyId[0] = 'M';
        alphaNum(&szKeyId[1], sumOut[10]);
        alphaNum(&szKeyId[2], sumOut[11]);
        alphaNum(&szKeyId[3], sumOut[12]);
        szKeyId[4] = '-';
        alphaNum(&szKeyId[5], sumOut[13]);
        alphaNum(&szKeyId[6], sumOut[14]);
        alphaNum(&szKeyId[7], sumOut[15]);
        numeric(&szKeyId[8], sumOut[16]);
        numeric(&szKeyId[10], sumOut[17]);
        numeric(&szKeyId[12], sumOut[18]);
        numeric(&szKeyId[14], sumOut[19]);
        szKeyId[16] = '\0';
    }

    return TRUE;
}
