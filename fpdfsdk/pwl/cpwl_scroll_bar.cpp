// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/pwl/cpwl_scroll_bar.h"

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

#include "core/fxge/cfx_fillrenderoptions.h"
#include "core/fxge/cfx_pathdata.h"
#include "core/fxge/cfx_renderdevice.h"
#include "fpdfsdk/pwl/cpwl_wnd.h"
#include "third_party/base/check.h"
#include "third_party/base/stl_util.h"

namespace {

constexpr float kButtonWidth = 9.0f;
constexpr float kPosButtonMinWidth = 2.0f;

}  // namespace

void PWL_FLOATRANGE::Reset() {
  fMin = 0.0f;
  fMax = 0.0f;
}

void PWL_FLOATRANGE::Set(float min, float max) {
  fMin = std::min(min, max);
  fMax = std::max(min, max);
}

bool PWL_FLOATRANGE::In(float x) const {
  return (IsFloatBigger(x, fMin) || IsFloatEqual(x, fMin)) &&
         (IsFloatSmaller(x, fMax) || IsFloatEqual(x, fMax));
}

float PWL_FLOATRANGE::GetWidth() const {
  return fMax - fMin;
}

PWL_SCROLL_PRIVATEDATA::PWL_SCROLL_PRIVATEDATA() {
  Default();
}

void PWL_SCROLL_PRIVATEDATA::Default() {
  ScrollRange.Reset();
  fScrollPos = ScrollRange.fMin;
  fClientWidth = 0;
  fBigStep = 10;
  fSmallStep = 1;
}

void PWL_SCROLL_PRIVATEDATA::SetScrollRange(float min, float max) {
  ScrollRange.Set(min, max);

  if (IsFloatSmaller(fScrollPos, ScrollRange.fMin))
    fScrollPos = ScrollRange.fMin;
  if (IsFloatBigger(fScrollPos, ScrollRange.fMax))
    fScrollPos = ScrollRange.fMax;
}

void PWL_SCROLL_PRIVATEDATA::SetClientWidth(float width) {
  fClientWidth = width;
}

void PWL_SCROLL_PRIVATEDATA::SetSmallStep(float step) {
  fSmallStep = step;
}

void PWL_SCROLL_PRIVATEDATA::SetBigStep(float step) {
  fBigStep = step;
}

bool PWL_SCROLL_PRIVATEDATA::SetPos(float pos) {
  if (ScrollRange.In(pos)) {
    fScrollPos = pos;
    return true;
  }
  return false;
}

void PWL_SCROLL_PRIVATEDATA::AddSmall() {
  if (!SetPos(fScrollPos + fSmallStep))
    SetPos(ScrollRange.fMax);
}

void PWL_SCROLL_PRIVATEDATA::SubSmall() {
  if (!SetPos(fScrollPos - fSmallStep))
    SetPos(ScrollRange.fMin);
}

void PWL_SCROLL_PRIVATEDATA::AddBig() {
  if (!SetPos(fScrollPos + fBigStep))
    SetPos(ScrollRange.fMax);
}

void PWL_SCROLL_PRIVATEDATA::SubBig() {
  if (!SetPos(fScrollPos - fBigStep))
    SetPos(ScrollRange.fMin);
}

CPWL_SBButton::CPWL_SBButton(
    const CreateParams& cp,
    std::unique_ptr<IPWL_SystemHandler::PerWindowData> pAttachedData,
    PWL_SBBUTTON_TYPE eButtonType)
    : CPWL_Wnd(cp, std::move(pAttachedData)), m_eSBButtonType(eButtonType) {
  GetCreationParams()->eCursorType = FXCT_ARROW;
}

CPWL_SBButton::~CPWL_SBButton() = default;

