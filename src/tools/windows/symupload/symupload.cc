// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Tool to upload an exe/dll and its associated symbols to an HTTP server.
// The PDB file is located automatically, using the path embedded in the
// executable.  The upload is sent as a multipart/form-data POST request,
// with the following parameters:
//  code_file: the basename of the module, e.g. "app.exe"
//  debug_file: the basename of the debugging file, e.g. "app.pdb"
//  debug_identifier: the debug file's identifier, usually consisting of
//                    the guid and age embedded in the pdb, e.g.
//                    "11111111BBBB3333DDDD555555555555F"
//  product: the HTTP-friendly product name, e.g. "MyApp"
//  version: the file version of the module, e.g. "1.2.3.4"
//  os: the operating system that the module was built for, always
//      "windows" in this implementation.
//  cpu: the CPU that the module was built for, typically "x86".
//  symbol_file: the contents of the breakpad-format symbol file

#include <windows.h>
#include <dbghelp.h>
#include <wininet.h>

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include "common/windows/string_utils-inl.h"

#include "common/windows/http_upload.h"

using std::string;
using std::wstring;
using std::vector;
using std::map;
using google_breakpad::HTTPUpload;
using google_breakpad::WindowsStringUtils;

// Extracts the file version information for the given filename,
// as a string, for example, "1.2.3.4".  Returns true on success.
static bool GetFileVersionString(const wchar_t *filename, wstring *version) {
  DWORD handle;
  DWORD version_size = GetFileVersionInfoSize(filename, &handle);
  if (version_size < sizeof(VS_FIXEDFILEINFO)) {
    return false;
  }

  vector<char> version_info(version_size);
  if (!GetFileVersionInfo(filename, handle, version_size, &version_info[0])) {
    return false;
  }

  void *file_info_buffer = NULL;
  unsigned int file_info_length;
  if (!VerQueryValue(&version_info[0], L"\\",
                     &file_info_buffer, &file_info_length)) {
    return false;
  }

  // The maximum value of each version component is 65535 (0xffff),
  // so the max length is 24, including the terminating null.
  wchar_t ver_string[24];
  VS_FIXEDFILEINFO *file_info =
    reinterpret_cast<VS_FIXEDFILEINFO*>(file_info_buffer);
  swprintf(ver_string, sizeof(ver_string) / sizeof(ver_string[0]),
           L"%d.%d.%d.%d",
           file_info->dwFileVersionMS >> 16,
           file_info->dwFileVersionMS & 0xffff,
           file_info->dwFileVersionLS >> 16,
           file_info->dwFileVersionLS & 0xffff);

  // remove when VC++7.1 is no longer supported
  ver_string[sizeof(ver_string) / sizeof(ver_string[0]) - 1] = L'\0';

  *version = ver_string;
  return true;
}

__declspec(noreturn) void printUsageAndExit() {
  wprintf(L"Usage:\n\n"
          L"    symupload [--timeout NN] [--product product_name] ^\n"
          L"              <file.sym> <file.exe|file.dll> <symbol upload URL> ^\n"
          L"              [...<symbol upload URLs>]\n\n");
  wprintf(L"  - Timeout is in milliseconds, or can be 0 to be unlimited.\n");
  wprintf(L"  - product_name is an HTTP-friendly product name. It must only\n"
          L"    contain an ascii subset: alphanumeric and punctuation.\n"
          L"    This string is case-sensitive.\n\n");
  wprintf(L"Example:\n\n"
          L"    symupload.exe --timeout 0 --product Chrome ^\n"
          L"        chrome.dll http://no.free.symbol.server.for.you\n");
  exit(0);
}

int wmain(int argc, wchar_t *argv[]) {
  const wchar_t *symbol_file;
  wstring code_file;
  const wchar_t *product = nullptr;
  int timeout = -1;
  int currentarg = 1;
  while (argc > currentarg + 1) {
    if (!wcscmp(L"--timeout", argv[currentarg])) {
      timeout = _wtoi(argv[currentarg + 1]);
      currentarg += 2;
      continue;
    }
    if (!wcscmp(L"--product", argv[currentarg])) {
      product = argv[currentarg + 1];
      currentarg += 2;
      continue;
    }
    break;
  }

  // extracting symbol file name
  if (argc >= currentarg + 3)
    symbol_file = argv[currentarg++];
  else
    printUsageAndExit();

  // extracting code file name
  if (argc >= currentarg + 2) {
	  const wchar_t *code_file_raw = argv[currentarg++];
	  code_file = WindowsStringUtils::GetBaseName(wstring(code_file_raw));
  }
  else
    printUsageAndExit();

  // extracting symbol and writing to temp file
  /*wstring symbol_file;
  PDBModuleInfo pdb_info;
  if (!DumpSymbolsToTempFile(module, &symbol_file, &pdb_info)) {
    fwprintf(stderr, L"Could not get symbol data from %s\n", module);
    return 1;
  }*/

  //wstring code_file = WindowsStringUtils::GetBaseName(wstring(symbol_file));

  // generating debug identifier
  std::hash<wstring> wstr_hash;
  char buffer[256];
  sprintf(buffer, "%d", wstr_hash(code_file + symbol_file));
  string debug_identifier_raw = string(buffer);
  wstring debug_identifier = wstring(debug_identifier_raw.begin(), debug_identifier_raw.end());

  map<wstring, wstring> parameters;
  parameters[L"code_file"] = code_file;
  parameters[L"debug_file"] = symbol_file;
  parameters[L"debug_identifier"] = debug_identifier; // NB: may not work as intended
  parameters[L"os"] = L"windows";  // This version of symupload is Windows-only
  parameters[L"cpu"] = L"x86";
  
  // Don't make a missing product name a hard error.  Issue a warning and let
  // the server decide whether to reject files without product name.
  if (product) {
    parameters[L"product"] = product;
  } else {
    fwprintf(
        stderr,
        L"Warning: No product name (flag --product) was specified for %s\n",
        symbol_file);
  }

  // Don't make a missing version a hard error.  Issue a warning, and let the
  // server decide whether to reject files without versions.
  wstring file_version;
  if (GetFileVersionString(symbol_file, &file_version)) {
    parameters[L"version"] = file_version;
  } else {
    fwprintf(stderr, L"Warning: Could not get file version for %s\n", symbol_file);
  }

  //map<wstring, wstring> files;
  //files[L"symbol_file"] = symbol_file;

  bool success = true;

  while (currentarg < argc) {
    int response_code;
    if (!HTTPUpload::SendRequest(
		argv[currentarg]
		, parameters
		, symbol_file
		, L"symbol_file"
		, timeout == -1 ? NULL : &timeout
		, nullptr
		, &response_code
	)) {
      success = false;
      fwprintf(stderr,
               L"Symbol file upload to %s failed. Response code = %ld\n",
               argv[currentarg], response_code);
    }
	wprintf(L"Response code = %ld\n", response_code);
    currentarg++;
  }

  //_wunlink(symbol_file.c_str());

  if (success) {
    wprintf(L"Uploaded symbols for windows-x86/%s (%s %s)\n",
            symbol_file, code_file.c_str(),
            file_version.c_str());
  }

  return success ? 0 : 1;
}
