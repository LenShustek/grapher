#pragma once

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wingdi.h>  // the old GDI routines

#include "Resource.h"

// C++ crap from the template we started with

template<class Interface>
inline void
SafeRelease(
   Interface **ppInterfaceToRelease) {
   if (*ppInterfaceToRelease != NULL) {
      (*ppInterfaceToRelease)->Release();
      (*ppInterfaceToRelease) = NULL; } }

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

class grapherApp {
public:
   grapherApp();
   ~grapherApp();
   HRESULT Initialize();

private:
   HRESULT CreateDeviceIndependentResources();
   static void create_plot_window(HWND handle);
   //static void create_Vpopup_window(HWND handle);
   static void create_marker_window(HWND handle);
   static void create_label_window(HWND handle);
   //HRESULT CreateDeviceResources(HWND handle);
   static HRESULT CreateWindowResources(struct window_t *ww);
   void DiscardDeviceResources();
   //void drawsine(int ncycles, int npts, D2D1_SIZE_F targetsize);
   HRESULT drawplots();
   //void OnResize(UINT width, UINT height);

   static LRESULT CALLBACK WndProc(
      HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK WndProcPlot(
      HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK WndProcMarker(
      HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK WndProcLabel(
      HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
   static LRESULT CALLBACK WndProcVpopup(
      HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
   //HWND m_hwnd;
   ID2D1Factory *m_pD2DFactory;
   IWICImagingFactory *m_pWICFactory;
   IDWriteFactory *m_pDWriteFactory;
   //ID2D1HwndRenderTarget *m_pRenderTarget;
   IDWriteTextFormat *m_pTextFormat;
   ID2D1SolidColorBrush *m_pBlackBrush; };

//*