void CPWL_SBButton::DrawThisAppearance(CFX_RenderDevice* pDevice,
                                       const CFX_Matrix& mtUser2Device) {
  if (!IsVisible())
    return;

  CFX_FloatRect rectWnd = GetWindowRect();
  if (rectWnd.IsEmpty())
    return;

  CFX_PointF ptCenter = GetCenterPoint();
  int32_t nTransparency = GetTransparency();

  // draw border
  pDevice->DrawStrokeRect(mtUser2Device, rectWnd,
                          ArgbEncode(nTransparency, 100, 100, 100), 0.0f);
  pDevice->DrawStrokeRect(mtUser2Device, rectWnd.GetDeflated(0.5f, 0.5f),
                          ArgbEncode(nTransparency, 255, 255, 255), 1.0f);

  if (m_eSBButtonType != PSBT_POS) {
    // draw background
    pDevice->DrawShadow(mtUser2Device, true, false,
                        rectWnd.GetDeflated(1.0f, 1.0f), nTransparency, 80,
                        220);
    // draw arrow
    if (rectWnd.top - rectWnd.bottom > 6.0f) {
      float fX = rectWnd.left + 1.5f;
      float fY = rectWnd.bottom;
      std::vector<CFX_PointF> pts;
      static constexpr float kOffsetsX[] = {2.5f, 2.5f, 4.5f, 6.5f,
                                            6.5f, 4.5f, 2.5f};
      static constexpr float kOffsetsY[] = {5.0f, 6.0f, 4.0f, 6.0f,
                                            5.0f, 3.0f, 5.0f};
      static constexpr float kOffsetsMinY[] = {4.0f, 3.0f, 5.0f, 3.0f,
                                               4.0f, 6.0f, 4.0f};
      static_assert(pdfium::size(kOffsetsX) == pdfium::size(kOffsetsY),
                    "Wrong offset count");
      static_assert(pdfium::size(kOffsetsX) == pdfium::size(kOffsetsMinY),
                    "Wrong offset count");
      const float* pOffsetsY =
          m_eSBButtonType == PSBT_MIN ? kOffsetsMinY : kOffsetsY;
      for (size_t i = 0; i < pdfium::size(kOffsetsX); ++i)
        pts.push_back(CFX_PointF(fX + kOffsetsX[i], fY + pOffsetsY[i]));
      pDevice->DrawFillArea(mtUser2Device, pts,
                            ArgbEncode(nTransparency, 255, 255, 255));
    }
    return;
  }

  // draw shadow effect
  CFX_PointF ptTop = CFX_PointF(rectWnd.left, rectWnd.top - 1.0f);
  CFX_PointF ptBottom = CFX_PointF(rectWnd.left, rectWnd.bottom + 1.0f);

  ptTop.x += 1.5f;
  ptBottom.x += 1.5f;

  const FX_COLORREF refs[] = {ArgbEncode(nTransparency, 210, 210, 210),
                              ArgbEncode(nTransparency, 220, 220, 220),
                              ArgbEncode(nTransparency, 240, 240, 240),
                              ArgbEncode(nTransparency, 240, 240, 240),
                              ArgbEncode(nTransparency, 210, 210, 210),
                              ArgbEncode(nTransparency, 180, 180, 180),
                              ArgbEncode(nTransparency, 150, 150, 150),
                              ArgbEncode(nTransparency, 150, 150, 150),
                              ArgbEncode(nTransparency, 180, 180, 180),
                              ArgbEncode(nTransparency, 210, 210, 210)};
  for (FX_COLORREF ref : refs) {
    pDevice->DrawStrokeLine(&mtUser2Device, ptTop, ptBottom, ref, 1.0f);

    ptTop.x += 1.0f;
    ptBottom.x += 1.0f;
  }

  // draw friction
  if (rectWnd.Height() <= 8.0f)
    return;

  FX_COLORREF crStroke = ArgbEncode(nTransparency, 120, 120, 120);
  float nFrictionWidth = 5.0f;
  float nFrictionHeight = 5.5f;
  CFX_PointF ptLeft = CFX_PointF(ptCenter.x - nFrictionWidth / 2.0f,
                                 ptCenter.y - nFrictionHeight / 2.0f + 0.5f);
  CFX_PointF ptRight = CFX_PointF(ptCenter.x + nFrictionWidth / 2.0f,
                                  ptCenter.y - nFrictionHeight / 2.0f + 0.5f);

  for (size_t i = 0; i < 3; ++i) {
    pDevice->DrawStrokeLine(&mtUser2Device, ptLeft, ptRight, crStroke, 1.0f);
    ptLeft.y += 2.0f;
    ptRight.y += 2.0f;
  }
}

bool CPWL_SBButton::OnLButtonDown(uint32_t nFlag, const CFX_PointF& point) {
  CPWL_Wnd::OnLButtonDown(nFlag, point);

  if (CPWL_Wnd* pParent = GetParentWindow())
    pParent->NotifyLButtonDown(this, point);

  m_bMouseDown = true;
  SetCapture();

  return true;
}

