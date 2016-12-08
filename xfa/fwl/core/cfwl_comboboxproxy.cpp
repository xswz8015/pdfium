// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/core/cfwl_comboboxproxy.h"

#include <memory>
#include <utility>

#include "xfa/fwl/core/cfwl_app.h"
#include "xfa/fwl/core/cfwl_combobox.h"
#include "xfa/fwl/core/cfwl_msgkillfocus.h"
#include "xfa/fwl/core/cfwl_msgmouse.h"
#include "xfa/fwl/core/cfwl_notedriver.h"

CFWL_ComboBoxProxy::CFWL_ComboBoxProxy(
    CFWL_ComboBox* pComboBox,
    const CFWL_App* app,
    std::unique_ptr<CFWL_WidgetProperties> properties,
    CFWL_Widget* pOuter)
    : CFWL_FormProxy(app, std::move(properties), pOuter),
      m_bLButtonDown(false),
      m_bLButtonUpSelf(false),
      m_pComboBox(pComboBox) {}

CFWL_ComboBoxProxy::~CFWL_ComboBoxProxy() {}

void CFWL_ComboBoxProxy::OnProcessMessage(CFWL_Message* pMessage) {
  if (!pMessage)
    return;

  switch (pMessage->GetType()) {
    case CFWL_Message::Type::Mouse: {
      CFWL_MsgMouse* pMsg = static_cast<CFWL_MsgMouse*>(pMessage);
      switch (pMsg->m_dwCmd) {
        case FWL_MouseCommand::LeftButtonDown:
          OnLButtonDown(pMsg);
          break;
        case FWL_MouseCommand::LeftButtonUp:
          OnLButtonUp(pMsg);
          break;
        default:
          break;
      }
      break;
    }
    case CFWL_Message::Type::KillFocus:
      OnFocusChanged(pMessage, false);
      break;
    case CFWL_Message::Type::SetFocus:
      OnFocusChanged(pMessage, true);
      break;
    default:
      break;
  }
  CFWL_Widget::OnProcessMessage(pMessage);
}

void CFWL_ComboBoxProxy::OnDrawWidget(CFX_Graphics* pGraphics,
                                      const CFX_Matrix* pMatrix) {
  m_pComboBox->DrawStretchHandler(pGraphics, pMatrix);
}

void CFWL_ComboBoxProxy::OnLButtonDown(CFWL_Message* pMessage) {
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  CFX_RectF rtWidget;
  GetWidgetRect(rtWidget);
  rtWidget.left = rtWidget.top = 0;

  CFWL_MsgMouse* pMsg = static_cast<CFWL_MsgMouse*>(pMessage);
  if (rtWidget.Contains(pMsg->m_fx, pMsg->m_fy)) {
    m_bLButtonDown = true;
    pDriver->SetGrab(this, true);
  } else {
    m_bLButtonDown = false;
    pDriver->SetGrab(this, false);
    m_pComboBox->ShowDropList(false);
  }
}

void CFWL_ComboBoxProxy::OnLButtonUp(CFWL_Message* pMessage) {
  m_bLButtonDown = false;
  const CFWL_App* pApp = GetOwnerApp();
  if (!pApp)
    return;

  CFWL_NoteDriver* pDriver =
      static_cast<CFWL_NoteDriver*>(pApp->GetNoteDriver());
  pDriver->SetGrab(this, false);
  if (!m_bLButtonUpSelf) {
    m_bLButtonUpSelf = true;
    return;
  }

  CFWL_MsgMouse* pMsg = static_cast<CFWL_MsgMouse*>(pMessage);
  CFX_RectF rect;
  GetWidgetRect(rect);
  rect.left = rect.top = 0;
  if (!rect.Contains(pMsg->m_fx, pMsg->m_fy) &&
      m_pComboBox->IsDropListVisible()) {
    m_pComboBox->ShowDropList(false);
  }
}

void CFWL_ComboBoxProxy::OnFocusChanged(CFWL_Message* pMessage, bool bSet) {
  if (bSet)
    return;

  CFWL_MsgKillFocus* pMsg = static_cast<CFWL_MsgKillFocus*>(pMessage);
  if (!pMsg->m_pSetFocus)
    m_pComboBox->ShowDropList(false);
}