bool CPWL_SBButton::OnLButtonUp(uint32_t nFlag, const CFX_PointF& point) {
  CPWL_Wnd::OnLButtonUp(nFlag, point);

  if (CPWL_Wnd* pParent = GetParentWindow())
    pParent->NotifyLButtonUp(this, point);

  m_bMouseDown = false;
  ReleaseCapture();

  return true;
}

bool CPWL_SBButton::OnMouseMove(uint32_t nFlag, const CFX_PointF& point) {
  CPWL_Wnd::OnMouseMove(nFlag, point);

  if (CPWL_Wnd* pParent = GetParentWindow())
    pParent->NotifyMouseMove(this, point);

  return true;
}

CPWL_ScrollBar::CPWL_ScrollBar(
    const CreateParams& cp,
    std::unique_ptr<IPWL_SystemHandler::PerWindowData> pAttachedData)
    : CPWL_Wnd(cp, std::move(pAttachedData)) {
  GetCreationParams()->eCursorType = FXCT_ARROW;
}

CPWL_ScrollBar::~CPWL_ScrollBar() = default;

void CPWL_ScrollBar::OnDestroy() {
  // Until cleanup takes place in the virtual destructor for CPWL_Wnd
  // subclasses, implement the virtual OnDestroy method that does the
  // cleanup first, then invokes the superclass OnDestroy ... gee,
  // like a dtor would.
  m_pMinButton.Release();
  m_pMaxButton.Release();
  m_pPosButton.Release();
  CPWL_Wnd::OnDestroy();
}

bool CPWL_ScrollBar::RePosChildWnd() {
  CFX_FloatRect rcClient = GetClientRect();
  CFX_FloatRect rcMinButton;
  CFX_FloatRect rcMaxButton;
  if (IsFloatBigger(rcClient.top - rcClient.bottom,
                    kButtonWidth * 2 + kPosButtonMinWidth + 2)) {
    rcMinButton = CFX_FloatRect(rcClient.left, rcClient.top - kButtonWidth,
                                rcClient.right, rcClient.top);
    rcMaxButton = CFX_FloatRect(rcClient.left, rcClient.bottom, rcClient.right,
                                rcClient.bottom + kButtonWidth);
  } else {
    float fBWidth =
        (rcClient.top - rcClient.bottom - kPosButtonMinWidth - 2) / 2;
    if (IsFloatBigger(fBWidth, 0)) {
      rcMinButton = CFX_FloatRect(rcClient.left, rcClient.top - fBWidth,
                                  rcClient.right, rcClient.top);
      rcMaxButton = CFX_FloatRect(rcClient.left, rcClient.bottom,
                                  rcClient.right, rcClient.bottom + fBWidth);
    } else {
      if (!SetVisible(false))
        return false;
    }
  }

  ObservedPtr<CPWL_ScrollBar> thisObserved(this);
  if (m_pMinButton) {
    m_pMinButton->Move(rcMinButton, true, false);
    if (!thisObserved)
      return false;
  }
  if (m_pMaxButton) {
    m_pMaxButton->Move(rcMaxButton, true, false);
    if (!thisObserved)
      return false;
  }

  return MovePosButton(false);
}

void CPWL_ScrollBar::DrawThisAppearance(CFX_RenderDevice* pDevice,
                                        const CFX_Matrix& mtUser2Device) {
  CFX_FloatRect rectWnd = GetWindowRect();

  if (IsVisible() && !rectWnd.IsEmpty()) {
    pDevice->DrawFillRect(&mtUser2Device, rectWnd, GetBackgroundColor(),
                          GetTransparency());

    pDevice->DrawStrokeLine(
        &mtUser2Device, CFX_PointF(rectWnd.left + 2.0f, rectWnd.top - 2.0f),
        CFX_PointF(rectWnd.left + 2.0f, rectWnd.bottom + 2.0f),
        ArgbEncode(GetTransparency(), 100, 100, 100), 1.0f);

    pDevice->DrawStrokeLine(
        &mtUser2Device, CFX_PointF(rectWnd.right - 2.0f, rectWnd.top - 2.0f),
        CFX_PointF(rectWnd.right - 2.0f, rectWnd.bottom + 2.0f),
        ArgbEncode(GetTransparency(), 100, 100, 100), 1.0f);
  }
}

bool CPWL_ScrollBar::OnLButtonDown(uint32_t nFlag, const CFX_PointF& point) {
  CPWL_Wnd::OnLButtonDown(nFlag, point);

  if (HasFlag(PWS_AUTOTRANSPARENT)) {
    if (GetTransparency() != 255) {
      SetTransparency(255);
      if (!InvalidateRect(nullptr))
        return true;
    }
  }

  if (m_pPosButton && m_pPosButton->IsVisible()) {
    CFX_FloatRect rcClient = GetClientRect();
    CFX_FloatRect rcPosButton = m_pPosButton->GetWindowRect();
    CFX_FloatRect rcMinArea =
        CFX_FloatRect(rcClient.left, rcPosButton.top, rcClient.right,
                      rcClient.top - kButtonWidth);
    CFX_FloatRect rcMaxArea =
        CFX_FloatRect(rcClient.left, rcClient.bottom + kButtonWidth,
                      rcClient.right, rcPosButton.bottom);

    rcMinArea.Normalize();
    rcMaxArea.Normalize();

    if (rcMinArea.Contains(point)) {
      m_sData.SubBig();
      if (!MovePosButton(true))
        return true;
      NotifyScrollWindow();
    }

    if (rcMaxArea.Contains(point)) {
      m_sData.AddBig();
      if (!MovePosButton(true))
        return true;
      NotifyScrollWindow();
    }
  }

  return true;
}

bool CPWL_ScrollBar::OnLButtonUp(uint32_t nFlag, const CFX_PointF& point) {
  CPWL_Wnd::OnLButtonUp(nFlag, point);

  if (HasFlag(PWS_AUTOTRANSPARENT)) {
    if (GetTransparency() != PWL_SCROLLBAR_TRANSPARENCY) {
      SetTransparency(PWL_SCROLLBAR_TRANSPARENCY);
      if (!InvalidateRect(nullptr))
        return true;
    }
  }

  m_pTimer.reset();
  m_bMouseDown = false;
  return true;
}

void CPWL_ScrollBar::SetScrollInfo(const PWL_SCROLL_INFO& info) {
  if (info == m_OriginInfo)
    return;

  m_OriginInfo = info;
  float fMax =
      std::max(0.0f, info.fContentMax - info.fContentMin - info.fPlateWidth);
  SetScrollRange(0, fMax, info.fPlateWidth);
  SetScrollStep(info.fBigStep, info.fSmallStep);
}

void CPWL_ScrollBar::SetScrollPosition(float pos) {
  pos = m_OriginInfo.fContentMax - pos;
  SetScrollPos(pos);
}

void CPWL_ScrollBar::NotifyLButtonDown(CPWL_Wnd* child, const CFX_PointF& pos) {
  if (child == m_pMinButton)
    OnMinButtonLBDown(pos);
  else if (child == m_pMaxButton)
    OnMaxButtonLBDown(pos);
  else if (child == m_pPosButton)
    OnPosButtonLBDown(pos);
}

void CPWL_ScrollBar::NotifyLButtonUp(CPWL_Wnd* child, const CFX_PointF& pos) {
  if (child == m_pMinButton)
    OnMinButtonLBUp(pos);
  else if (child == m_pMaxButton)
    OnMaxButtonLBUp(pos);
  else if (child == m_pPosButton)
    OnPosButtonLBUp(pos);
}

void CPWL_ScrollBar::NotifyMouseMove(CPWL_Wnd* child, const CFX_PointF& pos) {
  if (child == m_pMinButton)
    OnMinButtonMouseMove(pos);
  else if (child == m_pMaxButton)
    OnMaxButtonMouseMove(pos);
  else if (child == m_pPosButton)
    OnPosButtonMouseMove(pos);
}

void CPWL_ScrollBar::CreateButtons(const CreateParams& cp) {
  CreateParams scp = cp;
  scp.dwBorderWidth = 2;
  scp.nBorderStyle = BorderStyle::kBeveled;
  scp.dwFlags =
      PWS_VISIBLE | PWS_CHILD | PWS_BORDER | PWS_BACKGROUND | PWS_NOREFRESHCLIP;

  if (!m_pMinButton) {
    auto pButton =
        std::make_unique<CPWL_SBButton>(scp, CloneAttachedData(), PSBT_MIN);
    m_pMinButton = pButton.get();
    AddChild(std::move(pButton));
    m_pMinButton->Realize();
  }

  if (!m_pMaxButton) {
    auto pButton =
        std::make_unique<CPWL_SBButton>(scp, CloneAttachedData(), PSBT_MAX);
    m_pMaxButton = pButton.get();
    AddChild(std::move(pButton));
    m_pMaxButton->Realize();
  }

  if (!m_pPosButton) {
    auto pButton =
        std::make_unique<CPWL_SBButton>(scp, CloneAttachedData(), PSBT_POS);
    m_pPosButton = pButton.get();
    ObservedPtr<CPWL_ScrollBar> thisObserved(this);
    if (m_pPosButton->SetVisible(false) && thisObserved) {
      AddChild(std::move(pButton));
      m_pPosButton->Realize();
    }
  }
}

float CPWL_ScrollBar::GetScrollBarWidth() const {
  if (!IsVisible())
    return 0;

  return PWL_SCROLLBAR_WIDTH;
}

void CPWL_ScrollBar::SetScrollRange(float fMin,
                                    float fMax,
                                    float fClientWidth) {
  if (!m_pPosButton)
    return;

  ObservedPtr<CPWL_ScrollBar> thisObserved(this);
  m_sData.SetScrollRange(fMin, fMax);
  m_sData.SetClientWidth(fClientWidth);

  if (IsFloatSmaller(m_sData.ScrollRange.GetWidth(), 0.0f)) {
    m_pPosButton->SetVisible(false);
    // Note, |this| may no longer be viable at this point. If more work needs
    // to be done, check thisObserved.
    return;
  }

  if (!m_pPosButton->SetVisible(true) || !thisObserved)
    return;

  MovePosButton(true);
  // Note, |this| may no longer be viable at this point. If more work needs
  // to be done, check the return value of MovePosButton().
}

void CPWL_ScrollBar::SetScrollPos(float fPos) {
  float fOldPos = m_sData.fScrollPos;
  m_sData.SetPos(fPos);
  if (!IsFloatEqual(m_sData.fScrollPos, fOldPos)) {
    MovePosButton(true);
    // Note, |this| may no longer be viable at this point. If more work needs
    // to be done, check the return value of MovePosButton().
  }
}

void CPWL_ScrollBar::SetScrollStep(float fBigStep, float fSmallStep) {
  m_sData.SetBigStep(fBigStep);
  m_sData.SetSmallStep(fSmallStep);
}

bool CPWL_ScrollBar::MovePosButton(bool bRefresh) {
  DCHECK(m_pMinButton);
  DCHECK(m_pMaxButton);

  if (m_pPosButton->IsVisible()) {
    CFX_FloatRect rcPosArea = GetScrollArea();
    float fBottom = TrueToFace(m_sData.fScrollPos + m_sData.fClientWidth);
    float fTop = TrueToFace(m_sData.fScrollPos);

    if (IsFloatSmaller(fTop - fBottom, kPosButtonMinWidth))
      fBottom = fTop - kPosButtonMinWidth;

    if (IsFloatSmaller(fBottom, rcPosArea.bottom)) {
      fBottom = rcPosArea.bottom;
      fTop = fBottom + kPosButtonMinWidth;
    }

    CFX_FloatRect rcPosButton =
        CFX_FloatRect(rcPosArea.left, fBottom, rcPosArea.right, fTop);

    ObservedPtr<CPWL_ScrollBar> thisObserved(this);
    m_pPosButton->Move(rcPosButton, true, bRefresh);
    if (!thisObserved)
      return false;
  }

  return true;
}

void CPWL_ScrollBar::OnMinButtonLBDown(const CFX_PointF& point) {
  m_sData.SubSmall();
  if (!MovePosButton(true))
    return;

  NotifyScrollWindow();
  m_bMinOrMax = true;
  m_pTimer = std::make_unique<CFX_Timer>(GetTimerHandler(), this, 100);
}

void CPWL_ScrollBar::OnMinButtonLBUp(const CFX_PointF& point) {}

void CPWL_ScrollBar::OnMinButtonMouseMove(const CFX_PointF& point) {}

void CPWL_ScrollBar::OnMaxButtonLBDown(const CFX_PointF& point) {
  m_sData.AddSmall();
  if (!MovePosButton(true))
    return;

  NotifyScrollWindow();
  m_bMinOrMax = false;
  m_pTimer = std::make_unique<CFX_Timer>(GetTimerHandler(), this, 100);
}

void CPWL_ScrollBar::OnMaxButtonLBUp(const CFX_PointF& point) {}

void CPWL_ScrollBar::OnMaxButtonMouseMove(const CFX_PointF& point) {}

void CPWL_ScrollBar::OnPosButtonLBDown(const CFX_PointF& point) {
  m_bMouseDown = true;

  if (m_pPosButton) {
    CFX_FloatRect rcPosButton = m_pPosButton->GetWindowRect();
    m_nOldPos = point.y;
    m_fOldPosButton = rcPosButton.top;
  }
}

void CPWL_ScrollBar::OnPosButtonLBUp(const CFX_PointF& point) {
  if (m_bMouseDown) {
    if (!m_bNotifyForever)
      NotifyScrollWindow();
  }
  m_bMouseDown = false;
}

void CPWL_ScrollBar::OnPosButtonMouseMove(const CFX_PointF& point) {
  if (fabs(point.y - m_nOldPos) < 1)
    return;

  float fOldScrollPos = m_sData.fScrollPos;
  float fNewPos = FaceToTrue(m_fOldPosButton + point.y - m_nOldPos);
  if (m_bMouseDown) {
    if (IsFloatSmaller(fNewPos, m_sData.ScrollRange.fMin)) {
      fNewPos = m_sData.ScrollRange.fMin;
    }

    if (IsFloatBigger(fNewPos, m_sData.ScrollRange.fMax)) {
      fNewPos = m_sData.ScrollRange.fMax;
    }

    m_sData.SetPos(fNewPos);

    if (!IsFloatEqual(fOldScrollPos, m_sData.fScrollPos)) {
      if (!MovePosButton(true))
        return;

      if (m_bNotifyForever)
        NotifyScrollWindow();
    }
  }
}

void CPWL_ScrollBar::NotifyScrollWindow() {
  CPWL_Wnd* pParent = GetParentWindow();
  if (!pParent)
    return;

  pParent->ScrollWindowVertically(m_OriginInfo.fContentMax -
                                  m_sData.fScrollPos);
}

CFX_FloatRect CPWL_ScrollBar::GetScrollArea() const {
  CFX_FloatRect rcClient = GetClientRect();
  if (!m_pMinButton || !m_pMaxButton)
    return rcClient;

  CFX_FloatRect rcMin = m_pMinButton->GetWindowRect();
  CFX_FloatRect rcMax = m_pMaxButton->GetWindowRect();
  float fMinHeight = rcMin.Height();
  float fMaxHeight = rcMax.Height();

  CFX_FloatRect rcArea;
  if (rcClient.top - rcClient.bottom > fMinHeight + fMaxHeight + 2) {
    rcArea = CFX_FloatRect(rcClient.left, rcClient.bottom + fMinHeight + 1,
                           rcClient.right, rcClient.top - fMaxHeight - 1);
  } else {
    rcArea = CFX_FloatRect(rcClient.left, rcClient.bottom + fMinHeight + 1,
                           rcClient.right, rcClient.bottom + fMinHeight + 1);
  }

  rcArea.Normalize();
  return rcArea;
}

float CPWL_ScrollBar::TrueToFace(float fTrue) {
  CFX_FloatRect rcPosArea = GetScrollArea();
  float fFactWidth = m_sData.ScrollRange.GetWidth() + m_sData.fClientWidth;
  fFactWidth = fFactWidth == 0 ? 1 : fFactWidth;
  return rcPosArea.top -
         fTrue * (rcPosArea.top - rcPosArea.bottom) / fFactWidth;
}

float CPWL_ScrollBar::FaceToTrue(float fFace) {
  CFX_FloatRect rcPosArea = GetScrollArea();
  float fFactWidth = m_sData.ScrollRange.GetWidth() + m_sData.fClientWidth;
  fFactWidth = fFactWidth == 0 ? 1 : fFactWidth;
  return (rcPosArea.top - fFace) * fFactWidth /
         (rcPosArea.top - rcPosArea.bottom);
}

void CPWL_ScrollBar::CreateChildWnd(const CreateParams& cp) {
  CreateButtons(cp);
}

void CPWL_ScrollBar::OnTimerFired() {
  PWL_SCROLL_PRIVATEDATA sTemp = m_sData;
  if (m_bMinOrMax)
    m_sData.SubSmall();
  else
    m_sData.AddSmall();

  if (sTemp == m_sData)
    return;

  if (!MovePosButton(true))
    return;

  NotifyScrollWindow();
}
